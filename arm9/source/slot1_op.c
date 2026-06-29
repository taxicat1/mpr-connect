#include <nds.h>

#include "slot1_op.h"

// This was horribly confusing
#define CARD_DATA_LEN_MASK  (CARD_BLK_SIZE(7))
#define CARD_DATA_LEN_4     (CARD_BLK_SIZE(7))
#define CARD_DATA_LEN_200   (CARD_BLK_SIZE(1))
#define CARD_DATA_LEN_400   (CARD_BLK_SIZE(2))
#define CARD_DATA_LEN_800   (CARD_BLK_SIZE(3))
#define CARD_DATA_LEN_1000  (CARD_BLK_SIZE(4))
#define CARD_DATA_LEN_2000  (CARD_BLK_SIZE(5))
#define CARD_DATA_LEN_4000  (CARD_BLK_SIZE(6))

#define CARD_DELAY1_MASK    (CARD_DELAY1(0x1FFF))

typedef union {
	u8   bytes[8];  // In reverse order for Blowfish encryption
	u32  words[2];
} CardCommand;

typedef struct {
	u16  iii;    //<! Random, same for future commands
	u16  jjj;    //<! Random, same for future commands
	u32  kkkkk;  //<! Counter, counts up for each command after 3C, does not count up for resends
	u16  llll;   //<! Random, same for future commands
	u16  mmm;    //<! Random, same for future commands, key2 seed
	u16  nnn;    //<! Random, same for future commands, key2 seed
} KeyParam;

typedef struct {
	u32  P[18];
	u32  S[4][256];
} BlowfishCtx;

#include "blowfish_init.dat"

typedef struct {
	KeyParam     param;
	BlowfishCtx  bf;
} Key1Ctx;

typedef struct {
	bool        inited;
	u32         card_id;
	tNDSHeader  rom_header;
} CardSlot1Data;

static CardSlot1Data sCardCtx = { 0 };

static void Blowfish_Init(BlowfishCtx* ctx, u32 game_code, int level);
static void Blowfish_Encrypt(const BlowfishCtx* ctx, u32* datap);
static void Blowfish_EncryptCommand(const BlowfishCtx* ctx, CardCommand* cmd);

static void KeyParam_Init(KeyParam* param);
static void KeyParam_IncrementCounter(KeyParam* param);

static void Key1_Init(Key1Ctx* key, u32 game_code);
static void Key1_MakeCmdActivateKey1(Key1Ctx* key, CardCommand* dst);
static void Key1_MakeCmdActivateKey2(Key1Ctx* key, CardCommand* dst);
static void Key1_MakeCmdEnterMainDataMode(Key1Ctx* key, CardCommand* dst);
static void Key1_ConfigureKey2Hardware(Key1Ctx* key, int device_type);
static void Key1_MakeCmdGetCardId10(Key1Ctx* key1, CardCommand* dst);


// Misc helpers

static u16 rand16(void) {
	// Unimportant
	static u32 state = 0xDEADBEEF;
	state = (state * 1223106847) + 1;
	return state >> 16;
}


static void sendCommand(const CardCommand* cmd, u32 opflags) {
	while (REG_ROMCTRL & CARD_BUSY) {
		continue;
	}
	
	REG_AUXSPICNT = CARD_ENABLE | CARD_IRQ;
	
	for (int i = 0; i < 8; i++) {
		REG_CARD_COMMAND[i] = cmd->bytes[7 - i];
	}
	
	REG_ROMCTRL = opflags | CARD_ACTIVATE | CARD_nRESET;
}


static void copyCommandReturn(void* dest, u32 max_len) {
	u32 data_copied = 0;
	do {
		if (REG_ROMCTRL & CARD_DATA_READY) {
			u32 data = REG_CARD_DATA_RD;
			if (dest != NULL && data_copied < max_len) {
				*(u32*)((u32)dest + data_copied) = data;
				data_copied += 4;
			}
		}
	} while (REG_ROMCTRL & CARD_BUSY);
}


static void ignoreCommandReturn(void) {
	copyCommandReturn(NULL, 0);
}



// Blowfish stuff

static u32 byteSwap(u32 x) {
	// Compiler does a surprisingly good job optimizing this
	u32 a = x & 0xFF000000;
	u32 b = x & 0x00FF0000;
	u32 c = x & 0x0000FF00;
	u32 d = x & 0x000000FF;
	return (a >> 24) | (b >> 8) | (c << 8) | (d << 24);
}


