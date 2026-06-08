#include <nds.h>

#include "get_eoo.h"
#include "slot1_op.h"

typedef struct {
	tNDSHeader*  header;
	void*        arm9;
	void*        arm7;
} EooData;

// What on EARTH does eoo mean???
static EooData sEoo = { NULL, NULL, NULL };


#if 0
static u32 getEooRomOffset(void) {
	// TODO read FNT/FAT?? A possibility
}
#endif


static inline u32 fourAlignUp(u32 x) {
	return (x + 3) & ~3;
}


bool Eoo_Init(u32 eoo_rom_offset) {
	sEoo.header = malloc(sizeof(tNDSHeader));
	if (sEoo.header == NULL) {
		return FALSE;
	}
	
	Slot1_ReadRom(sEoo.header, eoo_rom_offset, sizeof(tNDSHeader));
	
	// Sanity check to make sure we really got the header
	if (swiCRC16(0xFFFF, sEoo.header, sizeof(tNDSHeader)) != 0x0000) {
		free(sEoo.header);
		sEoo.header = NULL;
		return FALSE;
	}
	
	// Arrange memory so there is a contiguous [header]..[ARM9]..[ARM7] for copying later
	// I expect malloc() to do this anyway, but to be rigorous let's manage it
	u32 header_space = fourAlignUp(sizeof(tNDSHeader));
	u32 arm9_space   = fourAlignUp(sEoo.header->arm9binarySize);
	u32 arm7_space   = fourAlignUp(sEoo.header->arm7binarySize);
	u32 total_space  = header_space + arm9_space + arm7_space;
	
	void* boot_region_base = realloc(sEoo.header, total_space);
	if (boot_region_base == NULL) {
		free(sEoo.header);
		sEoo.header = NULL;
		return FALSE;
	}
	
	sEoo.header = (void*)((u32)boot_region_base);
	sEoo.arm9   = (void*)((u32)boot_region_base + header_space);
	sEoo.arm7   = (void*)((u32)boot_region_base + header_space + arm9_space);
	
	Slot1_ReadRom(sEoo.arm9, eoo_rom_offset + sEoo.header->arm9romOffset, sEoo.header->arm9binarySize);
	Slot1_ReadRom(sEoo.arm7, eoo_rom_offset + sEoo.header->arm7romOffset, sEoo.header->arm7binarySize);
	
	return TRUE;
}


const tNDSHeader* Eoo_GetHeader(void) {
	return sEoo.header;
}


const void* Eoo_GetArm9(void) {
	return sEoo.arm9;
}


const void* Eoo_GetArm7(void) {
	return sEoo.arm7;
}
