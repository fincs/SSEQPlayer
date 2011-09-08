#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <sndcommon.h>

static void sndsysMsgHandler(int, void*);

void InstallSoundSys()
{
	/* Install FIFO */
	fifoSetDatamsgHandler(FIFO_SNDSYS, sndsysMsgHandler, 0);
}

static void sndsysMsgHandler(int bytes, void* user_data)
{
	sndsysMsg msg;
	fifoGetDatamsg(FIFO_SNDSYS, bytes, (u8*) &msg);
}

/* The following code must be rethought: */

/*
int PlaySmp(sndreg_t* smp, int a, int d, int s, int r, int vol, int vel, int pan)
{
	sndsysMsg msg;
	msg.msg = SNDSYS_PLAY;
	msg.sndreg = *smp;
	msg.a = (u8) a;
	msg.d = (u8) d;
	msg.s = (u8) s;
	msg.r = (u8) r;
	msg.vol = (u8) vol;
	msg.vel = (u8) vel;
	msg.pan = (s8) pan;
	fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg);
	return (int) fifoGetRetValue(FIFO_SNDSYS);
}

void StopSmp(int handle)
{
	sndsysMsg msg;
	msg.msg = SNDSYS_STOP;
	msg.ch = handle;
	fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg);
}
*/

static bool LoadFile(data_t* pData, const char* fname)
{
	FILE* f = fopen(fname, "rb");
	fseek(f, 0, SEEK_END);
	pData->size = ftell(f);
	rewind(f);
	pData->data = malloc(pData->size);
	fread(pData->data, 1, pData->size, f);
	fclose(f);
	DC_FlushRange(pData->data, pData->size);
	return true;
}

void PlaySeq(const char* seqFile, const char* bnkFile, const char* warFile)
{
	sndsysMsg msg;
	msg.msg = SNDSYS_PLAYSEQ;

	LoadFile(&msg.seq, seqFile);
	LoadFile(&msg.bnk, bnkFile);
	LoadFile(&msg.war, warFile);

	fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg);
}

void StopSeq()
{
	sndsysMsg msg;
	msg.msg = SNDSYS_STOPSEQ;
	fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg);
}
