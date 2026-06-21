#include <nds.h>

#include "ds_mode_7.h"
#include "vcount_spinwait.h"


static void doBoot(void* volatile* entry_ptr) {
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = 0xFFFFFFFF;
	
	if (isDSiMode()) {
		DSMode7_Enable();
	}
	
	// CRITICALLY IMPORTANT: Both ARM9 and ARM7 needs a valid ROM control register
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
			void* volatile* entry_ptr = (void* volatile*)fifoGetAddress(FIFO_USER_01);
			doBoot(entry_ptr);
		}
		
		inputGetAndSend();
		swiWaitForVBlank();
	}
	
	// <unreachable>
	return 0;
}