static void Blowfish_Init(BlowfishCtx* ctx, u32 game_code, int level) {
	memcpy(ctx, &sBlowfishInit, sizeof(BlowfishCtx));
	
	u32 keycode[3] = {
		game_code,
		game_code >> 1,
		game_code << 1
	};
	
	for (int l = 0; l < level; l++) {
		if (l == 3) {
			keycode[1] <<= 1;
			keycode[2] >>= 1;
		}
		
		Blowfish_Encrypt(ctx, &keycode[1]); // ( keycode[1], keycode[2] )
		Blowfish_Encrypt(ctx, &keycode[0]); // ( keycode[0], keycode[1] )
		
		for (int i = 0; i < 18; i++) {
			ctx->P[i] ^= byteSwap(keycode[i % 2]);
		}
		
		u32 tmp[2] = { 0, 0 };
		
		// Generate P-array
		for (int i = 0; i < 18; i += 2) {
			Blowfish_Encrypt(ctx, &tmp[0]);
			ctx->P[i]     = tmp[1];
			ctx->P[i + 1] = tmp[0];
		}
		
		// Generate S-boxes
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 256; j += 2) {
				Blowfish_Encrypt(ctx, &tmp[0]);
				ctx->S[i][j]     = tmp[1];
				ctx->S[i][j + 1] = tmp[0];
			}
		}
	}
}


static void Blowfish_Encrypt(const BlowfishCtx* ctx, u32* datap) {
	u32 x = datap[1];
	u32 y = datap[0];
	
	for (int i = 0; i < 16; i++) {
		x ^= ctx->P[i];
		
		u32 a = ctx->S[0][(u8)(x >> 24)];
		u32 b = ctx->S[1][(u8)(x >> 16)];
		u32 c = ctx->S[2][(u8)(x >> 8)];
		u32 d = ctx->S[3][(u8)(x)];
		y ^= ((a + b) ^ c) + d;
		
		u32 tmp = x;
		x = y;
		y = tmp;
	}
	
	datap[0] = x ^ ctx->P[16];
	datap[1] = y ^ ctx->P[17];
}


static void Blowfish_EncryptCommand(const BlowfishCtx* ctx, CardCommand* cmd) {
	Blowfish_Encrypt(ctx, &cmd->words[0]);
}



// Key1 stuff

static void KeyParam_Init(KeyParam* param) {
	param->iii   = rand16() & 0xFFF;
	param->jjj   = rand16() & 0xFFF;
	param->kkkkk = rand16() ^ (rand16() << 4);
	param->llll  = rand16();
	param->mmm   = rand16() & 0xFFF;
	param->nnn   = rand16() & 0xFFF;
}


static void KeyParam_IncrementCounter(KeyParam* param) {
	param->kkkkk = (param->kkkkk + 1) & 0xFFFFF;
}


static void Key1_Init(Key1Ctx* key1, u32 game_code) {
	KeyParam_Init(&key1->param);
	Blowfish_Init(&key1->bf, game_code, 2);
}


static void Key1_MakeCmdActivateKey1(Key1Ctx* key1, CardCommand* dst) {
	// 3C II IJ JJ XK KK KK XX
	u32 x_1 = 0x0;
	u32 x_2 = 0x00;
	
	dst->bytes[7] = CARD_CMD_ACTIVATE_BF;
	dst->bytes[6] = key1->param.iii >> 4;
	dst->bytes[5] = (key1->param.iii << 4) | (key1->param.jjj >> 8);
	dst->bytes[4] = key1->param.jjj;
	dst->bytes[3] = (x_1 << 4) | (key1->param.kkkkk >> 16);
	dst->bytes[2] = key1->param.kkkkk >> 8;
	dst->bytes[1] = key1->param.kkkkk;
	dst->bytes[0] = x_2;
}


static void Key1_MakeCmdActivateKey2(Key1Ctx* key1, CardCommand* dst) {
	// 4L LL LM MM NN NK KK KK
	dst->bytes[7] = CARD_CMD_ACTIVATE_SEC | (key1->param.llll >> 12);
	dst->bytes[6] = key1->param.llll >> 4;
	dst->bytes[5] = (key1->param.llll << 4) | (key1->param.mmm >> 8);
	dst->bytes[4] = key1->param.mmm;
	dst->bytes[3] = key1->param.nnn >> 4;
	dst->bytes[2] = (key1->param.nnn << 4) | (key1->param.kkkkk >> 16);
	dst->bytes[1] = key1->param.kkkkk >> 8;
	dst->bytes[0] = key1->param.kkkkk;
	
	Blowfish_EncryptCommand(&key1->bf, dst);
	KeyParam_IncrementCounter(&key1->param);
}


