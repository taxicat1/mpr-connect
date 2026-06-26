#include <nds.h>

#include "ds_mode_7.h"
#include "hardware_mode.h"
#include "vcount_spinwait.h"


static HardwareMode detectHardware(void) {
	if (isDSiMode()) {
		return HW_DSI_MODE_ON_DSI;
	} else if ((REG_GPIO_WIFI & GPIO_WIFI_MODE_MASK) == GPIO_WIFI_MODE_NTR) {
		return HW_DS_MODE_ON_DSI;
	} else {
		return HW_DS_MODE_ON_DS;
	}
}


static void doBoot(void* volatile* entry_ptr) {
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = 0xFFFFFFFF;
	
	if (isDSiMode()) {
		DSMode7_Enable();
	}
	
	// CRITICALLY IMPORTANT: Both ARM9 and ARM7 need a valid ROM control register
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
	
	// Send hardware information to the ARM9
	HardwareMode cur_hardware = detectHardware();
	fifoSendValue32(FIFO_USER_01, (u32)cur_hardware);
	
	while (TRUE) {
		if (fifoCheckAddress(FIFO_USER_01)) {
			void* volatile* entry_ptr = (void* volatile*)fifoGetAddress(FIFO_USER_01);
			doBoot(entry_ptr);
			
			// <unreachable>
			break;
		}
		
		inputGetAndSend();
		swiWaitForVBlank();
	}
	
	return 0;
}
