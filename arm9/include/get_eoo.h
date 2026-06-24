#ifndef GET_EOO_H
#define GET_EOO_H

#include <nds.h>

bool Eoo_Init(void);
const tNDSHeader* Eoo_GetHeader(void);
const void* Eoo_GetArm9(void);
const void* Eoo_GetArm7(void);

#endif
