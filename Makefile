CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

# PSL1GHT tools
SFO := $(PSL1GHT)/host/bin/sfo.py
PKG := $(PSL1GHT)/host/bin/pkg.py
PKG_GEO := $(PSL1GHT)/host/bin/package_finalize
SELF_NPDRM := $(PSL1GHT)/host/bin/make_self_npdrm

PPU_SRCS = main.c dialog.c fileutils.c fnt35.c fnt_print.c graphics.c i18n.c network.c parse.c syscall8.c vpshader.vp vpshader2.vp fpshader.fp fpshader2.fp
PPU_EMBEDDED_SRCS = PNG/BGG.PNG PNG/BGH.PNG PNG/HIGHLIGHT.PNG

PPU_TARGET = open_manager.elf

GITHEAD := $(shell git describe --abbrev=4 --tags 2>/dev/null)
ifeq ($(strip $(GITHEAD)),)
GITHEAD := $(shell git rev-parse HEAD)
endif
ifneq ($(strip $(shell git diff-index -m --name-only HEAD)),)
GITHEAD := $(GITHEAD)-dirty
endif

PPU_LDLIBS = -lfont_stub -lfontFT_stub -lfreetype_stub -lpthread -latrac3plus_stub -lmixer -laudio_stub -lftp -lrtc_stub -lnet_stub -lnetctl_stub -lpngdec_stub -lm -ldbgfont_gcm -lgcm_cmd -lgcm_sys_stub -lio_stub -lsysmodule_stub -lsysutil_stub -lfs_stub -lhttp_util_stub -lhttp_stub
PPU_CPPFLAGS += -Werror -D'FOLDER_NAME="$(FOLDER_NAME)"'


PACKAGE_NAME = UP0001-GAIA01985_00-7679866932773369
FOLDER_NAME = $(shell echo $(PACKAGE_NAME) | sed -n 's/^[[:alnum:]]*-\([[:alnum:]]*\)_.*/\1/p')

WITH_GAMES_DIR ?= BDRIPS

PKG_TARGET = $(PACKAGE_NAME)-$(GITHEAD)-$(WITH_GAMES_DIR).pkg

CLEANFILES = PS3_GAME/USRDIR/EBOOT.BIN $(OBJS_DIR)/$(PPU_TARGET) readme.aux readme.log readme.out readme.tex
SUBDIRS = ext

PPU_CPPFLAGS += -D'GAMES_DIR="$(WITH_GAMES_DIR)"'

ifneq ($(strip $(WITH_CFW)),)
PPU_SRCS += hvcall.c mm.c payload.c

ifneq ($(strip $(WITH_PL3)),)
PPU_EMBEDDED_SRCS += payload/pl3/patch.txt payload/pl3/payload.bin
PPU_CPPFLAGS += -DWITH_PL3
else
PPU_EMBEDDED_SRCS += payload/syscall36/patch.txt payload/syscall36/payload.bin
endif

PPU_CPPFLAGS += -DWITH_CFW
endif

ifneq ($(strip $(WITH_COVERS_DIR)),)
PPU_CPPFLAGS += -D'COVERS_DIR="$(WITH_COVERS_DIR)"'
endif

ifneq ($(strip $(WITHOUT_SAVE_STATUS)),)
PPU_CPPFLAGS += -DWITHOUT_SAVE_STATUS
endif

ifeq ($(strip $(WITH_SOUND)),)
PPU_CPPFLAGS += -DWITHOUT_SOUND

else

PPU_SRCS += at3plus.c waveout.c

ifneq (exists, $(shell [ -f at3plus.c -a -f at3plus.h -a -f waveout.c -a -f waveout.h ] && echo exists) )
PPU_INCDIRS += -I$(CELL_SDK)/samples/sdk/codec/atrac3plus_simple

at3plus.c: $(CELL_SDK)/samples/sdk/codec/atrac3plus_simple/at3plus.c
	cp $< $@
waveout.c: $(CELL_SDK)/samples/sdk/codec/atrac3plus_simple/waveout.c
	cp $< $@
endif

endif

include $(CELL_MK_DIR)/sdk.target.mk

all : EBOOT.BIN $(PKG_TARGET) docs
docs : readme.html readme.pdf

$(OBJS_DIR)/$(PPU_TARGET): $(PPU_TARGET)
	@mkdir -p $(dir $(@))
	$(PPU_STRIP) -s $< -o $@
	@echo setting ftp home path to /
	./ext/libftphack/libftphack $@

PS3_GAME/PARAM.SFO: package.xml
	$(SFO) -f $< $@

PS3_GAME/USRDIR/EBOOT.BIN: $(OBJS_DIR)/$(PPU_TARGET) PS3_GAME/PARAM.SFO
	@mkdir -p $(dir $(@))
	$(SELF_NPDRM) $< $@ $(PACKAGE_NAME)

$(PKG_TARGET): PS3_GAME/USRDIR/EBOOT.BIN
	@echo generating package.
	$(PKG) --contentid $(PACKAGE_NAME) PS3_GAME/ $@
	$(PKG_GEO) $@

EBOOT.BIN: $(OBJS_DIR)/$(PPU_TARGET)	# to use in /app_home/PS3_GAME
	@echo generating EBOOT.BIN to use in /app_home/PS3_GAME
	$(MAKE_FSELF) $< $@

readme.html: readme.mkd
	maruku -o $@ $<

readme.pdf: readme.mkd
	maruku --pdf -o $@ $<

#$(OBJS_DIR)/version.ppu.o : PPU_CPPFLAGS += -DGITHEAD=\"$(GITHEAD)\"

#.PHONY: $(OBJS_DIR)/version.ppu.o
