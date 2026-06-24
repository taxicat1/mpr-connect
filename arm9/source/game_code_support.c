#include <nds.h>

#include "pkmn_game_codes.h"
#include "game_code_support.h"


bool GameCode_IsSupported(u32 game_code) {
	static const u32 supported_game_codes[] = {
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
	
	for (int i = 0; supported_game_codes[i] != GAME_CODE_OTHER; i++) {
		if (supported_game_codes[i] == game_code) {
			return TRUE;
		}
	}
	
	return FALSE;
}
