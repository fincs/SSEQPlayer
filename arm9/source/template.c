#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sndcommon.h>
#include <filesystem.h>

int error(const char* txt)
{
	printf("\nERROR: %s\n", txt);
	return 1;
}

int defaultLoad(void** sseqData, void** sbnkData, void* swarData[])
{
	*sseqData = NULL;
	*sbnkData = NULL;
	memset(swarData, 0, 4*sizeof(void*));

	*sseqData = LoadFile("YourFile.sseq", NULL);
	if (!*sseqData) return error("Can't load SSEQ!");
	*sbnkData = LoadFile("YourFile.sbnk", NULL);
	if (!*sbnkData) { free(*sseqData); return error("Can't load SBNK!"); }
	swarData[0] = LoadFile("YourFile.swar", NULL);
	swarData[1] = LoadFile("YourFile2.swar", NULL);
	swarData[2] = LoadFile("YourFile3.swar", NULL);
	swarData[3] = LoadFile("YourFile4.swar", NULL);
	return 0;
}

int argvLoad(int argc, char* argv[], void** sseqData, void** sbnkData, void* swarData[])
{
	*sseqData = NULL;
	*sbnkData = NULL;
	memset(swarData, 0, 4*sizeof(void*));

	*sseqData = LoadFile(argv[0], NULL);
	if (!*sseqData) return error("Can't load SSEQ!");
	*sbnkData = LoadFile(argv[1], NULL);
	if (!*sbnkData) { free(*sseqData); return error("Can't load SBNK!"); }
	swarData[0] = LoadFile(argv[2], NULL);
	if (argc >= 4) swarData[1] = LoadFile(argv[3], NULL);
	if (argc >= 5) swarData[2] = LoadFile(argv[4], NULL);
	if (argc >= 6) swarData[3] = LoadFile(argv[5], NULL);
	return 0;
}

void *g_sseqData, *g_sbnkData;
void* g_swarData[4];

void anykey()
{
	iprintf("Press any key to exit...\n");
	for(;;)
	{
		scanKeys();
		if (keysDown())
			break;
	}
}

int main(int argc, char* argv[])
{
	consoleDemoInit();
	InstallSoundSys();

	iprintf("\n\n\tSSEQ player PoC v1\n");
	iprintf("\tfincs.drunkencoders.com\n");
	iprintf("\tgithub.com/fincs");

	if (argc < 4)
	{
		if (!nitroFSInit())
		{
			error("Can't initialize NitroFS!");
			anykey();
			return 0;
		}
		if (defaultLoad(&g_sseqData, &g_sbnkData, g_swarData) != 0)
		{
			anykey();
			return 0;
		}
	}else
	{
		if (argvLoad(argc-1, argv+1, &g_sseqData, &g_sbnkData, g_swarData) != 0)
		{
			anykey();
			return 0;
		}
	}

	PlaySeq(g_sseqData, g_sbnkData, g_swarData);

	for(;;)
	{
		swiWaitForVBlank();

		scanKeys();
		if (keysDown() & KEY_START) break;
		if (keysDown() & KEY_B) StopSeq();
	}

	return 0;
}
