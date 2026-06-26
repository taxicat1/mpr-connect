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

typedef struct {
	u32  fnt_contents_start_offset;
	u16  fat_contents_start_idx;
	union {
		u16  num_subfolders;     // Root folder only
		u16  parent_folder_idx;
	};
} FSFolderAlloc;

typedef struct {
	u32  rom_start_offset;
	u32  rom_end_offset;
} FSFATEntry;

typedef enum {
	FS_FILE,
	FS_FOLDER
} FSItemType;


static s16 FS_FindFolderItem(
	const void*           fnt_data,
	const FSFolderAlloc*  cur_folder,
	const char*           target_name,
	u32                   target_name_len,
	FSItemType            target_type
) {
	s16 folder_item_idx = cur_folder->fat_contents_start_idx;
	const char* folder_contents_list = (const char*)fnt_data + cur_folder->fnt_contents_start_offset;
	while (TRUE) {
		u8 item_metadata = *folder_contents_list++;
		if (item_metadata == 0) {
			// End of folder contents
			return -1;
		}
		
		FSItemType type = (item_metadata & 0x80) ? FS_FOLDER : FS_FILE;
		u32 name_len = item_metadata & ~0x80;
		
		// Check for match against target
		if (type == target_type && name_len == target_name_len && strncmp(folder_contents_list, target_name, name_len) == 0) {
			// Found target
			if (type == FS_FOLDER) {
				// Read two byte folder index data after name
				s16 folder_name_idx = folder_contents_list[name_len] | (folder_contents_list[name_len + 1] << 8);
				return folder_name_idx & ~0xF000;
			} else {
				// Use normal item index within folder
				return folder_item_idx;
			}
		}
		
		// Advance to next item
		folder_item_idx++;
		folder_contents_list += name_len;
		if (type == FS_FOLDER) {
			// Advance past two byte folder index data
			folder_contents_list += 2;
		}
	}
}


static u32 getEooRomOffset(void) {
	const tNDSHeader* slot1_header = Slot1_GetHeader();
	
	// Get file name table
	void* fnt_data = malloc(slot1_header->filenameSize);
	if (fnt_data == NULL) {
		return 0xFFFFFFFF;
	}
	Slot1_ReadRom(fnt_data, slot1_header->filenameOffset, slot1_header->filenameSize);
	
	// Find root -> data
	const FSFolderAlloc* root_folder = (const FSFolderAlloc*)fnt_data;
	s16 data_fnt_idx = FS_FindFolderItem(fnt_data, root_folder, "data", 4, FS_FOLDER);
	if (data_fnt_idx == -1) {
		free(fnt_data);
		return 0xFFFFFFFF;
	}
	
	// Find data -> eoo.dat
	const FSFolderAlloc* data_folder = (const FSFolderAlloc*)fnt_data + data_fnt_idx;
	s16 eoo_dat_fat_idx = FS_FindFolderItem(fnt_data, data_folder, "eoo.dat", 7, FS_FILE);
	if (eoo_dat_fat_idx == -1) {
		free(fnt_data);
		return 0xFFFFFFFF;
	}
	
	// Finally get location of eoo.dat from file allocation table
	FSFATEntry eoo_dat_fat;
	Slot1_ReadRom(&eoo_dat_fat, slot1_header->fatOffset + (eoo_dat_fat_idx * sizeof(FSFATEntry)), sizeof(FSFATEntry));
	
	free(fnt_data);
	return eoo_dat_fat.rom_start_offset;
}


static u32 fourAlignUp(u32 x) {
	return (x + 3) & ~3;
}


bool Eoo_Init(void) {
	u32 eoo_rom_offset = getEooRomOffset();
	if (eoo_rom_offset == 0xFFFFFFFF) {
		return FALSE;
	}
	
	sEoo.header = malloc(sizeof(tNDSHeader));
	if (sEoo.header == NULL) {
		return FALSE;
	}
	Slot1_ReadRom(sEoo.header, eoo_rom_offset, sizeof(tNDSHeader));
	
	// Sanity check to make sure we really got the header
	if (swiCRC16(0xFFFF, sEoo.header, sizeof(tNDSHeader) - sizeof(u16)) != sEoo.header->headerCRC16) {
		free(sEoo.header);
		sEoo.header = NULL;
		return FALSE;
	}
	
	// Arrange memory so there is a contiguous [header]..[ARM9]..[ARM7].. for copying later
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
	
	sEoo.header = (tNDSHeader*)boot_region_base;
	sEoo.arm9 = (void*)((u32)boot_region_base + header_space);
	sEoo.arm7 = (void*)((u32)boot_region_base + header_space + arm9_space);
	
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
