BLOCKSDS             ?= /opt/blocksds/core
BLOCKSDSEXT          ?= /opt/blocksds/external
WONDERFUL_TOOLCHAIN  ?= /opt/wonderful

# User config
# ===========

NAME  := mpr-connect

GAME_TITLE      := My Pokémon Ranch
GAME_SUBTITLE   := Connect to Wii
GAME_AUTHOR     := Mow
GAME_ICON       := icon.png

# A compile_commands.json file is created if this is set to 1
COMPDB ?= 0

# Tools
# -----

MAKE      := make
RM        := rm -rf
NDSTOOL   := $(BLOCKSDS)/tools/ndstool/ndstool

ifeq ($(OS),Windows_NT)
MAKE_CIA  := make_cia/windows/make_cia.exe
else
MAKE_CIA  := make_cia/linux/make_cia
endif

# Verbose flag
# ------------

ifeq ($(VERBOSE),1)
V  :=
else
V  := @
endif

# Directories
# -----------

ARM9DIR  := arm9
ARM7DIR  := arm7

# Build artifacts
# ---------------

ROM  := $(NAME).nds
DSI  := $(NAME).dsi
CIA  := $(NAME).cia

# Targets
# -------

.PHONY: all clean arm9 arm7

all: $(ROM) $(CIA)

clean:
	@echo "  CLEAN"
	$(V)$(MAKE) -f Makefile.arm9 clean --no-print-directory
	$(V)$(MAKE) -f Makefile.arm7 clean --no-print-directory
	$(V)$(RM) $(ROM) $(CIA) build compile_commands.json

arm9:
	$(V)+$(MAKE) -f Makefile.arm9 COMPDB=$(COMPDB) --no-print-directory

arm7:
	$(V)+$(MAKE) -f Makefile.arm7 COMPDB=$(COMPDB) --no-print-directory

ifeq ($(COMPDB),1)
# Add an additional dependency to the "all" rule
all: compile_commands.json

compile_commands.json: arm9 arm7
	@echo "  MERGE   compile_commands.json"
	$(V)$(WONDERFUL_TOOLCHAIN)/bin/wf-compile-commands-merge $@ \
		build/*/compile_commands.json
endif

# Combine the title strings
ifeq ($(strip $(GAME_SUBTITLE)),)
  GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_AUTHOR)
else
  GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_SUBTITLE);$(GAME_AUTHOR)
endif

$(ROM): arm9 arm7
	@echo "  NDSTOOL $@"
	$(V)$(NDSTOOL) -c $@ \
		-7 build/arm7.elf -9 build/arm9.elf \
		-b $(GAME_ICON) "$(GAME_FULL_TITLE)"

# CIA stuff
# ---------

$(DSI): arm9 arm7
	@echo "  NDSTOOL $@"
	$(V)$(NDSTOOL) -c $@ \
		-7 build/arm7.elf -9 build/arm9.elf \
		-b $(GAME_ICON) "$(GAME_FULL_TITLE)" \
		-g HPRA 00 "MPR CONNECT" -z 80040000 -a 00018120

$(CIA): $(DSI)
	@echo "  MAKECIA $@"
	$(V)$(MAKE_CIA) --srl=$<
