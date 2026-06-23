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


# Source code paths
# -----------------

# List of folders to combine into the root of NitroFS:
#NITROFSDIR := nitrofs

# Tools
# -----

MAKE  := make
RM    := rm -rf

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

# Build artfacts
# --------------

ROM := $(NAME).nds

# Targets
# -------

.PHONY: all clean arm9 arm7

all: $(ROM)

clean:
	@echo "  CLEAN"
	$(V)$(MAKE) -f Makefile.arm9 clean --no-print-directory
	$(V)$(MAKE) -f Makefile.arm7 clean --no-print-directory
	$(V)$(RM) $(ROM) build compile_commands.json

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

ifneq ($(strip $(NITROFSDIR)),)
# Additional arguments for ndstool
NDSTOOL_ARGS  := -d $(NITROFSDIR)

# Make the NDS ROM depend on the filesystem only if it is needed
$(ROM): $(NITROFSDIR)
endif

# Combine the title strings
ifeq ($(strip $(GAME_SUBTITLE)),)
  GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_AUTHOR)
else
  GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_SUBTITLE);$(GAME_AUTHOR)
endif

$(ROM): arm9 arm7
	@echo "  NDSTOOL $@"
	$(V)$(BLOCKSDS)/tools/ndstool/ndstool -c $@ \
		-7 build/arm7.elf -9 build/arm9.elf \
		-b $(GAME_ICON) "$(GAME_FULL_TITLE)" \
		$(NDSTOOL_ARGS)
