#include <nds.h>

#include "ds_mode.h"
#include "vcount_spinwait.h"


static void doBoot(void* volatile* entry_ptr) {
	if (isDSiMode()) {
		DSMode_TouchAndSoundEnable();
	}
	
	REG_IE = 0;
	REG_IF = 0xFFFFFFFF;
	REG_AUXIE = 0;
	REG_AUXIF = 0xFFFFFFFF;
	
	// CRITICALLY IMPORTANT: ARM7 needs a valid ROM control register
	// This register is not shared between ARM7/ARM9 like many of the others
	REG_ROMCTRL |= CARD_nRESET;
	
	void* entry;
	do {
		entry = *entry_ptr;
	} while (entry == NULL);
	
	VCount_SpinWait();
	((void (*)(void))entry)();
	// <unreachable>
}


int main(int argc, char* argv[]) {
	readUserSettings();
	ledBlink(LED_ALWAYS_ON);
	irqInit();
	fifoInit();
	installSystemFIFO();
	
	irqEnable(IRQ_VBLANK);
	
	while (TRUE) {
		if (fifoCheckAddress(FIFO_USER_01)) {
			doBoot((void* volatile*)fifoGetAddress(FIFO_USER_01));
		}
		inputGetAndSend();
		swiWaitForVBlank();
	}
	
	// <unreachable>
	return 0;
}
