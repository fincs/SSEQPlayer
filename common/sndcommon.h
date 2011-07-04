#pragma once

#define FIFO_SNDSYS FIFO_USER_01

void InstallSoundSys();

enum
{
	/*SNDSYS_PLAY = 1, SNDSYS_STOP,*/ SNDSYS_PLAYSEQ
};

typedef struct
{
	u32 CR, SOURCE;
	u16 TIMER, REPEAT_POINT;
	u32 LENGTH;
} sndreg_t;

typedef struct
{
	void* data;
	int size;
} data_t;

typedef struct
{
	int msg;
	union
	{
		/*
		struct
		{
			sndreg_t sndreg;
			u8 a,d,s,r;
			u8 vol, vel; u8 pan; u8 padding;
		};
		int ch;
		*/
		struct
		{
			data_t seq;
			data_t bnk;
			data_t war;
		};
	};
} sndsysMsg;

#define fifoRetWait(ch) while(!fifoCheckValue32(ch))
#define fifoRetValue(ch) fifoGetValue32(ch)

static inline u32 fifoGetRetValue(int ch)
{
	fifoRetWait(ch);
	return fifoRetValue(ch);
}

#ifdef ARM7

enum { ADSR_NONE = 0, ADSR_START, ADSR_ATTACK, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE };

#define ADSR_MIXVOL(a,b) (((a)*(b))/127)
#define ADSR_MIXVOL3(a,b,c) (((a)*(b)*(c))/16129)
//#define ADSR_MIXVOL4(a,b,c,d) ((int)((u32)((a)*(b)*(c)*(d))/2048383))
#define SCHANNEL_ACTIVE(ch) (SCHANNEL_CR(ch) & SCHANNEL_ENABLE)

#define ADSR_K_AMP2VOL 723
#define ADSR_THRESHOLD (ADSR_K_AMP2VOL*128)

typedef struct
{
	int state;
	int vol,vel,expr,pan,pan2;
	int ampl;
	int a,d,s,r;
	int prio;
	int* extra;
	
	sndreg_t reg;
} ADSR_stat_t;

extern ADSR_stat_t ADSR_ch[16];

volatile extern int seq_bpm;

void seq_tick();

void PlaySeq(data_t* seq, data_t* bnk, data_t* war);

int ds_freechn();
int ds_freepsg();
int ds_freenoise();

int CnvAttk(int attk);
int CnvFall(int fall);
int CnvSust(int sust);

#endif

#ifdef ARM9

/*
int PlaySmp(sndreg_t* smp, int a, int d, int s, int r, int vol, int vel, int pan);
void StopSmp(int handle);
*/
void PlaySeq(const char* seqFile, const char* bnkFile, const char* warFile);

#endif
