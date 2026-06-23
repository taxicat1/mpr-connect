#include <stdio.h>
#include <nds.h>
#include <nds/ndstypes.h>

#include "slot1_op.h"
#include "vcount_spinwait.h"
#include "hardware_mode.h"
#include "bin_memcpy32.h"
#include "game_param.h"
#include "get_eoo.h"
#include "pkmn_game_codes.h"
#include "top_screen_img.h"
#include "ds_mode_9.h"


static void prepareBoot() {
	// Make the system think we originally booted from Slot-1
	const tNDSHeader* slot1_header = Slot1_GetHeader();
	u32 slot1_card_id = Slot1_GetCardId();
	
	// ARM7 boot info copy of the header
	tNDSHeader* arm7_header = (tNDSHeader*)0x023FE940;
	memcpy(arm7_header, slot1_header, sizeof(tNDSHeader));
	
	// Firmware places these here for the SDK to use
	*(vu32*)0x027FF800 = slot1_card_id; // Card ID via 0x90
	*(vu32*)0x027FF804 = slot1_card_id; // Card ID via 0x10
	*(vu32*)0x027FFC00 = slot1_card_id; // Card ID via 0xB8
	*(vu16*)0x027FF808 = slot1_header->headerCRC16;
	*(vu16*)0x027FFC10 = 0; // Is debugger attached
	*(vu16*)0x027FFC40 = 1; // Boot source (0 = invalid, 1 = Slot-1, 2 = Download Play, 3 = system NAND)
	
	// The game does this manually before rebooting into eoo.dat
	// There are checks for this within the SRL
	tNDSHeader* arg_header = (tNDSHeader*)0x027FF000;
	memcpy(arg_header, slot1_header, sizeof(tNDSHeader));
	*(vu32*)&arg_header->gameCode[0] = GAME_CODE_D_JA;
	*(vu16*)&arg_header->makercode[0] = MAKER_CODE_NINTENDO;
}


void ITCM_CODE doBoot(const tNDSHeader* boot_header, const void* boot_arm9, const void* boot_arm7, void* rsa_verify_addr) {
	// Copy the boot header into the reset region
	// We will also use this to signal the ARM7 to boot
	tNDSHeader* reset_header = (tNDSHeader*)0x027FFE00;
	memcpy(reset_header, boot_header, sizeof(tNDSHeader));
	
	// Sync with ARM7
	// Apparently this read must access `boot_header`, not `reset_header`. Caching issue?
	void* arm7_entry = boot_header->arm7executeAddress;
	reset_header->arm7executeAddress = NULL;
	fifoSendAddress(FIFO_USER_01, &reset_header->arm7executeAddress);
	swiWaitForVBlank();
	
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = 0xFFFFFFFF;
	
	if (isDSiMode()) {
		DSMode9_Enable();
	}
	
	// Load the binaries (ITCM memcpy)
	Bin_Memcpy32(reset_header->arm9destination, boot_arm9, reset_header->arm9binarySize);
	Bin_Memcpy32(reset_header->arm7destination, boot_arm7, reset_header->arm7binarySize);
	
	// Patch out RSA verification
	((vu32*)rsa_verify_addr)[0] = 0xE3A00001; // mov r0, #1
	((vu32*)rsa_verify_addr)[1] = 0xE12FFF1E; // bx lr
	
	// Write entry back so ARM7 can see it
	reset_header->arm7executeAddress = arm7_entry;
	
	VCount_SpinWait();
	((void (*)(void))reset_header->arm9executeAddress)();
	// <unreachable>
}


int main(int argc, char* argv[]) {
	// Display top screen graphic
	videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    int bg = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    memcpy(bgGetGfxPtr(bg), top_screen_imgBitmap, top_screen_imgBitmapLen);
    bgShow(bg);
	
	// Set up bottom screen console for error reporting
	videoSetModeSub(MODE_0_2D);
	vramSetBankC(VRAM_C_SUB_BG);
	PrintConsole bottom_screen;
	consoleInit(&bottom_screen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
	consoleSelect(&bottom_screen);
	
	// Wait for hardware infomation from the ARM7
	HardwareMode cur_hardware;
	while (TRUE) {
		if (fifoCheckValue32(FIFO_USER_01)) {
			cur_hardware = (HardwareMode)fifoGetValue32(FIFO_USER_01);
			break;
		}
		swiWaitForVBlank();
	}
	
	// Ready for main loop
	bool card_detected = FALSE;
	while (TRUE) {
		swiWaitForVBlank();
		
		// If there was a card and now it's ejected, clear the console early
		if (card_detected && Slot1_CheckPullout()) {
			card_detected = FALSE;
			consoleClear();
		}
		
		// Wait for start to do anything else
		scanKeys();
		if (!(keysDown() & KEY_START)) {
			continue;
		}
		
		// User pressed start, so try to boot
		consoleClear();
		
		// Try to initialize Slot-1
		bool slot1_ok = Slot1_InitCard();
		if (!slot1_ok) {
			printf("Failed to read Slot-1 Card!    \n");
			switch (cur_hardware) {
				case DS_MODE_ON_DSI:
					printf("Running in DS mode on DSi/3DS. \n"
					       "This configuration will not    \n"
					       "work if the card is removed.   \n"
					       "Use HaxxStation with the card  \n"
					       "inserted in advance.           \n");
					break;
				
				case DS_MODE_ON_DS:
				case DSI_MODE_ON_DSI:
					printf("Re-insert it and try again.    \n");
					break;
			}
			
			continue;
		}
		
		// We were able to talk to something
		card_detected = TRUE;
		
		// Check the game we found in Slot-1 and load parameters for it
		u32 game_code = Slot1_GetGameCode();
		const GameBootParam* boot_param = GameBootParam_Get(game_code);
		if (boot_param == NULL)  {
			printf("Slot-1 Card is not Pokemon     \n"
			       "Diamond, Pearl, or Platinum    \n");
			continue;
		}
		
		// Set up eoo.dat into RAM
		bool eoo_ok = Eoo_Init(boot_param->eoo_offset);
		if (!eoo_ok) {
			printf("Failed to start connection to  \n"
			       "Ranch! This should not happen! \n");
			continue;
		}
		
		// Must be OK if we got here
		
		// Prepare to boot, setting up environment like the original game does
		prepareBoot();
		
		// Ready to boot
		doBoot(
			Eoo_GetHeader(),
			Eoo_GetArm9(),
			Eoo_GetArm7(),
			boot_param->eoo_rsa_verify
		);
	}
	
	// <unreachable>
	return 0;
}
