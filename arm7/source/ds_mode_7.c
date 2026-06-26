#include <nds.h>

#include "ds_mode_7.h"


void DSMode7_Enable(void) {
	// DSi mode interrupts off
	REG_AUXIE = 0;
	REG_AUXIF = 0xFFFFFFFF;
	
	// DS mode touch panel and sound codecs
	// I did not write this. I do not know how this works.
	REG_SOUNDCNT = 0;
	REG_SNDCAP0CNT = 0;
	REG_SNDCAP1CNT = 0;
	
	cdcWriteReg(CDC_SOUND, 0x26, 0xA7);
	cdcWriteReg(CDC_SOUND, 0x27, 0xA7);
	cdcWriteReg(CDC_SOUND, 0x2E, 0x03);
	cdcWriteReg(CDC_TOUCHCNT, 0x03, 0x00);
	cdcWriteReg(CDC_SOUND, 0x21, 0x20);
	cdcWriteReg(CDC_SOUND, 0x22, 0xF0);
	cdcWriteReg(CDC_SOUND, 0x22, 0x70);
	cdcWriteReg(CDC_CONTROL, 0x52, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	
	cdcReadReg(CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x98);
	cdcWriteReg(0xFF, 0x05, 0x00);
	
	writePowerManagement(PM_READ_REGISTER, 0x00);
	writePowerManagement(PM_CONTROL_REG, 0x0D);
	
	// DS mode WiFi
	gpioSetWifiMode(GPIO_WIFI_MODE_NTR);
	
	// DS mode new block clock
	REG_SCFG_CLK = 0x0181;
	
	// DS mode BIOS
	REG_SCFG_ROM = 0x0703;
	
	// DS mode SCFG
	REG_SCFG_EXT = 0x93FBFB06;
	REG_SCFG_EXT &= 0x7FFFFFFF;
}
