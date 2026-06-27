#include <nds.h>

#include "game_code_support.h"
#include "pkmn_game_codes.h"

static const u32 sSupportedGameCodes[] = {
	GAME_CODE_D_EN,
	GAME_CODE_D_JA,
	GAME_CODE_D_DE,
	GAME_CODE_D_ES,
	GAME_CODE_D_FR,
	GAME_CODE_D_IT,
	
	GAME_CODE_P_EN,
	GAME_CODE_P_JA,
	GAME_CODE_P_DE,
	GAME_CODE_P_ES,
	GAME_CODE_P_FR,
	GAME_CODE_P_IT,
	
	GAME_CODE_PT_EN,
	GAME_CODE_PT_JA,
	GAME_CODE_PT_DE,
	GAME_CODE_PT_ES,
	GAME_CODE_PT_FR,
	GAME_CODE_PT_IT,
	
	GAME_CODE_OTHER  // EOL
};


int GameCode_IsSupported(u32 game_code) {
	for (int i = 0; sSupportedGameCodes[i] != GAME_CODE_OTHER; i++) {
		if (sSupportedGameCodes[i] == game_code) {
			return TRUE;
		}
	}
	
	return FALSE;
}
