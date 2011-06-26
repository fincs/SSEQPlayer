#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndcommon.h>

#define ADJUST_FREQ(baseFreq,noteN,baseN) (((int)(baseFreq)*freqTable[noteN])/freqTable[baseN])

static const int freqTable[128] = {
	8, 9, 9, 10, 11, 11, 12, 13, 14, 15, 15,
	16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31,
	33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62,
	65, 69, 73, 78, 83, 87, 92, 98, 104, 110, 117, 123,
	131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
	262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
	523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
	1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
	2093, 2217, 2349, 2489, 2637, 2794, 2969, 3136, 3322, 3520, 3729, 3951,
	4185, 4435, 4599, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
	8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544
};

// info about the sample
typedef struct tagSwavInfo
{
	u8  nWaveType;    // 0 = PCM8, 1 = PCM16, 2 = (IMA-)ADPCM
	u8  bLoop;        // Loop flag = TRUE|FALSE
	u16 nSampleRate;  // Sampling Rate
	u16 nTime;        // (ARM7_CLOCK / nSampleRate) [ARM7_CLOCK: 33.513982MHz / 2 = 1.6756991 E +7]
	u16 nLoopOffset;  // Loop Offset (expressed in words (32-bits))
	u32 nNonLoopLen;  // Non Loop Length (expressed in words (32-bits))
} SWAVINFO;

#define SOUND_FORMAT(a) (((int)(a))<<29)
#define SOUND_LOOP(a) ((a) ? SOUND_REPEAT : SOUND_ONE_SHOT)
#define GETSAMP(a) ((void*)((char*)(a)+sizeof(SWAVINFO)))

SWAVINFO* GetWav(void* war, int id)
{
	return (SWAVINFO*)((int)war + ((int*)((int)war+60))[id]);
}

u32 GetInstr(void* bnk, int id)
{
	return *(u32*)((int)bnk + 60 + 4*id);
}

#define INST_TYPE(a) ((a) & 0xFF)
#define INST_OFF(a) ((a) >> 8)

#define GETINSTDATA(bnk,a) ( (u8*) ( (int)(bnk) + (int)INST_OFF(a) ) )

typedef struct
{
	u8 vol, vel, expr, pan;
} playinfo_t;

typedef struct
{
	u16 wavid;
	u16 warid;
	u8 tnote;
	u8 a,d,s,r;
	u8 pan;
} notedef_t;

int ds_freechn2(int prio)
{
	register int i;
	for(i = 0; i < 16; i ++)
		if (!SCHANNEL_ACTIVE(i) && ADSR_ch[i].state != ADSR_START)
			return i;
	for(i = 0; i < 16; i ++)
		if (ADSR_ch[i].prio < prio)
		{
			if (ADSR_ch[i].extra)
			{
#ifdef LOG_CUTOFF
				nocashMessage("Cutoff prevention #1");
#endif
				*ADSR_ch[i].extra = 0;
			}
			return i;
		}
	return -1;
}

int _Note(void* bnk, void* war, int instr, int note, int prio, playinfo_t* playinfo, int* extra)
{
	int ch = ds_freechn2(prio);
	if (ch < 0) return -1;

	ADSR_stat_t* chstat = ADSR_ch + ch;
	
	u32 inst = GetInstr(bnk, instr);
	u8* insdata = GETINSTDATA(bnk, inst);
	notedef_t* notedef = NULL;
	SWAVINFO* wavinfo = NULL;
	int fRecord = INST_TYPE(inst);
	if (fRecord == 0) return -1;
	else if (fRecord < 16) notedef = (notedef_t*) insdata;
	else if (fRecord == 16)
	{
		if ((insdata[0] <= note) && (note <= insdata[1]))
		{
			int rn = note - insdata[0];
			notedef = (notedef_t*) (insdata + 2 + 2 + rn*(2+sizeof(notedef_t)));
		}else return -1;
	}else if (fRecord == 17)
	{
		int reg;
		for(reg = 0; reg < 8; reg ++)
			if (note <= insdata[reg]) break;
		if (reg == 8) return -1;
		notedef = (notedef_t*) (insdata + 8 + 2 + reg*(2+sizeof(notedef_t)));
	}else return -1;

	wavinfo = GetWav(war, notedef->wavid);
	chstat->reg.CR = SOUND_FORMAT(wavinfo->nWaveType) | SOUND_LOOP(wavinfo->bLoop) | SCHANNEL_ENABLE;
	chstat->reg.SOURCE = (u32)GETSAMP(wavinfo);
	chstat->reg.TIMER = SOUND_FREQ(ADJUST_FREQ((int)wavinfo->nSampleRate, note, notedef->tnote));
	chstat->reg.REPEAT_POINT = wavinfo->nLoopOffset;
	chstat->reg.LENGTH = wavinfo->nNonLoopLen;
	
	chstat->vol = playinfo->vol;
	chstat->vel = playinfo->vel;
	chstat->expr = playinfo->expr;
	chstat->pan = playinfo->pan;
	chstat->pan2 = notedef->pan;
	chstat->a = CnvAttk(notedef->a);
	chstat->d = CnvFall(notedef->d)<<2; // HACK: please help
	chstat->s = CnvSust(notedef->s);
	chstat->r = CnvFall(notedef->r)<<2; // HACK: please help
	chstat->prio = prio;
	chstat->extra = extra;
	chstat->state = ADSR_START;

	return ch;
}