static void Key1_MakeCmdEnterMainDataMode(Key1Ctx* key1, CardCommand* dst) {
	// AL LL LI II JJ JK KK KK
	dst->bytes[7] = CARD_CMD_DATA_MODE | (key1->param.llll >> 12);
	dst->bytes[6] = key1->param.llll >> 4;
	dst->bytes[5] = (key1->param.llll << 4) | (key1->param.iii >> 8);
	dst->bytes[4] = key1->param.iii;
	dst->bytes[3] = key1->param.jjj >> 4;
	dst->bytes[2] = (key1->param.jjj << 4) | (key1->param.kkkkk >> 16);
	dst->bytes[1] = key1->param.kkkkk >> 8;
	dst->bytes[0] = key1->param.kkkkk;
	
	Blowfish_EncryptCommand(&key1->bf, dst);
	KeyParam_IncrementCounter(&key1->param);
}


static void Key1_ConfigureKey2Hardware(Key1Ctx* key1, int device_type) {
	static const u8 card_seed_bytes[] = { 0xE8, 0x4D, 0x5A, 0xB1, 0x17, 0x8F, 0x99, 0xD5 };
	
	REG_ROMCTRL = 0;
	
	REG_CARD_1B0 = 0x00006000 | (key1->param.mmm << 27) | (key1->param.nnn << 15) | card_seed_bytes[device_type % 8];
	REG_CARD_1B4 = 0x879B9B05;
	REG_CARD_1B8 = key1->param.mmm >> 5;
	REG_CARD_1BA = 0x005C;
	
	REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED | CARD_SEC_EN | CARD_SEC_DAT;
}


static void Key1_MakeCmdGetCardId10(Key1Ctx* key1, CardCommand* dst) {
	// 1L LL LI II JJ JK KK KK
	dst->bytes[7] = CARD_CMD_SECURE_CHIPID | (key1->param.llll >> 12);
	dst->bytes[6] = key1->param.llll >> 4;
	dst->bytes[5] = (key1->param.llll << 4) | (key1->param.iii >> 8);
	dst->bytes[4] = key1->param.iii;
	dst->bytes[3] = key1->param.jjj >> 4;
	dst->bytes[2] = (key1->param.jjj << 4) | (key1->param.kkkkk >> 16);
	dst->bytes[1] = key1->param.kkkkk >> 8;
	dst->bytes[0] = key1->param.kkkkk;
	
	Blowfish_EncryptCommand(&key1->bf, dst);
	KeyParam_IncrementCounter(&key1->param);
}



// Slot1 stuff

static bool cardIdValid(u32 card_id) {
	return (card_id != 0) && ((card_id >> 16) != 0xFFFF);
}


static bool romHeaderValid(tNDSHeader* rom_header) {
	return swiCRC16(0xFFFF, rom_header, sizeof(tNDSHeader) - sizeof(u16)) == rom_header->headerCRC16;
}


static bool slot1Disabled(void) {
	return (REG_SCFG_MC & SCFG_MC_PWR_MASK) != SCFG_MC_PWR_ON;
}


static void cardDelay(void) {
	// This is witchcraft
	u16 timeout = sCardCtx.rom_header.readTimeout;
	
	TIMER_DATA(0) = 0 - (((timeout & 0x3FFF) + 3));
	TIMER_CR(0)   = TIMER_DIV_256 | TIMER_ENABLE;
	
	while (TIMER_DATA(0) != 0xFFFF) {
		continue;
	}
	
	TIMER_CR(0)   = 0;
	TIMER_DATA(0) = 0;
}


