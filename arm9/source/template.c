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
	PlaySeq("YourFile.sseq", "YourFile.sbnk", "YourFile.swar");

	//PlaySeq("SEQ_BGM_BICYCLE.sseq", "BANK_MUS_WB_BICYCLE.sbnk", "WAVE_MUS_WB_BICYCLE.swar");
	//PlaySeq("SEQ_BGM_DIVING.sseq", "BANK_MUS_WB_DIVING.sbnk", "WAVE_MUS_WB_DIVING.swar");
	//PlaySeq("SEQ_BGM_MSL_01.sseq", "BANK_MUS_WB_MSL_01.sbnk", "WAVE_MUS_WB_MSL_01.swar");
	//PlaySeq("SEQ_BGM_VS_N_2.sseq", "BANK_MUS_WB_VS_N_2.sbnk", "WAVE_MUS_WB_VS_N_2.swar");
	//PlaySeq("SEQ_BGM_VS_SHIN.sseq", "BANK_MUS_WB_VS_SHIN.sbnk", "WAVE_MUS_WB_VS_SHIN.swar");
	//PlaySeq("SEQ_BGM_E_SHIRONA.sseq", "BANK_MUS_WB_E_SHIRONA.sbnk", "WAVE_MUS_WB_E_SHIRONA.swar");
	//PlaySeq("SEQ_BGM_VS_G_CIS.sseq", "BANK_MUS_WB_VS_G_CIS.sbnk", "WAVE_MUS_WB_VS_G_CIS.swar");
	//PlaySeq("SEQ_BGM_R_D_SP.sseq", "BANK_MUS_WB_R_D_SP.sbnk", "WAVE_MUS_WB_R_D_SP.swar");
	//PlaySeq("SEQ_BGM_R_F.sseq", "BANK_MUS_WB_R_F.sbnk", "WAVE_MUS_WB_R_F.swar");
	//PlaySeq("SEQ_BGM_D_CHAMPROAD.sseq", "BANK_MUS_WB_D_CHAMPROAD.sbnk", "WAVE_MUS_WB_D_CHAMPROAD.swar");
	//PlaySeq("SEQ_BGM_VS_SHIRONA.sseq", "BANK_MUS_WB_VS_SHIRONA.sbnk", "WAVE_MUS_WB_VS_SHIRONA.swar");

	for(;;)
	{
		swiWaitForVBlank();

		scanKeys();
		if (keysDown() & KEY_START) break;

		/*
		touchPosition touch;
		touchRead(&touch);
		iprintf("\x1b[10;0HTouch x = %04i, %04i\n", touch.rawx, touch.px);
		iprintf("Touch y = %04i, %04i\n", touch.rawy, touch.py);
		*/
	}

	return 0;
}
