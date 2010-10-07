CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS = main.cpp at3plus.c graphics.cpp waveout.c $(VPSHADER_PPU_OBJS) $(FPSHADER_PPU_OBJS)
PPU_TARGET = open_manager.elf

PPU_LDLIBS = -lfont_stub -lfontFT_stub -lfreetype_stub -lpthread -latrac3plus_stub -lmixer -laudio_stub -lftp  -lnet_stub -lnetctl_stub -lpngdec_stub -lm -ldbgfont_gcm -lgcm_cmd -lgcm_sys_stub -lio_stub -lsysmodule_stub -lsysutil_stub -lfs_stub\

all : EBOOT.BIN $(PKG_TARGET)

PPU_INCDIRS += -I$(CELL_SDK)/target/ppu/include/sysutil
PPU_CFLAGS  += -g

VPSHADER_SRCS = vpshader.cg vpshader2.cg
FPSHADER_SRCS = fpshader.cg fpshader2.cg

VPSHADER_PPU_OBJS = $(patsubst %.cg, $(OBJS_DIR)/%.ppu.o, $(VPSHADER_SRCS))
FPSHADER_PPU_OBJS = $(patsubst %.cg, $(OBJS_DIR)/%.ppu.o, $(FPSHADER_SRCS))

PKG_TARGET = UP0001-OMAN46756_00-0000111122223333.pkg

CLEANFILES = PS3_GAME/USRDIR/EBOOT.BIN $(OBJS_DIR)/$(PPU_TARGET)

include $(CELL_MK_DIR)/sdk.target.mk

PPU_OBJS += $(VPSHADER_PPU_OBJS) $(FPSHADER_PPU_OBJS)

$(VPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.vpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@

$(FPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.fpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@

$(OBJS_DIR)/$(PPU_TARGET): $(PPU_TARGET)
	$(PPU_STRIP) -s $< -o $@

PS3_GAME/USRDIR/EBOOT.BIN: $(OBJS_DIR)/$(PPU_TARGET)
	$(MAKE_FSELF_NPDRM) $< $@

$(PKG_TARGET): openmanager.conf PS3_GAME/USRDIR/EBOOT.BIN
	@echo generating package.
	$(MAKE_PACKAGE_NPDRM) openmanager.conf PS3_GAME/

EBOOT.BIN: $(OBJS_DIR)/$(PPU_TARGET)	# to use in /app_home/PS3_GAME
	@echo generating EBOOT.BIN to use in /app_home/PS3_GAME
	$(MAKE_FSELF) $< $@