static void resetCard(void) {
	// Make sure Slot-1 is on
	if (isDSiMode() && slot1Disabled()) {
		enableSlot1();
		for (int i = 0; i < 15; i++) {
			swiWaitForVBlank();
		}
	}
	
	// Reset the card
	REG_ROMCTRL = 0;
	REG_AUXSPICNT = 0;
	
	for (int i = 0; i < 25; i++) {
		swiWaitForVBlank();
	}
	
	REG_AUXSPICNT = CARD_CR1_ENABLE | CARD_CR1_IRQ;
	REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED;
	
	// Dummy command after reset
	CardCommand cmd = { 0 };
	cmd.bytes[7] = CARD_CMD_DUMMY;
	u32 opflags = CARD_DATA_LEN_200 | CARD_CLK_SLOW | CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F);
	sendCommand(&cmd, opflags);
	ignoreCommandReturn();
}


static u32 getCardId90(void) {
	CardCommand cmd = { 0 };
	cmd.bytes[7] = CARD_CMD_HEADER_CHIPID;
	u32 opflags = CARD_DATA_LEN_4 | CARD_CLK_SLOW; 
	
	u32 card_id_90 = 0xFFFF0090; // Dummy
	sendCommand(&cmd, opflags);
	copyCommandReturn(&card_id_90, 4);
	return card_id_90;
}


static u32 getCardIdB8(void) {
	CardCommand cmd = { 0 };
	cmd.bytes[7] = CARD_CMD_DATA_CHIPID;
	u32 ctrl_13 = sCardCtx.rom_header.cardControl13;
	u32 opflags = (ctrl_13 & ~CARD_DELAY1_MASK) | CARD_DATA_LEN_4;
	
	u32 card_id_b8 = 0xFFFF00B8; // Dummy
	sendCommand(&cmd, opflags);
	copyCommandReturn(&card_id_b8, 4);
	return card_id_b8;
}


static void getCardHeader(void* dst) {
	CardCommand cmd = { 0 };
	cmd.bytes[7] = CARD_CMD_HEADER_READ;
	u32 opflags = CARD_DATA_LEN_200 | CARD_CLK_SLOW | CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F);
	sendCommand(&cmd, opflags);
	copyCommandReturn(dst, sizeof(tNDSHeader));
}


bool Slot1_InitCard(void) {
	sysSetCardOwner(BUS_OWNER_ARM9);
	
	// Check for pullout, which will remove the inited flag if it detects such
	Slot1_CheckPullout();
	if (sCardCtx.inited) {
		return true;
	}
	
	// Check if we are running from Download Play and the card is already inited
	if (*(vu16*)0x027FFC40 == 2) {
		tNDSHeader* rom_header = (tNDSHeader*)0x023FE940;
		if (romHeaderValid(rom_header)) {
			// Need to get the header first for the card command flags
			memcpy(&sCardCtx.rom_header, rom_header, sizeof(tNDSHeader));
			
			u32 card_id = getCardIdB8();
			if (cardIdValid(card_id) && card_id == getCardIdB8()) {
				// Seems good
				sCardCtx.card_id = card_id;
				sCardCtx.inited = true;
				return true;
			}
		}
	}
	
	// Card reset
	resetCard();
	
	// Get card ID (0x90)
	u32 card_id_90 = getCardId90();
	if (!cardIdValid(card_id_90)) {
		return false;
	}
	
	// Read ROM header
	getCardHeader(&sCardCtx.rom_header);
	if (!romHeaderValid(&sCardCtx.rom_header)) {
		return false;
	}
	
	bool normal_chip = card_id_90 & 0x80000000;
	
	// Key1 + Blowfish setup (not needed after we enter main data mode)
	Key1Ctx key1;
	u32 game_code = *(u32*)(&sCardCtx.rom_header.gameCode[0]);
	Key1_Init(&key1, game_code);
	
	// Set up flags for key1 communication
	u32 ctrl_13 = sCardCtx.rom_header.cardControl13;
	u32 ctrl_bf = sCardCtx.rom_header.cardControlBF;
	
	u32 flags_key1 = (ctrl_13 & (CARD_WR | CARD_CLK_SLOW)) |
	                 ((ctrl_bf & (CARD_CLK_SLOW | CARD_DELAY1(0x1FFF))) + ((ctrl_bf & CARD_DELAY2(0x3F)) >> 16));
	
	if (!normal_chip) {
		flags_key1 |= CARD_SEC_LARGE;
	}
	
	CardCommand cmd;
	
	// Activate key1 mode (Blowfish) (unencrypted command)
	Key1_MakeCmdActivateKey1(&key1, &cmd);
	u32 opflags = ctrl_13 & (CARD_WR | CARD_CLK_SLOW);
	sendCommand(&cmd, opflags);
	ignoreCommandReturn();
	
	// Activate key2 mode (hardware)
	Key1_MakeCmdActivateKey2(&key1, &cmd);
	if (normal_chip) {
		sendCommand(&cmd, flags_key1);
		ignoreCommandReturn();
		cardDelay();
	}
	sendCommand(&cmd, flags_key1);
	ignoreCommandReturn();
	
	// Configure key2 hardware registers
	Key1_ConfigureKey2Hardware(&key1, sCardCtx.rom_header.deviceType);
	u32 flags_key2 = flags_key1 | CARD_SEC_EN | CARD_SEC_DAT;
	
	// Get card ID (0x10)
	Key1_MakeCmdGetCardId10(&key1, &cmd);
	if (normal_chip) {
		sendCommand(&cmd, flags_key2);
		ignoreCommandReturn();
		cardDelay();
	}
	opflags = flags_key2 | CARD_DATA_LEN_4;
	sendCommand(&cmd, opflags);
	u32 card_id_10 = 0xFFFF0010; // Dummy
	copyCommandReturn(&card_id_10, 4);
	
	// Check card IDs to make sure key1 is working
	if (card_id_10 != card_id_90) {
		return false;
	}
	
	// Enter main data mode for B7/B8
	Key1_MakeCmdEnterMainDataMode(&key1, &cmd);
	if (normal_chip) {
		sendCommand(&cmd, flags_key2);
		ignoreCommandReturn();
	}
	cardDelay();
	sendCommand(&cmd, flags_key2);
	ignoreCommandReturn();
	
	// Get card ID (0xB8)
	// Success here means B7 commands will also work
	u32 card_id_b8 = getCardIdB8();
	if (card_id_b8 != card_id_10) {
		return false;
	}
	
	sCardCtx.card_id = card_id_b8;
	sCardCtx.inited = true;
	
	return true;
}


