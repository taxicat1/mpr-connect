#ifndef BIN_UTIL_H
#define BIN_UTIL_H

#include <nds.h>

extern void Bin_Memcpy32(void* dst, const void* src, u32 len);
extern void Bin_PatchRSACheck(void* arm9_start, u32 arm9_len);
extern void Bin_ClearCache(void);

#endif
