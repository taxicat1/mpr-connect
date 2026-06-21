#include <nds.h>
#include <nds/ndstypes.h>

#include "ds_mode_9.h"


void DSMode9_Enable(void) {
	// DS mode main clock
	setCpuClock(FALSE);
	
	// DS mode SCFG
	// This freezes the system. Seems to work without it?
	//REG_SCFG_EXT = 0x83000000;
	//REG_SCFG_EXT &= 0x7FFFFFFF;
}
