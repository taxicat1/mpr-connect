#ifndef GAME_PARAM_H
#define GAME_PARAM_H

#include <nds.h>

typedef struct {
	u32    game_code;       //<! Game serial (ADAJ, CPUE, etc.)
	u32    eoo_offset;      //<! Offset within the ROM that eoo.dat starts (a NitroFS viewer can show this) TODO don't hardcode this?
	void*  eoo_rsa_verify;  //<! Location within eoo.dat of CRYPTO_VerifySignatureWithHash (RSA-1024 SHA-1 verification)
} GameBootParam;

const GameBootParam* GameBootParam_Get(u32 game_code);

#endif