void _NoteStop(int n)
{
	ADSR_stat_t* chstat = ADSR_ch + n;
	chstat->state = ADSR_RELEASE;
	chstat->extra = NULL;
}

typedef struct
{
	int count;
	int handle;
} notestat_t;

typedef struct
{
	notestat_t notes[16];
	int count;
	int pos;
	int ret;
	int prio;
	u16 patch;
	u16 waitmode;
	playinfo_t playinfo;
} trackstat_t;

int ntracks = 0;
u8* seqData = NULL;
void* seqBnk = NULL, *seqWar = NULL;
trackstat_t tracks[16];

#define SEQ_READ8(pos) seqData[(pos)]
#define SEQ_READ16(pos) ((u16)seqData[(pos)] | ((u16)seqData[(pos)+1] << 8))
#define SEQ_READ24(pos) ((u32)seqData[(pos)] | ((u32)seqData[(pos)+1] << 8) | ((u32)seqData[(pos)+2] << 16))

void PlaySeq(data_t* seq, data_t* bnk, data_t* war)
{
	seqBnk = bnk->data;
	seqWar = war->data;

	// Load sequence data
	seqData = (u8*)seq->data + ((u32*)seq->data)[6];
	ntracks = 0;

	if (*seqData != 0xFE) return;
	int i, pos = 3;
	for (i = 1, ntracks = 1;; i ++, ntracks ++)
	{
		if (SEQ_READ8(pos) != 0x93) break; pos += 2;
		memset(tracks + i, 0, sizeof(trackstat_t));
		tracks[i].pos = SEQ_READ24(pos); pos += 3;
		tracks[i].playinfo.vol = 64;
		tracks[i].playinfo.vel = 64;
		tracks[i].playinfo.expr = 127;
		tracks[i].playinfo.pan = 64;
		tracks[i].prio = 64;
	}

	// Prepare first track
	memset(tracks + 0, 0, sizeof(trackstat_t));
	tracks[0].pos = pos;
	tracks[0].playinfo.vol = 64;
	tracks[0].playinfo.vel = 64;
	tracks[0].playinfo.expr = 127;
	tracks[0].playinfo.pan = 64;
	tracks[0].prio = 64;
	seq_bpm = 120;
}

volatile int seq_bpm = 0;

void track_tick(int n);

void seq_tick()
{
	int i;
#ifdef LOG_SEQ
	nocashMessage("Tick!");
#endif
	for (i = 0; i < ntracks; i ++)
		track_tick(i);
}

int read_vl(int* pos)
{
	int v = 0;
	for(;;)
	{
		int data = SEQ_READ8(*pos); (*pos) ++;
		v = (v << 7) | (data & 0x7F);
		if (!(data & 0x80)) break;
	}
	return v;
}

int seq_freenote(notestat_t* notes)
{
	register int i;
	for(i = 0; i < 16; i ++) if(!notes[i].count) return i;
	return -1;
}

void seq_updatenotes(notestat_t* notes, playinfo_t* info)
{
	int i = 0;
	for (i = 0; i < 16; i ++)
	{
		if (!notes[i].count) continue;
		ADSR_stat_t* chstat = ADSR_ch + notes[i].handle;
		chstat->vol = info->vol;
		chstat->expr = info->expr;
		chstat->pan = info->pan;
	}
}

