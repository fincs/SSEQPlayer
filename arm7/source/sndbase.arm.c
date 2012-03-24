#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <sndcommon.h>

static void sound_timer();
static void sndsysMsgHandler(int, void*);

void InstallSoundSys()
{
	/* Power sound on */
	powerOn(POWER_SOUND);
	writePowerManagement(PM_CONTROL_REG, ( readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE ) | PM_SOUND_AMP );
	REG_SOUNDCNT |= SOUND_ENABLE;
	REG_MASTER_VOLUME = 127;

	/* Install timer */
	timerStart(1, ClockDivider_64, -2728, sound_timer);

	/* Install FIFO */
	fifoSetDatamsgHandler(FIFO_SNDSYS, sndsysMsgHandler, 0);

	/* Clear track-channel assignations */
	register int i;
	for (i = 0; i < 16; i ++)
	{
		ADSR_ch[i].track = -1;
		ADSR_ch[i].prio = 0;
	}
}

static void ADSR_tick();

static void sound_timer()
{
	static volatile int v = 0;

	ADSR_tick();
	
	while (v > 240)
		v -= 240, seq_tick();
	v += seq_bpm;

#ifdef LOG_SEQ
	char tt[20];
	siprintf(tt, "%X", v);
	nocashMessage(tt);
#endif
}

ADSR_stat_t ADSR_ch[16];
static u16 ADSR_vol[16];

volatile int ADSR_mastervolume = 127;

static void ADSR_tickchn(int);

static void ADSR_tick()
{
	register int i;
	for(i = 0; i < 16; i ++)
		ADSR_tickchn(i);
}

