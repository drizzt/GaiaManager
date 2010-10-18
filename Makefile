.PHONY: gen_pkg npdrm_package

CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PPU_SRCS = main.cpp graphics.cpp icon.cpp syscall8.c $(VPSHADER_PPU_OBJS) $(FPSHADER_PPU_OBJS)
PPU_TARGET = open_manager.elf

PPU_INCDIRS	+= -I$(CELL_SDK)/target/ppu/include/sysutil
PPU_CFLAGS  += -g


PPU_LDLIBS 	+= -lftp  -lnet_stub -lrtc_stub -lnetctl_stub\
                    -lpngdec_stub -lm -ldbgfont_gcm -lgcm_cmd -lgcm_sys_stub\
		    -lio_stub -lsysmodule_stub -lsysutil_stub -lfs_stub

VPSHADER_SRCS = vpshader.cg vpshader2.cg
FPSHADER_SRCS = fpshader.cg fpshader2.cg

VPSHADER_PPU_OBJS = $(patsubst %.cg, $(OBJS_DIR)/%.ppu.o, $(VPSHADER_SRCS))
FPSHADER_PPU_OBJS = $(patsubst %.cg, $(OBJS_DIR)/%.ppu.o, $(FPSHADER_SRCS))

include $(CELL_MK_DIR)/sdk.target.mk

PPU_OBJS += $(VPSHADER_PPU_OBJS) $(FPSHADER_PPU_OBJS)

$(VPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.vpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@

$(FPSHADER_PPU_OBJS): $(OBJS_DIR)/%.ppu.o : %.fpo
	@mkdir -p $(dir $(@))
	$(PPU_OBJCOPY)  -I binary -O elf64-powerpc-celloslv2 -B powerpc $< $@


gen_pkg:: $(PPU_TARGET)
	$(PPU_STRIP) -s $< -o $(OBJS_DIR)/$(PPU_TARGET)
	$(MAKE_FSELF) $(OBJS_DIR)/$(PPU_TARGET) EBOOT.BIN				# to use in /app_home/PS3_GAME
	$(MAKE_FSELF_NPDRM) $(OBJS_DIR)/$(PPU_TARGET) PS3_GAME/USRDIR/EBOOT.BIN
	$(MAKE_PACKAGE_NPDRM) openmanager.conf PS3_GAME/				# to install pkg