void track_tick(int n)
{
	trackstat_t* track = tracks + n;

	// Run each note:
	int i;
	for (i = 0; i < 8; i ++)
	{
		notestat_t* note = track->notes + i;
		if (note->count)
		{
			ADSR_stat_t* chstat = ADSR_ch + note->handle;
			if (chstat->state == ADSR_NONE || !SCHANNEL_ACTIVE(note->handle))
			{
				chstat->state = ADSR_NONE;
				chstat->extra = NULL;
#ifdef LOG_CUTOFF
				nocashMessage("Cutoff prevention #3");
#endif
				note->count = 0;
				continue;
			}
			note->count --;
			if (!note->count) _NoteStop(note->handle);
		}
	}

	if (track->count)
	{
		track->count --;
		if (track->count) return;
	}

	while (!track->count)
	{
#ifdef LOG_SEQ
		int oldpos = track->pos;
#endif
		int cmd = SEQ_READ8(track->pos); track->pos ++;
#ifdef LOG_SEQ
		char buf[64];
		siprintf(buf, "%02X-%08X-%X", cmd, (int)(seqData + oldpos), oldpos);
		nocashMessage(buf);
#endif
		if (cmd < 0x80)
		{
#ifdef LOG_SEQ
			nocashMessage("NOTE-ON");
#endif
			// NOTE-ON
			u8  vel = SEQ_READ8(track->pos); track->pos ++;
			int len = read_vl(&track->pos);
			i = seq_freenote(track->notes);
			if (track->waitmode) track->count = len;
			if (i < 0) continue;

			track->playinfo.vel = vel;
			int handle = _Note(seqBnk, seqWar, track->patch, cmd, track->prio, &track->playinfo, &track->notes[i].count);
			if (handle < 0) continue;

			track->notes[i].count = len;
			track->notes[i].handle = handle;
		}else switch(cmd)
		{
			case 0x80: // REST
			{
#ifdef LOG_SEQ
				nocashMessage("REST");
#endif
				track->count = read_vl(&track->pos);
				break;
			}
			case 0x81: // PATCH CHANGE
			{
#ifdef LOG_SEQ
				nocashMessage("PATCH");
#endif
				track->patch = read_vl(&track->pos);
				break;
			}
			case 0x94: // JUMP
			{
#ifdef LOG_SEQ
				nocashMessage("JUMP");
#endif
				track->pos = SEQ_READ24(track->pos);
				break;
			}
			case 0x95: // CALL
			{
#ifdef LOG_SEQ
				nocashMessage("CALL");
#endif
				int dest = SEQ_READ24(track->pos);
				track->ret = track->pos + 3;
				track->pos = dest;
				break;
			}
			case 0xFD: // RET
			{
#ifdef LOG_SEQ
				nocashMessage("RET");
#endif
				track->pos = track->ret;
				break;
			}
			case 0xC0: // PAN
			{
#ifdef LOG_SEQ
				nocashMessage("PAN");
#endif
				track->playinfo.pan = SEQ_READ8(track->pos); track->pos ++;
				seq_updatenotes(track->notes, &track->playinfo);
				break;
			}
			case 0xC1: // VOL
			{
#ifdef LOG_SEQ
				nocashMessage("VOL");
#endif
				track->playinfo.vol = SEQ_READ8(track->pos); track->pos ++;
				seq_updatenotes(track->notes, &track->playinfo);
				break;
			}
			case 0xC2: // MASTER VOL
			{
#ifdef LOG_SEQ
				nocashMessage("MASTER VOL");
#endif
				REG_MASTER_VOLUME = SEQ_READ8(track->pos); track->pos ++;
				break;
			}
			case 0xC3: // TRANSPOSE
			case 0xC4: // PITCH BEND
			case 0xC5: // PITCH BEND RANGE
			case 0xC8: // TIE
			case 0xC9: // PORTAMENTO
			case 0xCA: // MODULATION DEPTH
			case 0xCB: // MODULATION SPEED
			case 0xCC: // MODULATION TYPE
			case 0xCD: // MODULATION RANGE
			case 0xCE: // PORTAMENTO ON/OFF
			case 0xCF: // PORTAMENTO TIME
			case 0xD0: // ATTACK
			case 0xD1: // DECAY
			case 0xD2: // SUSTAIN
			case 0xD3: // RELEASE
			case 0xD4: // LOOP START
			case 0xD6: // PRINT VAR
			{
				// TODO
#ifdef LOG_SEQ
				nocashMessage("DUMMY1");
#endif
				track->pos ++;
				break;
			}
			case 0xC6: // PRIORITY
			{
#ifdef LOG_SEQ
				nocashMessage("PRIORITY");
#endif
				track->prio = SEQ_READ8(track->pos); track->pos ++;
				break;
			}
			case 0xC7: // NOTEWAIT
			{
#ifdef LOG_SEQ
				nocashMessage("NOTEWAIT");
#endif
				track->waitmode = SEQ_READ8(track->pos); track->pos ++;
				break;
			}
			case 0xFC: // LOOP END
			{
				// TODO
#ifdef LOG_SEQ
				nocashMessage("DUMMY0");
#endif
				break;
			}
			case 0xD5: // EXPR
			{
#ifdef LOG_SEQ
				nocashMessage("EXPR");
#endif
				track->playinfo.expr = SEQ_READ8(track->pos); track->pos ++;
				seq_updatenotes(track->notes, &track->playinfo);
				break;
			}
			case 0xE0: // MODULATION DELAY
			case 0xE3: // SWEEP PITCH
			{
				// TODO
#ifdef LOG_SEQ
				nocashMessage("DUMMY2");
#endif
				track->pos += 2;
				break;
			}
			case 0xE1: // TEMPO
			{
#ifdef LOG_SEQ
				nocashMessage("TEMPO");
#endif
				seq_bpm = SEQ_READ16(track->pos); track->pos += 2;
				break;
			}
			case 0xFF: // END
			{
#ifdef LOG_SEQ
				nocashMessage("END");
#endif
				track->pos --;
				return;
			}
		}
	}
}