static void ADSR_tickchn(int ch)
{
	ADSR_stat_t* chstat = ADSR_ch + ch;
#define AMPL chstat->ampl
#define VOL chstat->vol
#define VEL chstat->vel
#define EXPR chstat->expr
#define PAN chstat->pan
#define PAN2 chstat->pan2
#define REG chstat->reg
#define ATKRATE chstat->a
#define DECRATE chstat->d
#define SUSLEVL chstat->s
#define RELRATE chstat->r
#define SETSTATE(s) chstat->state = (s)

	ADSR_vol[ch] = 0;

	if (chstat->state != ADSR_START && !SCHANNEL_ACTIVE(ch))
	{
_kill_chn:
		SETSTATE(ADSR_NONE);
		chstat->count = 0;
		chstat->track = -1;
		chstat->prio = 0;
		SCHANNEL_CR(ch) = 0;
		return;
	}

	switch (chstat->state)
	{
		case ADSR_NONE: return;
		case ADSR_SUSTAIN:
			break;
		case ADSR_START:
			SCHANNEL_CR(ch) = 0;
			SCHANNEL_SOURCE(ch) = REG.SOURCE;
			SCHANNEL_TIMER(ch) = -REG.TIMER;
			SCHANNEL_REPEAT_POINT(ch) = REG.REPEAT_POINT;
			SCHANNEL_LENGTH(ch) = REG.LENGTH;
			SCHANNEL_CR(ch) = REG.CR;
			AMPL = -ADSR_THRESHOLD;
			SETSTATE(ADSR_ATTACK);
		case ADSR_ATTACK:
			AMPL = (ATKRATE * AMPL) / 255;
			if (AMPL == 0) SETSTATE(ADSR_DECAY);
			break;
		case ADSR_DECAY:
			AMPL -= DECRATE;
			if (AMPL <= SUSLEVL) AMPL = SUSLEVL, SETSTATE(ADSR_SUSTAIN);
			break;
		case ADSR_RELEASE:
			AMPL -= RELRATE;
			if (AMPL <= -ADSR_THRESHOLD)
				goto _kill_chn;
			break;
	}

	// Update the modulation params
	int modParam = 0, modType = chstat->modType;
	do
	{
		if (chstat->modDelayCnt < chstat->modDelay)
		{
			chstat->modDelayCnt ++;
			break;
		}

		u16 speed = (u16)chstat->modSpeed << 6;
		u16 counter = (chstat->modCounter + speed) >> 8;
	
		while (counter >= 0x80)
			counter -= 0x80;
	
		chstat->modCounter += speed;
		chstat->modCounter &= 0xFF;
		chstat->modCounter |= counter << 8;

		modParam = GetSoundSine(chstat->modCounter >> 8) * chstat->modRange * chstat->modDepth;
	}while(0);

	modParam >>= 8;

#ifdef LOG_SEQ
	char buf[30];
	siprintf(buf, "%02X %02X %02X %02X %04X", chstat->modDepth, chstat->modSpeed, chstat->modType, chstat->modRange, chstat->modDelay);
	nocashMessage(buf);
#endif

#define CONV_VOL(a) (CnvSust(a)>>7)
#define SOUND_VOLDIV(n) ((n) << 8)

	int totalvol = CONV_VOL(ADSR_mastervolume);
	totalvol += CONV_VOL(VOL);
	totalvol += CONV_VOL(EXPR);
	totalvol += CONV_VOL(VEL);
	totalvol += AMPL >> 7;
	if (modType == 1)
	{
		totalvol += modParam;
		if (totalvol > 0) totalvol = 0;
	}
	totalvol += 723;
	if (totalvol < 0) totalvol = 0;

	u32 res = swiGetVolumeTable(totalvol);

	int pan = (int)PAN + (int)PAN2 - 64;
	if (modType == 2) pan += modParam;
	if (pan < 0) pan = 0;
	if (pan > 127) pan = 127;

	u32 cr = SCHANNEL_CR(ch) &~ (SOUND_VOL(0x7F) | SOUND_VOLDIV(3) | SOUND_PAN(0x7F));
	cr |= SOUND_VOL(res) | SOUND_PAN(pan);
	if (totalvol < (-240 + 723)) cr |= SOUND_VOLDIV(3);
	else if (totalvol < (-120 + 723)) cr |= SOUND_VOLDIV(2);
	else if (totalvol < (-60 + 723)) cr |= SOUND_VOLDIV(1);
	
	ADSR_vol[ch] = ((cr & SOUND_VOL(0x7F)) << 4) >> ((cr & SOUND_VOLDIV(3)) >> 8);
	SCHANNEL_CR(ch) = cr;
	u16 timer = REG.TIMER;
	if (modType == 0) timer = AdjustFreq(timer, modParam);
	SCHANNEL_TIMER(ch) = -timer;

#undef AMPL
#undef VOL
#undef VEL
#undef PAN
#undef PAN2
#undef REG
#undef ATKRATE
#undef DECRATE
#undef SUSLEVL
#undef RELRATE
#undef SETSTATE
}

static u8 pcmChnArray[] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
static u8 psgChnArray[] = { 13, 12, 11, 10, 9, 8 };
static u8 noiseChnArray[] = { 15, 14 };
static u8 arraySizes[] = { sizeof(pcmChnArray), sizeof(psgChnArray), sizeof(noiseChnArray) };
static u8* arrayArray[] = { pcmChnArray, psgChnArray, noiseChnArray };

int ds_allocchn(int type, int prio)
{
	u8* chnArray = arrayArray[type];
	u8 arraySize = arraySizes[type];

	int i;
	int curChnNo = -1;
	for (i = 0; i < arraySize; i ++)
	{
		int thisChnNo = chnArray[i];
		ADSR_stat_t* thisChn = ADSR_ch + thisChnNo;
		ADSR_stat_t* curChn = ADSR_ch + curChnNo;
		if (curChnNo != -1 && thisChn->prio >= curChn->prio)
		{
			if (thisChn->prio != curChn->prio)
				continue;
			if (ADSR_vol[curChnNo] <= ADSR_vol[thisChnNo])
				continue;
		}
		curChnNo = thisChnNo;
	}

	if (curChnNo == -1 || prio < ADSR_ch[curChnNo].prio) return -1;
	return curChnNo;
}

// Adapted from VGMTrans

