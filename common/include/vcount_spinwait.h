#ifndef VCOUNT_SPINWAIT_H
#define VCOUNT_SPINWAIT_H

#include <nds.h>

static inline void VCount_SpinWait(void) {
	while (REG_VCOUNT != 191) {
		continue;
	}
	
	while (REG_VCOUNT == 191) {
		continue;
	}
}

#endif
