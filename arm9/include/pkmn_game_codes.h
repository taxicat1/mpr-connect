#ifndef POKEMON_GAME_CODES_H
#define POKEMON_GAME_CODES_H

#define GAME_CODE(code)   ((code[0]) | (code[1] << 8) | (code[2] << 16) | (code[3] << 24))
#define MAKER_CODE(code)  ((code[0]) | (code[1] << 8))

#define GAME_CODE_D_EN   GAME_CODE("ADAE")
#define GAME_CODE_D_JA   GAME_CODE("ADAJ")
#define GAME_CODE_D_DE   GAME_CODE("ADAD")
#define GAME_CODE_D_ES   GAME_CODE("ADAS")
#define GAME_CODE_D_FR   GAME_CODE("ADAF")
#define GAME_CODE_D_IT   GAME_CODE("ADAI")
#define GAME_CODE_D_KR   GAME_CODE("ADAK")
#define GAME_CODE_P_EN   GAME_CODE("APAE")
#define GAME_CODE_P_JA   GAME_CODE("APAJ")
#define GAME_CODE_P_DE   GAME_CODE("APAJ")
#define GAME_CODE_P_ES   GAME_CODE("APAS")
#define GAME_CODE_P_FR   GAME_CODE("APAF")
#define GAME_CODE_P_IT   GAME_CODE("APAI")
#define GAME_CODE_P_KR   GAME_CODE("APAK")
#define GAME_CODE_PT_EN  GAME_CODE("CPUE")
#define GAME_CODE_PT_JA  GAME_CODE("CPUJ")
#define GAME_CODE_PT_DE  GAME_CODE("CPUD")
#define GAME_CODE_PT_ES  GAME_CODE("CPUS")
#define GAME_CODE_PT_FR  GAME_CODE("CPUF")
#define GAME_CODE_PT_IT  GAME_CODE("CPUI")
#define GAME_CODE_PT_KR  GAME_CODE("CPUK")

#define GAME_CODE_OTHER  GAME_CODE("####")

#define MAKER_CODE_NINTENDO  MAKER_CODE("01")

#endif
