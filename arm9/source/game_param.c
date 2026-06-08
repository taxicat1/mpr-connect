#include <nds.h>

#include "game_param.h"

#include "pkmn_game_codes.h"

#include "game_param.dat"


const GameBootParam* GameBootParam_Get(u32 game_code) {
	for (int i = 0; sGameBootParams[i].game_code != GAME_CODE_OTHER; i++) {
		if (sGameBootParams[i].game_code == game_code) {
			return &sGameBootParams[i];
		}
	}
	
	return NULL;
}