const tNDSHeader* Slot1_GetHeader(void) {
	if (!sCardCtx.inited) {
		return NULL;
	}
	
	return &sCardCtx.rom_header;
}


u32 Slot1_GetGameCode(void) {
	if (!sCardCtx.inited) {
		return 0xFFFFFFFF;
	}
	
	return *(u32*)&sCardCtx.rom_header.gameCode[0];
}


u32 Slot1_GetCardId(void) {
	if (!sCardCtx.inited) {
		return 0xFFFFFFFF;
	}
	
	return sCardCtx.card_id;
}


void Slot1_ReadRom(void* dst, u32 src_addr, u32 len) {
	if (!sCardCtx.inited) {
		return;
	}
	
	u32 skip_bytes = src_addr & 0x1FF;
	src_addr &= ~0x1FF;
	
	u8 page_buffer[0x200] __attribute__((aligned(4)));
	
	CardCommand cmd = { 0 };
	cmd.bytes[7] = CARD_CMD_DATA_READ;
	
	u32 ctrl_13 = sCardCtx.rom_header.cardControl13;
	u32 opflags = (ctrl_13 & ~CARD_DATA_LEN_MASK) | CARD_DATA_LEN_200;
	
	while (len != 0) {
		cmd.bytes[6] = src_addr >> 24;
		cmd.bytes[5] = src_addr >> 16;
		cmd.bytes[4] = src_addr >> 8;
		cmd.bytes[3] = src_addr;
		
		sendCommand(&cmd, opflags);
		copyCommandReturn(&page_buffer[0], 0x200);
		
		int copy_start = 0;
		int copy_end = len;
		
		if (skip_bytes > 0) {
			copy_start += skip_bytes;
			copy_end += skip_bytes;
			skip_bytes = 0;
		}
		
		if (copy_end > 0x200) {
			copy_end = 0x200;
		}
		
		int copy_len = copy_end - copy_start;
		
		memcpy(dst, &page_buffer[copy_start], copy_len);
		
		dst = (void*)((u32)dst + copy_len);
		len -= copy_len;
		src_addr += 0x200;
	}
}


bool Slot1_CheckPullout(void) {
	if (!sCardCtx.inited) {
		return true;
	}
	
	u32 card_id_b8 = getCardIdB8();
	if (card_id_b8 != sCardCtx.card_id) {
		sCardCtx.inited = false;
		return true;
	}
	
	return false;
}
