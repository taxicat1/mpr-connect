#ifndef SLOT1_OP_H
#define SLOT1_OP_H

#include <nds.h>

int Slot1_InitCard(void);
const tNDSHeader* Slot1_GetHeader(void);
u32 Slot1_GetGameCode(void);
u32 Slot1_GetCardId(void);
void Slot1_ReadRom(void* dst, u32 src_addr, u32 len);
int Slot1_CheckPullout(void);

#endif
