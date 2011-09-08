#include <nds.h>
#include <stdio.h>
#include <sndcommon.h>
#include <filesystem.h>

int main()
{
	consoleDemoInit();
	InstallSoundSys();

	if (!nitroFSInit())
	{
		iprintf("Filesystem FAIL");
		for(;;) swiWaitForVBlank();
	}

	iprintf("\n\n\tSSEQ player PoC v1\n");
	iprintf("\tfincs.drunkencoders.com\n");
	iprintf("\tgithub.com/fincs");

	// Change this line!
	PlaySeq("YourFile.sseq", "YourFile.sbnk", "YourFile.swar", "YourFile2.swar", "YourFile3.swar", "YourFile4.swar");

	for(;;)
	{
		swiWaitForVBlank();

		scanKeys();
		if (keysDown() & KEY_START) break;
		if (keysDown() & KEY_B) StopSeq();

		/*
		touchPosition touch;
		touchRead(&touch);
		iprintf("\x1b[10;0HTouch x = %04i, %04i\n", touch.rawx, touch.px);
		iprintf("Touch y = %04i, %04i\n", touch.rawy, touch.py);
		*/
	}

	return 0;
}
