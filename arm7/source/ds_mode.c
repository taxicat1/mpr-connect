#include <nds/arm7/codec.h>
#include <nds/arm7/audio.h>
#include <nds/ipc.h>
#include <nds/arm7/i2c.h>
#include <string.h>
#include <nds/timers.h>
#include <nds/dma.h>

#include "ds_mode.h"


// I did not write this. I do not know how this works.
void DSMode_TouchAndSoundEnable(void) {	
	REG_SOUNDCNT = 0;
	REG_SNDCAP0CNT = 0;
	REG_SNDCAP1CNT = 0;
	
	REG_SCFG_CLK = 0x0181;
	REG_SCFG_ROM = 0x0703;
	
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
	
	*(vu16*)0x04000500 = 0x807F;
	*(vu16*)0x04004C04 |= 0x0080;
	
	REG_SCFG_EXT = 0x93FBFB06;
	REG_SCFG_EXT &= 0x7FFFFFFF;
}