int CnvAttk(int attk)
{
	static const u8 lut[] =
	{
		0x00, 0x01, 0x05, 0x0E, 0x1A, 0x26, 0x33, 0x3F, 0x49, 0x54,
		0x5C, 0x64, 0x6D, 0x74, 0x7B, 0x7F, 0x84, 0x89, 0x8F
	};
	
	return (attk >= 0x6D) ? lut[0x7F-attk] : (0xFF-attk);
}

int CnvFall(int fall)
{
	if      (fall == 0x7F) return 0xFFFF;
	else if (fall == 0x7E) return 0x3C00;
	else if (fall < 0x32)  return ((fall<<1)+1) & 0xFFFF;
	else                   return (0x1E00/(0x7E - fall)) & 0xFFFF;
}

int CnvSust(int sust)
{
	static const s16 lut[] =
	{
		-32768, -722, -721, -651, -601, -562, -530, -503,
		-480, -460, -442, -425, -410, -396, -383, -371,
		-360, -349, -339, -330, -321, -313, -305, -297,
		-289, -282, -276, -269, -263, -257, -251, -245,
		-239, -234, -229, -224, -219, -214, -210, -205,
		-201, -196, -192, -188, -184, -180, -176, -173,
		-169, -165, -162, -158, -155, -152, -149, -145,
		-142, -139, -136, -133, -130, -127, -125, -122,
		-119, -116, -114, -111, -109, -106, -103, -101,
		-99, -96, -94, -91, -89, -87, -85, -82,
		-80, -78, -76, -74, -72, -70, -68, -66,
		-64, -62, -60, -58, -56, -54, -52, -50,
		-49, -47, -45, -43, -42, -40, -38, -36,
		-35, -33, -31, -30, -28, -27, -25, -23,
		-22, -20, -19, -17, -16, -14, -13, -11,
		-10, -8, -7, -6, -4, -3, -1, 0
	};

	return (int)lut[sust] << 7;
}

int GetSoundSine(int arg)
{
	static const int lut_size = 32;
	static const s8 lut[] =
	{
		0, 6, 12, 19, 25, 31, 37, 43, 49, 54, 60, 65, 71, 76, 81, 85, 90, 94,
		98, 102, 106, 109, 112, 115, 117, 120, 122, 123, 125, 126, 126, 127, 127
	};

	if (arg < 1*lut_size) return  lut[arg];
	if (arg < 2*lut_size) return  lut[2*lut_size - arg];
	if (arg < 3*lut_size) return -lut[arg - 2*lut_size];
	/*else*/              return -lut[4*lut_size - arg];
}

void sndsysMsgHandler(int bytes, void* user_data)
{
	sndsysMsg msg;
	fifoGetDatamsg(FIFO_SNDSYS, bytes, (u8*) &msg);

	switch(msg.msg)
	{
		/* The following code must be rethought */
		/*
		case SNDSYS_PLAY:
		{
			int ch = ds_freechn();
			if (ch < 0) goto _play_ret;

			ADSR_stat_t* chstat = ADSR_ch + ch;

			chstat->reg = msg.sndreg;
			chstat->a = CnvAttk(msg.a);
			chstat->d = CnvFall(msg.d);
			chstat->s = CnvSust(msg.s);
			chstat->r = CnvFall(msg.r);
			chstat->vol = msg.vol;
			chstat->vel = msg.vel;
			chstat->expr = 0x7F;
			chstat->pan = msg.pan;
			chstat->state = ADSR_START;

_play_ret:
			fifoSendValue32(FIFO_SNDSYS, (u32) ch);
			return;
		}

		case SNDSYS_STOP:
		{
			ADSR_stat_t* chstat = ADSR_ch + msg.ch;
			chstat->state = ADSR_RELEASE;
			return;
		}
		*/

		case SNDSYS_PLAYSEQ:
		{
			PlaySeq(msg.seq, msg.bnk, msg.war);
			return;
		}

		case SNDSYS_STOPSEQ:
		{
			StopSeq();
			return;
		}
	}
}
