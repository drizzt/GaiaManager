
#include <sys/spu_initialize.h>
#include <sys/ppu_thread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/process.h>
#include <sys/memory.h>
#include <sys/timer.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <cell/gcm.h>
#include <cell/pad/libpad.h>
#include <cell/keyboard.h>
#include <cell/sysmodule.h>
#include <cell/dbgfont.h>
#include <cell/codec/pngdec.h>
#include <cell/cell_fs.h>

#include <sysutil/sysutil_sysparam.h>
#include <sysutil/sysutil_msgdialog.h>
#include <cell/font.h>
#include <sys/ppu_thread.h>

#include <netex/net.h>
#include <netex/libnetctl.h>
#include <cell/fontFT.h>

#include <libftp.h>

#include "config.h"
#include "dialog.h"
#include "fileutils.h"
#include "i18n.h"
#include "main.h"
#include "network.h"
#include "graphics.h"
#include "parse.h"
#include "syscall8.h"
#ifndef WITHOUT_SOUND
#include "at3plus.h"
#endif

#define MAX_LIST 512

#define SETTINGS_FILE "/dev_hdd0/game/" FOLDER_NAME "/settings.cfg"

/* Embedded (default) PNG files */
extern uint32_t _binary_PNG_BGG_PNG_end;
extern uint32_t _binary_PNG_BGG_PNG_start;
extern uint32_t _binary_PNG_BGH_PNG_end;
extern uint32_t _binary_PNG_BGH_PNG_start;
extern uint32_t _binary_PNG_HIGHLIGHT_PNG_end;
extern uint32_t _binary_PNG_HIGHLIGHT_PNG_start;

enum BmModes {
	GAME = 0,
	HOMEBREW = 1
};

#ifndef WITHOUT_SOUND
static int fm = -1;
static char soundfile[512];
static sys_ppu_thread_t soundThread = 0;
#endif

static char hdd_folder[64] = "ASDFGHJKLMN";	// folder for games (deafult string is changed the first time it is loaded
static char hdd_folder_home[64] = FOLDER_NAME;	// folder for homebrew

static t_menu_list menu_list[MAX_LIST];
static int max_menu_list = 0;

static bool direct_boot = false;
static bool disc_less = false;
static int payload_type = 0;	//0 -> psgroove (or old psfreedom), 1 -> new pl3 with syscall35

static uint64_t mem_orig = 0x386000014E800020ULL;
static uint64_t mem_patched = 0xE92296887C0802A6ULL;
static uint64_t patchmode = 2;	//0 -> PS3 perms normally, 1-> Psjailbreak by default, 2-> Special for games as F1 2010 (option by default)

static t_menu_list menu_homebrew_list[MAX_LIST];
static int max_menu_homebrew_list = 0;
static int *max_list = &max_menu_list;

static enum BmModes mode_list = GAME;

static int game_sel = 0;

static unsigned cmd_pad = 0;

static void *host_addr;

static int up_count = 0, down_count = 0, left_count = 0, right_count = 0;

u32 new_pad = 0, old_pad = 0;

static int counter_png = 0;
static int old_fi = -1;
static u32 fdevices = 0;
static u32 forcedevices = 0;
static char filename[1024];
static char bluray_game[64];	// name of the game
static int ftp_flags = 0;
static int unload_mod = 0;
static bool want_to_quit = false;	// true when I need to quit

static int png_w = 0, png_h = 0;

time_t time_start;				// time counter init

static char string1[256];

static int load_libfont_module(void);
static void ftp_on(void);
static void ftp_off(void);
static int load_modules(void);
static int unload_modules(void);
static void *png_malloc(u32 size, void *a);
static int png_free(void *ptr, void *a);
static int png_out_mapmem(u8 * buffer, size_t buf_size);
#ifndef WITHOUT_SOUND
static void playBootSound(uint64_t ui __attribute__ ((unused)));
#endif
static int load_png_texture(u8 * data, char *name, uint32_t size);
static uint32_t syscall35(const char *srcpath, const char *dstpath);
void syscall36(const char *path);	// for some strange reasons it does not work as static
static void restorecall36(const char *path);
//static uint64_t peekq(uint64_t addr);
static void pokeq(uint64_t addr, uint64_t val);
static void cleanup(void);
static void fix_perm_recursive(const char *start_path);
static void sort_entries(t_menu_list * list, int *max);
static void delete_entries(t_menu_list * list, int *max, u32 flag);
static void fill_entries_from_device(char *path, t_menu_list * list, int *max, u32 flag, int sel);
#ifndef WITHOUT_SAVE_STATUS
static void parse_ini(void);
#endif
static void update_game_folder(char *ebootbin);
static void quit(void);
static void reset_game_list(int force, int sel);
static void set_hermes_mode(uint64_t mode);
static void copy_from_bluray(void);

static int load_libfont_module(void)
{
	int ret;
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FONT);
	if (ret == CELL_OK) {
		ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FREETYPE);
		if (ret == CELL_OK) {
			ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FONTFT);
			if (ret == CELL_OK) {
				return ret;		// Success
			}
			// Error handling as follows (Unload all loads)
			cellSysmoduleUnloadModule(CELL_SYSMODULE_FREETYPE);
		}
		cellSysmoduleUnloadModule(CELL_SYSMODULE_FONT);
	}
	return ret;					// Error end
}

int pad_read(void)
{
	int ret;

	u32 padd;

	u32 paddLX, paddLY, paddRX, paddRY;

	CellPadData databuf;
#if (CELL_SDK_VERSION<=0x210001)
	CellPadInfo infobuf;
#else
	CellPadInfo2 infobuf;
#endif
	static u32 old_info = 0;

	cmd_pad = 0;

#if (CELL_SDK_VERSION<=0x210001)
	ret = cellPadGetInfo(&infobuf);
#else
	ret = cellPadGetInfo2(&infobuf);
#endif
	if (ret != 0) {
		old_pad = new_pad = 0;
		return 1;
	}
#if (CELL_SDK_VERSION<=0x210001)
	if (infobuf.status[0] == CELL_PAD_STATUS_DISCONNECTED) {
#else
	if (infobuf.port_status[0] == CELL_PAD_STATUS_DISCONNECTED) {
#endif
		old_pad = new_pad = 0;
		return 1;
	}
#if (CELL_SDK_VERSION<=0x210001)
	if ((infobuf.info & CELL_PAD_INFO_INTERCEPTED)
		&& (!(old_info & CELL_PAD_INFO_INTERCEPTED))) {
		old_info = infobuf.info;
	} else if ((!(infobuf.info & CELL_PAD_INFO_INTERCEPTED))
			   && (old_info & CELL_PAD_INFO_INTERCEPTED)) {
		old_info = infobuf.info;
		old_pad = new_pad = 0;
		return 1;
	}
#else
	if ((infobuf.system_info & CELL_PAD_INFO_INTERCEPTED)
		&& (!(old_info & CELL_PAD_INFO_INTERCEPTED))) {
		old_info = infobuf.system_info;
	} else if ((!(infobuf.system_info & CELL_PAD_INFO_INTERCEPTED))
			   && (old_info & CELL_PAD_INFO_INTERCEPTED)) {
		old_info = infobuf.system_info;
		old_pad = new_pad = 0;
		return 1;
	}
#endif

	ret = cellPadGetData(0, &databuf);

	if (ret != CELL_OK) {
		old_pad = new_pad = 0;
		return 1;
	}

	if (databuf.len == 0) {
		new_pad = 0;
		return 1;
	}

	padd = (databuf.button[2] | (databuf.button[3] << 8));

	/* @drizzt Add support for analog sticks
	 * TODO: Add support for right analog stick */
	paddRX = databuf.button[4];
	paddRY = databuf.button[5];
	paddLX = databuf.button[6];
	paddLY = databuf.button[7];

	if (paddLX < 0x10)
		padd |= BUTTON_LEFT;
	else if (paddLX > 0xe0)
		padd |= BUTTON_RIGHT;

	if (paddLY < 0x10)
		padd |= BUTTON_UP;
	else if (paddLY > 0xe0)
		padd |= BUTTON_DOWN;

	new_pad = padd & (~old_pad);
	old_pad = padd;

	return 1;
}

/****************************************************/
/* FTP SECTION                                      */
/****************************************************/

static void ftp_handler(CellFtpServiceEvent event, void *data __attribute__ ((unused)), size_t datalen
						__attribute__ ((unused)))
{

//DPrintf("Event %i %x %i\n", event, (u32) data, datalen);

	switch (event) {
	case 0:

		break;
	case CELL_FTP_SERVICE_EVENT_SHUTDOWN:
	case CELL_FTP_SERVICE_EVENT_FATAL:
	case CELL_FTP_SERVICE_EVENT_STOPPED:

		ftp_flags |= 4;

		break;
	default:
		break;
	}
}

static void ftp_on(void)
{
//  int ret;

	if (!(ftp_flags & 1)) {
//      ret = sys_net_initialize_network();
//      if (ret < 0)
//          return;
		ftp_flags |= 1;
	}

	if (!(ftp_flags & 2)
		&& cellFtpServiceRegisterHandler(ftp_handler) >= 0) {
		ftp_flags |= 2;
		cellFtpServiceStart();
	}
}

static void ftp_off(void)
{

	if (ftp_flags & 2) {
		uint64_t result;

		cellFtpServiceStop(&result);

		cellFtpServiceUnregisterHandler();

		ftp_flags &= ~6;
	}

	if (ftp_flags & 1) {
//      sys_net_finalize_network();
		ftp_flags &= ~1;
	}
}

/****************************************************/
/* MODULES SECTION                                  */
/****************************************************/

static int load_modules(void)
{
	int ret;

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	if (ret != CELL_OK)
		return ret;
	else
		unload_mod |= 1;

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_PNGDEC);
	if (ret != CELL_OK)
		return ret;
	else
		unload_mod |= 2;

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_IO);
	if (ret != CELL_OK)
		return ret;
	else
		unload_mod |= 4;

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_GCM_SYS);
	if (ret != CELL_OK)
		return ret;
	else
		unload_mod |= 8;

	host_addr = memalign(0x100000, 0x100000);
	if (cellGcmInit(0x10000, 0x100000, host_addr) != CELL_OK)
		return -1;

	if (initDisplay() != 0)
		return -1;

	initShader();

	setDrawEnv();

	if (setRenderObject())
		return -1;

	ret = cellPadInit(1);
	if (ret != 0)
		return ret;

	setRenderTarget();

	initFont();

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
	if (ret < 0)
		return ret;
	else
		unload_mod |= 16;

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_HTTP);
	if (ret != CELL_OK)
		return ret;
	else
		unload_mod |= 32;

#ifndef WITHOUT_SOUND
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_ATRAC3PLUS);
	if (ret != CELL_OK)
		return ret;
	else
		unload_mod |= 64;
#endif

	return ret;
}

static int unload_modules(void)
{
#ifndef WITHOUT_SAVE_STATUS
	FILE *fid;

	// save previous status
	fid = fopen(SETTINGS_FILE, "w");
	if (fid) {
		fprintf(fid, "patchmode = %llu\n", patchmode);
		fprintf(fid, "disc_less = %d\n", disc_less);
		fprintf(fid, "direct_boot = %d\n", direct_boot);
		fprintf(fid, "ftp_flags = %d\n", ftp_flags);
		fprintf(fid, "hdd_folder = %s\n", hdd_folder);
		fclose(fid);
	}
#endif

	sys_net_finalize_network();

	ftp_off();

	cellPadEnd();

	termFont();

	free(host_addr);

#ifndef WITHOUT_SOUND
	if (unload_mod & 64)
		cellSysmoduleUnloadModule(CELL_SYSMODULE_ATRAC3PLUS);
#endif

	if (unload_mod & 32)
		cellSysmoduleUnloadModule(CELL_SYSMODULE_HTTP);

	if (unload_mod & 16)
		cellSysmoduleUnloadModule(CELL_SYSMODULE_NET);

	if (unload_mod & 8)
		cellSysmoduleUnloadModule(CELL_SYSMODULE_GCM_SYS);

	if (unload_mod & 4)
		cellSysmoduleUnloadModule(CELL_SYSMODULE_IO);

	if (unload_mod & 2)
		cellSysmoduleUnloadModule(CELL_SYSMODULE_PNGDEC);

	if (unload_mod & 1)
		cellSysmoduleUnloadModule(CELL_SYSMODULE_FS);

	return 0;
}

SYS_PROCESS_PARAM(1001, 0x10000)

/****************************************************/
/* PNG SECTION                                      */
/****************************************************/
typedef struct CtrlMallocArg {
	u32 mallocCallCounts;

} CtrlMallocArg;

typedef struct CtrlFreeArg {
	u32 freeCallCounts;

} CtrlFreeArg;

static void *png_malloc(u32 size, void *a)
{
	CtrlMallocArg *arg;

	arg = (CtrlMallocArg *) a;

	arg->mallocCallCounts++;

	return malloc(size);
}

static int png_free(void *ptr, void *a)
{
	CtrlFreeArg *arg;

	arg = (CtrlFreeArg *) a;

	arg->freeCallCounts++;

	free(ptr);

	return 0;
}

static int png_out_mapmem(u8 * buffer, size_t buf_size)
{
	int ret;
	u32 offset;

	ret = cellGcmMapMainMemory(buffer, buf_size, &offset);

	if (CELL_OK != ret)
		return ret;

	return 0;

}

#ifndef WITHOUT_SOUND
static void playBootSound(uint64_t ui __attribute__ ((unused)))
{
	int32_t status;
	BGMArg bgmArg;

	cellFsClose(fm);
	status = cellFsOpen(soundfile, CELL_FS_O_RDONLY, &fm, NULL, 0);
	bgmArg.fd = fm;
	status = init_atrac3plus(&bgmArg);
	play_atrac3plus((uintptr_t) & bgmArg);
	cellFsClose(fm);
	sys_ppu_thread_exit(0);

}
#endif

/* This function read a PNG to a variable (data).
 * If size == 0 name is the filename of the PNG file to load,
 * else name is the buffer containing the PNG data and size is its size
 */
static int load_png_texture(u8 * data, char *name, uint32_t size)
{
	int ret_file, ret, ok = -1;

	CellPngDecMainHandle mHandle;
	CellPngDecSubHandle sHandle;

	CellPngDecThreadInParam InParam;
	CellPngDecThreadOutParam OutParam;

	CellPngDecSrc src;
	CellPngDecOpnInfo opnInfo;
	CellPngDecInfo info;
	CellPngDecDataOutInfo dOutInfo;
	CellPngDecDataCtrlParam dCtrlParam;
	CellPngDecInParam inParam;
	CellPngDecOutParam outParam;

	CtrlMallocArg MallocArg;
	CtrlFreeArg FreeArg;

	int ret_png = -1;

	InParam.spuThreadEnable = CELL_PNGDEC_SPU_THREAD_DISABLE;
	InParam.ppuThreadPriority = 512;
	InParam.spuThreadPriority = 200;
	InParam.cbCtrlMallocFunc = png_malloc;
	InParam.cbCtrlMallocArg = &MallocArg;
	InParam.cbCtrlFreeFunc = png_free;
	InParam.cbCtrlFreeArg = &FreeArg;

	ret_png = ret = cellPngDecCreate(&mHandle, &InParam, &OutParam);

	memset(data, 0xff, (DISPLAY_WIDTH * DISPLAY_HEIGHT * 4));

	png_w = png_h = 0;

	if (ret_png == CELL_OK) {

		memset(&src, 0, sizeof(CellPngDecSrc));
		if (size == 0) {
			src.srcSelect = CELL_PNGDEC_FILE;
			src.fileName = name;
		} else {
			src.srcSelect = CELL_PNGDEC_BUFFER;
			src.streamPtr = name;
			src.streamSize = size;
		}

		src.spuThreadEnable = CELL_PNGDEC_SPU_THREAD_DISABLE;

		ret_file = ret = cellPngDecOpen(mHandle, &sHandle, &src, &opnInfo);

		if (ret == CELL_OK) {
			ret = cellPngDecReadHeader(mHandle, sHandle, &info);
		}

		if (ret == CELL_OK) {
			inParam.commandPtr = NULL;
			inParam.outputMode = CELL_PNGDEC_TOP_TO_BOTTOM;
			inParam.outputColorSpace = CELL_PNGDEC_RGBA;
			inParam.outputBitDepth = 8;
			inParam.outputPackFlag = CELL_PNGDEC_1BYTE_PER_1PIXEL;

			if ((info.colorSpace == CELL_PNGDEC_GRAYSCALE_ALPHA)
				|| (info.colorSpace == CELL_PNGDEC_RGBA)
				|| (info.chunkInformation & 0x10))
				inParam.outputAlphaSelect = CELL_PNGDEC_STREAM_ALPHA;
			else
				inParam.outputAlphaSelect = CELL_PNGDEC_FIX_ALPHA;

			inParam.outputColorAlpha = 0xff;

			ret = cellPngDecSetParameter(mHandle, sHandle, &inParam, &outParam);
		}

		if (ret == CELL_OK) {
			dCtrlParam.outputBytesPerLine = DISPLAY_WIDTH * 4;
			ret = cellPngDecDecodeData(mHandle, sHandle, data, &dCtrlParam, &dOutInfo);
			//sys_timer_usleep(300);

			if ((ret == CELL_OK)
				&& (dOutInfo.status == CELL_PNGDEC_DEC_STATUS_FINISH)) {
				png_w = outParam.outputWidth;
				png_h = outParam.outputHeight;
				ok = 0;
			}
		}

		if (ret_file == 0)
			ret = cellPngDecClose(mHandle, sHandle);

		ret = cellPngDecDestroy(mHandle);

	}

	InParam.spuThreadEnable = CELL_PNGDEC_SPU_THREAD_DISABLE;

	return ok;
}

/****************************************************/
/* syscalls                                         */
/****************************************************/

/* psfreedom uses a generic syscall35 instead of syscall36 */
static uint32_t syscall35(const char *srcpath, const char *dstpath)
{
	system_call_2(35, (uint32_t) srcpath, (uint32_t) dstpath);
	return_to_user_prog(uint32_t);
}

// for some strange reasons syscall36 does not work as static
void syscall36(const char *path)
{
	if (syscall35("/dev_bdvd", path) != 0) {
		system_call_1(36, (uint32_t) path);
	} else if (disc_less) {
		syscall35("/app_home", path);
	}
}

static void restorecall36(const char *path)
{
	if (syscall35(path, NULL) != 0) {
		system_call_1(36, (uint32_t) path);
	}
}

/****************************************************/
/* UTILS                                            */
/****************************************************/

#if 0							// Unused
static uint64_t peekq(uint64_t addr)
{
	system_call_1(6, addr);
	return_to_user_prog(uint64_t);
}
#endif

static void pokeq(uint64_t addr, uint64_t val)
{
	system_call_2(7, addr, val);
}

static void fix_perm_recursive(const char *start_path)
{
	int dir_fd;
	uint64_t nread;
	char f_name[CELL_FS_MAX_FS_FILE_NAME_LENGTH + 1];
	CellFsDirent dir_ent;
	CellFsErrno err;
	if (cellFsOpendir(start_path, &dir_fd) == CELL_FS_SUCCEEDED) {
		cellFsChmod(start_path, 0777);
		while (1) {
			err = cellFsReaddir(dir_fd, &dir_ent, &nread);
			if (nread != 0) {
				if (!strcmp(dir_ent.d_name, ".")
					|| !strcmp(dir_ent.d_name, ".."))
					continue;

				sprintf(f_name, "%s/%s", start_path, dir_ent.d_name);

				if (dir_ent.d_type == CELL_FS_TYPE_DIRECTORY) {
					cellFsChmod(f_name, 0777);
					fix_perm_recursive(f_name);
				} else if (dir_ent.d_type == CELL_FS_TYPE_REGULAR) {
					cellFsChmod(f_name, 0666);
				}
			} else {
				break;
			}
		}
		err = cellFsClosedir(dir_fd);
	}
}

static void sort_entries(t_menu_list * list, int *max)
{
	int n, m;
	int fi = (*max);

	for (n = 0; n < (fi - 1); n++)
		for (m = n + 1; m < fi; m++) {
			if ((strcasecmp(list[n].title, list[m].title) > 0 && ((list[n].flags | list[m].flags) & 2048) == 0)
				|| ((list[m].flags & 2048) && n == 0)) {
				t_menu_list swap;
				swap = list[n];
				list[n] = list[m];
				list[m] = swap;
			}
		}
}

static void delete_entries(t_menu_list * list, int *max, u32 flag)
{
	int n;

	n = 0;

	while (n < (*max)) {
		if (list[n].flags & flag) {
			if ((*max) > 1) {
				list[n].flags = 0;
				list[n] = list[(*max) - 1];
				(*max)--;
			} else {
				if ((*max) == 1)
					(*max)--;
				break;
			}

		} else
			n++;

	}
}

static void fill_entries_from_device(char *path, t_menu_list * list, int *max, u32 flag, int sel)
{
	DIR *dir;
	char file[1024];

	delete_entries(list, max, flag);

	if ((*max) < 0)
		*max = 0;

	dir = opendir(path);
	if (!dir)
		return;

	while (1) {
		struct dirent *entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		if (!(entry->d_type & DT_DIR))
			continue;

		list[*max].flags = flag;

		strncpy(list[*max].title, entry->d_name, 63);
		list[*max].title[63] = 0;

		sprintf(list[*max].path, "%s/%s", path, entry->d_name);

		if (sel == 0) {
			// read name in PARAM.SFO
			sprintf(file, "%s/PS3_GAME/PARAM.SFO", list[*max].path);

			parse_param_sfo(file, "TITLE", list[*max].title + 1 * (list[*max].title[0] == '_'));	// move +1 with '_'
			list[*max].title[63] = 0;
			parse_param_sfo(file, "TITLE_ID", list[*max].title_id);
			list[*max].title_id[63] = 0;
		} else {
			struct stat s;
			sprintf(file, "%s/EBOOT.BIN", list[*max].path);
			if (stat(file, &s) < 0)
				continue;
		}

		(*max)++;

		if (*max >= MAX_LIST)
			break;

	}

	closedir(dir);

}

#ifndef WITHOUT_SAVE_STATUS
static void parse_ini(void)
{
	FILE *fid;

	fid = fopen(SETTINGS_FILE, "r");
	if (!fid)
		return;

	while (fgets(filename, sizeof(filename), fid) != NULL) {
		if (filename[strlen(filename) - 1] == '\n')
			filename[strlen(filename) - 1] = '\0';
		if (strncmp(filename, "patchmode = ", 12) == 0)
			patchmode = strtoull(&filename[12], NULL, 10);
		else if (strncmp(filename, "disc_less = ", 12) == 0)
			disc_less = atoi(&filename[12]);
		else if (strncmp(filename, "direct_boot = ", 14) == 0)
			direct_boot = atoi(&filename[14]);
		else if (strncmp(filename, "ftp_flags = ", 12) == 0)
			ftp_flags = atoi(&filename[12]);
		else if (strncmp(filename, "hdd_folder = ", 13) == 0)
			strncpy(hdd_folder, &filename[13], sizeof(hdd_folder) - 1);
	}

	fclose(fid);
}
#endif

static void cleanup(void)
{
	struct dirent *entry;
	DIR *dir = opendir("/dev_hdd0/home");

	unlink("/dev_hdd0/vsh/pushlist/patch.dat");
	unlink("/dev_hdd0/vsh/pushlist/game.dat");

	if (!dir)
		return;

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.')
			continue;
		if (!(entry->d_type & DT_DIR))
			continue;

		sprintf(filename, "/dev_hdd0/home/%s/etc/boot_history.dat", entry->d_name);
		unlink(filename);
	}

	closedir(dir);
}

static void update_game_folder(char *ebootbin
#ifndef WITHOUT_SAVE_STATUS
							   __attribute__ ((unused))
#endif
	)
{
	int ret, dir_fixed = 0;
	char old_hdd_folder[64] = { 0, };

	DIR *dir, *dir2;
	strncpy(old_hdd_folder, hdd_folder, sizeof(hdd_folder));
	dir = opendir("/dev_hdd0/game");

	if (dir) {
		while (1) {
			struct dirent *entry = readdir(dir);

			if (!entry)
				break;
			if (entry->d_name[0] == '.')
				continue;

			if (!(entry->d_type & DT_DIR))
				continue;

			sprintf(filename, "/dev_hdd0/game/%s/%s", entry->d_name, GAMES_DIR);
			//sprintf(filename, "/dev_hdd0/GAMES");

			dir2 = opendir(filename);

			if (dir2) {
				closedir(dir2);

				dialog_ret = 0;

				sprintf(filename, "%s /%s %s", text_wantuse[region], entry->d_name, text_toinstall[region]);

				ret = cellMsgDialogOpen2(type_dialog_yes_no, filename, dialog_fun1, (void *) 0x0000aaaa, NULL);

				wait_dialog();

				if (dialog_ret == 1) {
					strncpy(hdd_folder, entry->d_name, 64);
					dir_fixed = 1;
					break;
				}
			}

		}
		closedir(dir);
	}

	if (!dir_fixed) {
		strcpy(hdd_folder, "./../.");	// Don't change it to ".."

		dir_fixed = 1;

		// create the folder
		sprintf(filename, "/dev_hdd0/game/%s/%s", hdd_folder, GAMES_DIR);
		//sprintf(filename, "/dev_hdd0/GAMES");
		mkdir(filename, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

		dialog_ret = 0;
		sprintf(filename, "/dev_hdd0/%s is the new folder for games", GAMES_DIR);

		ret = cellMsgDialogOpen2(type_dialog_ok, filename, dialog_fun2, (void *) 0x0000aaab, NULL);
		wait_dialog();
	}
#ifdef WITHOUT_SAVE_STATUS
	// modify EBOOT.BIN
	if (dir_fixed && ebootbin) {
		FILE *fp;
		int n;

		fp = fopen(ebootbin, "r+");
		if (fp != NULL) {
			int len;
			char *mem = NULL;

			fseek(fp, 0, SEEK_END);
			len = ftell(fp);

			mem = (char *) malloc(len + 16);
			if (!mem) {
				fclose(fp);
				return;
			}

			fseek(fp, 0, SEEK_SET);

			fread((void *) mem, len, 1, fp);

			for (n = 0; n < (int) (len - strlen(old_hdd_folder)); n++) {

				if (!memcmp(&mem[n], old_hdd_folder, strlen(old_hdd_folder))) {
					strncpy(&mem[n], hdd_folder, 64);

					fseek(fp, 0, SEEK_SET);
					fwrite((void *) mem, len, 1, fp);
					dialog_ret = 0;
					sprintf(filename, "%s\n%s", text_eboot[region], text_launcher[region]);
					ret = cellMsgDialogOpen2(type_dialog_ok, filename, dialog_fun2, (void *) 0x0000aaab, NULL);
					wait_dialog();

					break;
				}
			}

			fclose(fp);
		}
	}
#endif
	reset_game_list(1, 0);
}

static void quit(void)
{
	if (mode_list == GAME) {
		restorecall36((char *) "/app_home");
		restorecall36((char *) "/dev_bdvd");	// restore bluray
	} else {
		restorecall36((char *) "/dev_usb000");	// restore
	}

	cleanup();
	unload_modules();

	sys_process_exit(1);
}

/*****************************************************/
/* CALLBACK                                          */
/*****************************************************/

static void gfxSysutilCallback(uint64_t status, uint64_t param, void *userdata)
{
	(void) param;
	(void) userdata;
	switch (status) {
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		want_to_quit = true;
		break;
	case CELL_SYSUTIL_DRAWING_BEGIN:
	case CELL_SYSUTIL_DRAWING_END:
		break;
	default:
		printf("Graphics common: Unknown status received: 0x%llx\n", status);
	}
}

static void reset_game_list(int force, int sel)
{
	old_fi = -1;
	counter_png = 0;
	forcedevices = force;
	game_sel = sel;
}

static void set_hermes_mode(uint64_t mode)
{
	if (sys8_enable(0) > 0) {
		sys8_perm_mode(mode);
	} else {
		pokeq(0x80000000000505d0ULL, mode == 0 ? mem_patched : mem_orig);
	}
}

static void copy_from_bluray(void)
{
	int ret;
	char name[1024];
	int curr_device = 0;
	CellFsStat fstatus;
	char id[16];

	int n;

	for (n = 0; n < 11; n++) {
		dialog_ret = 0;

		if ((fdevices >> n) & 1) {

			if (n == 0)
				sprintf(filename, "%s\n\n%s BDVD %s HDD0?", bluray_game, text_wantcopy[region], text_to[region]);
			else
				sprintf(filename, "%s\n\n%s BDVD %s USB00%c?", bluray_game, text_wantcopy[region], text_to[region],
						47 + n);

			ret = cellMsgDialogOpen2(type_dialog_yes_no, filename, dialog_fun1, (void *) 0x0000aaaa, NULL);

			wait_dialog();

			if (dialog_ret == 1) {
				curr_device = n;
				break;
			}					// exit
		}
	}

	if (dialog_ret == 1) {

		if (curr_device == 0)
			sprintf(name, "/dev_hdd0");
		else
			sprintf(name, "/dev_usb00%c", 47 + curr_device);

		if (cellFsStat(name, &fstatus) == CELL_FS_SUCCEEDED && !parse_ps3_disc((char *)
																			   "/dev_bdvd/PS3_DISC.SFB", id)) {

			// reset to update datas
			reset_game_list(1 << curr_device, 0);

			if (curr_device == 0) {
				sprintf(name, "/dev_hdd0/game/%s/", hdd_folder);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				sprintf(name, "/dev_hdd0/game/%s/%s", hdd_folder, GAMES_DIR);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				sprintf(name, "/dev_hdd0/game/%s/%s/%s", hdd_folder, GAMES_DIR, id);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
			} else {
				sprintf(name, "/dev_usb00%c/%s", 47 + curr_device, GAMES_DIR);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				sprintf(name, "/dev_usb00%c/%s", 47 + curr_device, GAMES_DIR);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				sprintf(name, "/dev_usb00%c/%s/%s", 47 + curr_device, GAMES_DIR, id);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
			}

			time_start = time(NULL);
			abort_copy = 0;
			initConsole();
			file_counter = 0;
			new_pad = 0;

			if (curr_device != 0)
				copy_mode = 1;	// break files >= 4GB
			else
				copy_mode = 0;

			copy_is_split = 0;

			my_game_copy((char *) "/dev_bdvd", (char *) name);

			int seconds = (int) (time(NULL) - time_start);
			int vflip = 0;

			if (copy_is_split && !abort_copy) {

				if (curr_device == 0) {
					sprintf(filename, "/dev_hdd0/game/%s/%s/_%s", hdd_folder, GAMES_DIR, id);
				} else {
					sprintf(filename, "/dev_usb00%c/%s/_%s", 47 + curr_device, GAMES_DIR, id);
				}

				ret = rename(name, filename);

				if (curr_device == 0)
					sprintf(filename, "%s\n\nSplit game copied in HDD0 (non bootable)", id);
				else
					sprintf(filename, "%s\n\nSplit game copied in USB00%c (non bootable)", id, 47 + curr_device);

				dialog_ret = 0;
				ret = cellMsgDialogOpen2(type_dialog_ok, filename, dialog_fun2, (void *) 0x0000aaab, NULL);
				wait_dialog();

			}

			while (1) {

				if (abort_copy)
					sprintf(string1, "Aborted!!!  Time: %2.2i:%2.2i:%2.2i\n", seconds / 3600, (seconds / 60) % 60,
							seconds % 60);
				else {
					sprintf(string1, "Done! Files Copied: %i Time: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", file_counter,
							seconds / 3600, (seconds / 60) % 60, seconds % 60, ((double)
																				global_device_bytes)
							/ (1024.0 * 1024. * 1024.0));
				}

				cellGcmSetClearSurface(gCellGcmCurrentContext,
									   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
									   CELL_GCM_CLEAR_A);

				draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x000000ff);

				cellDbgFontPrintf(0.07f, 0.07f, 1.2f, 0xffffffff, string1);

				if (vflip & 32)
					cellDbgFontPrintf(0.5f - 0.15f, 1.0f - 0.07 * 2.0f, 1.2f, 0xffffffff, "Press X to Exit");
				vflip++;

				cellDbgFontDrawGcm();

				flip();

				pad_read();
				if (new_pad & BUTTON_CROSS) {
					new_pad = 0;
					break;
				}

			}

			if (abort_copy) {
				if (curr_device == 0)
					sprintf(filename, "%s\n\n%s HDD0?", id, text_delfailed[region]);
				else
					sprintf(filename, "%s\n\n%s USB00%c?", id, text_delfailed[region], 47 + curr_device);

				dialog_ret = 0;
				ret = cellMsgDialogOpen2(type_dialog_yes_no, filename, dialog_fun1, (void *) 0x0000aaaa, NULL);

				wait_dialog();

				if (dialog_ret == 1) {
					time_start = time(NULL);
					file_counter = 0;
					abort_copy = 0;
					my_game_delete((char *)
								   name);

					rmdir((char *) name);	// delete this folder

				} else {
					if (curr_device == 0) {
						sprintf(filename, "/dev_hdd0/game/%s/%s/_%s", hdd_folder, GAMES_DIR, id);
					} else {
						sprintf(filename, "/dev_usb00%c/%s/_%s", 47 + curr_device, GAMES_DIR, id);
					}

					ret = rename(name, filename);

				}
			}

			termConsole();
			game_sel = 0;

		}
	}
}

/****************************************************/
/* MAIN                                             */
/****************************************************/

int main(int argc, char *argv[])
{
	sys_spu_initialize(2, 0);
	//BGMArg bgmArg;
	int ret;
	//int    fm = -1;
	int one_time = 1;

	size_t pngsize = 0;
	char *pngptr = filename;
	struct stat st;

	u8 *text_bmp = NULL;
	u8 *text_h = NULL;
	u8 *text_bg = NULL;
	//char INPUT_FILE[] = "/dev_bdvd/PS3_GAME/SND0.AT3";
	ret = load_modules();
	load_libfont_module();

	// Fix EBOOT.BIN permission
	chmod(argv[0], 00666);

#ifndef WITHOUT_SOUND
	sprintf(soundfile, "/dev_hdd0/game/%s/USRDIR/BOOT.AT3", hdd_folder_home);
	sys_ppu_thread_create(&soundThread, playBootSound, NULL, 100, 0x8000, 0, (char *) "sound thread");
#endif

	//fix_perm_recursive("/dev_hdd0/game/OMAN46756/cache2/");
	cleanup();
	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_LANG, &region);

	cellNetCtlInit();

	sys_net_initialize_network();

#ifndef WITHOUT_SAVE_STATUS
	parse_ini();

	if (ftp_flags % 2) {
		int old_ftp_flags = ftp_flags;
		ftp_flags = 0;
		ftp_on();
		ftp_flags = old_ftp_flags;
	}
#endif

	//ftp_off();
	u32 frame_buf_size = DISPLAY_HEIGHT * DISPLAY_WIDTH * 4;

	frame_buf_size = (frame_buf_size + 0xfffff) & (~0xfffff);
	text_bmp = (u8 *) memalign(0x100000, frame_buf_size);
	text_bg = (u8 *) memalign(0x100000, frame_buf_size);
	text_h = (u8 *) memalign(0x100000, frame_buf_size);
	if (!text_bmp)
		exit(-1);

	if (png_out_mapmem(text_bmp, frame_buf_size))
		exit(-1);
	if (png_out_mapmem(text_bg, frame_buf_size))
		exit(-1);
	if (png_out_mapmem(text_h, frame_buf_size))
		exit(-1);

	cellSysutilRegisterCallback(0, gfxSysutilCallback, NULL);

	sprintf(filename, "/dev_hdd0/game/%s/USRDIR/BGG.PNG", hdd_folder_home);
	pngsize = 0;
	pngptr = filename;
	if (stat(filename, &st) < 0) {
		pngsize = _binary_PNG_BGG_PNG_end - _binary_PNG_BGG_PNG_start;
		pngptr = (char *) &_binary_PNG_BGG_PNG_start;
	}
	load_png_texture(text_bg, pngptr, pngsize);

	sprintf(filename, "/dev_hdd0/game/%s/USRDIR/HIGHLIGHT.PNG", hdd_folder_home);
	pngsize = 0;
	pngptr = filename;
	if (stat(filename, &st) < 0) {
		pngsize = _binary_PNG_HIGHLIGHT_PNG_end - _binary_PNG_HIGHLIGHT_PNG_start;
		pngptr = (char *) &_binary_PNG_HIGHLIGHT_PNG_start;
	}
	load_png_texture(text_h, pngptr, pngsize);

	setRenderColor();

	if (syscall35("/dev_hdd0", "/dev_hdd0") == 0) {
		payload_type = 1;
	} else {
		// Disable mem patch on startup
		set_hermes_mode(2ULL);
	}

	if (!memcmp(hdd_folder, "ASDFGHJKLM", 10) && hdd_folder[10] == 'N')
		update_game_folder(argv[0]);

	// create homebrew folder
	if (argc >= 1) {
		// creates homebrew folder
		if (!strncmp(argv[0], "/dev_hdd0/game/", 15)) {
			char *s;
			int n = 0;

			s = ((char *) argv[0]) + 15;
			while (*s != 0 && *s != '/' && n < 63) {
				hdd_folder_home[n] = *s++;
				n++;
			}
			hdd_folder_home[n] = 0;

			// create the folder
			sprintf(filename, "/dev_hdd0/game/%s/homebrew", hdd_folder_home);
			mkdir(filename, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
		}
	}

	max_menu_list = 0;
	max_menu_homebrew_list = 0;

	u32 fdevices_old = 0;
	int find_device = 0;

	restorecall36((char *) "/app_home");
	restorecall36((char *) "/dev_bdvd");	// select bluray

	/* main loop */
	while ((pad_read() != 0) || want_to_quit) {
		if (want_to_quit)
			quit();

		// scan for plug/unplug devices

		int count_devices = 0;

		if (one_time) {
			one_time = 0;
		} else {
			for (find_device = 0; find_device < 12; find_device++) {
				CellFsStat fstatus;
				if (find_device == 11)
					sprintf(filename, "/dev_bdvd");
				else if (find_device == 0)
					sprintf(filename, "/dev_hdd0");
				else
					sprintf(filename, "/dev_usb00%c", 47 + find_device);

				if (cellFsStat(filename, &fstatus) == CELL_FS_SUCCEEDED) {
					fdevices |= 1 << find_device;
				} else
					fdevices &= ~(1 << find_device);

				// limit to 5 the devices selectables
				if (((fdevices >> find_device) & 1)
					&& find_device != 11) {
					count_devices++;

					if (count_devices > 5)
						fdevices &= ~(1 << find_device);

				}
				// bdvd
				if (find_device == 11) {

					if (fdevices != fdevices_old || ((forcedevices >> find_device)
													 & 1)) {
						game_sel = 0;
						sprintf(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");
						bluray_game[0] = 0;
						parse_param_sfo(filename, "TITLE", bluray_game);
						bluray_game[63] = 0;

						if ((fdevices >> 11) & 1) {

							if (max_menu_list >= MAX_LIST)
								max_menu_list = MAX_LIST - 1;

							sprintf(menu_list[max_menu_list].path, "/dev_bdvd");

							memcpy(menu_list[max_menu_list].title, bluray_game, 63);
							menu_list[max_menu_list].title[63] = 0;
							menu_list[max_menu_list].flags = (1 << 11);

							parse_param_sfo(filename, "TITLE_ID", menu_list[max_menu_list].title_id);
							menu_list[max_menu_list].title_id[63] = 0;

							max_menu_list++;

						} else
							delete_entries(menu_list, &max_menu_list, (1 << 11));

						sort_entries(menu_list, &max_menu_list);
					}

					forcedevices &= ~(1 << find_device);
					fdevices_old &= ~(1 << find_device);
					fdevices_old |= fdevices & (1 << find_device);
				} else
					// refresh list 
				if (fdevices != fdevices_old || ((forcedevices >> find_device) & 1)) {
					game_sel = 0;

					forcedevices &= ~(1 << find_device);

					if (find_device == 0)
						sprintf(filename, "/dev_hdd0/game/%s/%s", hdd_folder, GAMES_DIR);
					else
						sprintf(filename, "/dev_usb00%c/%s", 47 + find_device, GAMES_DIR);

					if ((fdevices >> find_device) & 1)
						fill_entries_from_device(filename, menu_list, &max_menu_list, (1 << find_device), 0);
					else
						delete_entries(menu_list, &max_menu_list, (1 << find_device));
					sort_entries(menu_list, &max_menu_list);

					if (find_device >= 0) {
						if (find_device == 0)
							sprintf(filename, "/dev_hdd0/game/%s/homebrew", hdd_folder_home);
						else
							sprintf(filename, "/dev_usb00%c/homebrew", 47 + find_device);

						if ((fdevices >> find_device) & 1)
							fill_entries_from_device(filename, menu_homebrew_list, &max_menu_homebrew_list,
													 (1 << find_device), 1);
						else
							delete_entries(menu_homebrew_list, &max_menu_homebrew_list, (1 << find_device));
						sort_entries(menu_homebrew_list, &max_menu_homebrew_list);
					}
					fdevices_old &= ~(1 << find_device);
					fdevices_old |= fdevices & (1 << find_device);
				}
			}

			if (old_fi != game_sel && game_sel >= 0 && counter_png == 0) {
				old_fi = game_sel;
				if (mode_list == GAME) {
					sprintf(filename, "%s/COVER.PNG", menu_list[game_sel].path);
					if (stat(filename, &st) < 0) {
						sprintf(filename, "/dev_hdd0/%s/%s.PNG", COVERS_DIR, menu_list[game_sel].title_id);
						if (stat(filename, &st) < 0) {
							sprintf(filename, "%s/../../%s/%s.PNG", menu_list[game_sel].path, COVERS_DIR,
									menu_list[game_sel].title_id);
							if (stat(filename, &st) < 0) {
								sprintf(filename, "%s/PS3_GAME/ICON0.PNG", menu_list[game_sel].path);
							}
						}
					}
				} else
					sprintf(filename, "%s/ICON0.PNG", menu_homebrew_list[game_sel].path);
				load_png_texture(text_bmp, filename, 0);

				counter_png = 20;
			}

			if (counter_png)
				counter_png--;
		}

		if (old_pad & BUTTON_UP) {
			if (up_count > 7) {
				up_count = 0;
				game_sel--;
				if (game_sel < 0) {
					game_sel = *max_list - 1;
				}
			} else
				up_count++;

		} else
			up_count = 8;

		if (old_pad & BUTTON_LEFT) {
			if (left_count > 7) {
				left_count = 0;
				if (game_sel == 0) {
					game_sel = *max_list - 1;
				} else {
					game_sel = game_sel - 16;
					if (game_sel < 0)
						game_sel = 0;
				}
			} else
				left_count++;

		} else
			left_count = 8;

		if (old_pad & BUTTON_DOWN) {
			if (down_count > 7) {
				down_count = 0;
				game_sel++;
				if (game_sel >= *max_list)
					game_sel = 0;
			} else
				down_count++;

		} else
			down_count = 8;
		if (old_pad & BUTTON_L3) {
			reset_game_list(1, 0);
		}

		if (old_pad & BUTTON_RIGHT) {

			if (right_count > 7) {
				right_count = 0;
				if (game_sel == *max_list - 1) {
					game_sel = 0;
				} else {
					game_sel = game_sel + 16;
					if (game_sel >= *max_list)
						game_sel = *max_list - 1;
				}

			} else
				right_count++;

		} else
			right_count = 8;

		// update the game folder

		if ((new_pad & BUTTON_START) && (old_pad & BUTTON_SELECT))
			update_game_folder(argv[0]);

		if (new_pad & BUTTON_R2) {
			pngsize = 0;
			pngptr = filename;
			game_sel = 0;
			old_fi = -1;
			counter_png = 0;
			if (mode_list == GAME) {
				mode_list = HOMEBREW;
				max_list = &max_menu_homebrew_list;
				sprintf(filename, "/dev_hdd0/game/%s/USRDIR/BGH.PNG", hdd_folder_home);
				if (stat(filename, &st) < 0) {
					pngsize = _binary_PNG_BGH_PNG_end - _binary_PNG_BGH_PNG_start;
					pngptr = (char *) &_binary_PNG_BGH_PNG_start;
				}
			} else {
				mode_list = GAME;
				max_list = &max_menu_list;
				sprintf(filename, "/dev_hdd0/game/%s/USRDIR/BGG.PNG", hdd_folder_home);
				if (stat(filename, &st) < 0) {
					pngsize = _binary_PNG_BGG_PNG_end - _binary_PNG_BGG_PNG_start;
					pngptr = (char *) &_binary_PNG_BGG_PNG_start;
				}
			}
			load_png_texture(text_bg, pngptr, pngsize);
		}

		if ((new_pad & BUTTON_R3) && game_sel >= 0 && max_menu_list > 0 && mode_list == GAME) {

			time_start = time(NULL);

			abort_copy = 0;

			initConsole();

			file_counter = 0;
			new_pad = 0;

			global_device_bytes = 0;

			num_directories = file_counter = num_files_big = num_files_split = 0;

			my_game_test(menu_list[game_sel].path, true);

			DPrintf("Directories: %i Files: %i\nBig files: %i Split files: %i\n\n", num_directories, file_counter,
					num_files_big, num_files_split);

			int seconds = (int) (time(NULL) - time_start);
			int vflip = 0;

			while (1) {

				if (abort_copy)
					sprintf(string1, "Aborted!!!  Time: %2.2i:%2.2i:%2.2i\n", seconds / 3600, (seconds / 60) % 60,
							seconds % 60);
				else
					sprintf(string1, "Files Tested: %i Time: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", file_counter,
							seconds / 3600, (seconds / 60) % 60, seconds % 60, ((double)
																				global_device_bytes)
							/ (1024.0 * 1024. * 1024.0));

				cellGcmSetClearSurface(gCellGcmCurrentContext,
									   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
									   CELL_GCM_CLEAR_A);

				draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x000000ff);

				cellDbgFontPrintf(0.07f, 0.07f, 1.2f, 0xffffffff, string1);

				if (vflip & 32)
					cellDbgFontPrintf(0.5f - 0.15f, 1.0f - 0.07 * 2.0f, 1.2f, 0xffffffff, "Press X to Exit");
				vflip++;

				cellDbgFontDrawGcm();

				flip();

				pad_read();
				if (new_pad & BUTTON_CROSS) {
					new_pad = 0;
					break;
				}

			}
			termConsole();

		}
// delete from devices  

		if ((new_pad & BUTTON_TRIANGLE) && game_sel >= 0 && *max_list > 0
			&& ((mode_list == GAME && (!(menu_list[game_sel].flags & 2048)))
				|| mode_list == HOMEBREW)) {
			int n;
			t_menu_list *menu_tmp_list = (mode_list == GAME) ? menu_list : menu_homebrew_list;
			for (n = 0; n < 11; n++) {
				if ((menu_tmp_list[game_sel].flags >> n) & 1)
					break;
			}

			if (n == 0)
				sprintf(filename, "%s\n\n%s HDD0?", menu_tmp_list[game_sel].title, text_wantdel[region]);
			else
				sprintf(filename, "%s\n\n%s USB00%c?", menu_tmp_list[game_sel].title, text_wantdel[region], 47 + n);

			dialog_ret = 0;
			ret = cellMsgDialogOpen2(type_dialog_yes_no, filename, dialog_fun1, (void *) 0x0000aaaa, NULL);

			wait_dialog();

			if (dialog_ret == 1) {

				time_start = time(NULL);

				abort_copy = 0;
				initConsole();
				file_counter = 0;
				new_pad = 0;

				DPrintf("Starting... \n delete %s\n\n", menu_tmp_list[game_sel].path);

				my_game_delete((char *)
							   menu_tmp_list[game_sel].path);

				rmdir((char *) menu_tmp_list[game_sel].path);	// delete this folder

				// reset to update datas
				reset_game_list(1 << n, 0);

				int seconds = (int) (time(NULL) - time_start);
				int vflip = 0;

				while (1) {

					if (abort_copy)
						sprintf(string1, "Aborted!!!  Time: %2.2i:%2.2i:%2.2i\n", seconds / 3600, (seconds / 60) % 60,
								seconds % 60);
					else
						sprintf(string1, "Done!  Files Deleted: %i Time: %2.2i:%2.2i:%2.2i\n", file_counter,
								seconds / 3600, (seconds / 60) % 60, seconds % 60);

					cellGcmSetClearSurface(gCellGcmCurrentContext,
										   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
										   CELL_GCM_CLEAR_A);

					draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x000000ff);

					cellDbgFontPrintf(0.07f, 0.07f, 1.2f, 0xffffffff, string1);

					if (vflip & 32)
						cellDbgFontPrintf(0.5f - 0.15f, 1.0f - 0.07 * 2.0f, 1.2f, 0xffffffff, "Press X to Exit");

					vflip++;

					cellDbgFontDrawGcm();

					flip();

					pad_read();
					if (new_pad & BUTTON_CROSS) {
						new_pad = 0;
						break;
					}

				}
				termConsole();
			}
		}
// copy from devices

		if ((new_pad & BUTTON_CIRCLE) && game_sel >= 0 && max_menu_list > 0 && mode_list == GAME) {
			if (menu_list[game_sel].flags & 2048) {
				copy_from_bluray();
				continue;
			}

			int n;
			int curr_device = 0;
			char name[1024];
			int dest = 0;

			dialog_ret = 0;
			if (menu_list[game_sel].flags & 1)	// is hdd0
			{

				for (n = 1; n < 11; n++) {
					dialog_ret = 0;

					if ((fdevices >> n) & 1) {

						sprintf(filename, "%s\n\n%s HDD0 %s USB00%c?", menu_list[game_sel].title, text_wantcopy[region],
								text_to[region], 47 + n);

						ret = cellMsgDialogOpen2(type_dialog_yes_no, filename, dialog_fun1, (void *) 0x0000aaaa, NULL);

						wait_dialog();

						if (dialog_ret == 1) {
							curr_device = n;
							break;
						}		// exit
					}
				}

				dest = n;
				if (dialog_ret == 1) {

					sprintf(name, "/dev_usb00%c/%s", 47 + curr_device, GAMES_DIR);
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
					sprintf(name, "/dev_usb00%c/%s", 47 + curr_device, GAMES_DIR);
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
					sprintf(name, "/dev_usb00%c/%s/%s", 47 + curr_device, GAMES_DIR,
							strstr(menu_list[game_sel].path, "/" GAMES_DIR) + sizeof(GAMES_DIR) + 1);
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

				}

			} else if (fdevices & 1) {

				for (n = 1; n < 11; n++) {
					if ((menu_list[game_sel].flags >> n) & 1)
						break;
				}

				if (n == 11)
					continue;

				curr_device = 0;

				dest = 0;
				sprintf(filename, "%s\n\n%s USB00%c %s HDD0?", menu_list[game_sel].title, text_wantcopy[region], 47 + n,
						text_to[region]);
				dialog_ret = 0;
				ret = cellMsgDialogOpen2(type_dialog_yes_no, filename, dialog_fun1, (void *) 0x0000aaaa, NULL);

				wait_dialog();

				if (dialog_ret == 1) {
					char *p = strstr(menu_list[game_sel].path,
									 "/" GAMES_DIR) + sizeof(GAMES_DIR) + 1;

					if (p[0] == '_')
						p++;	// skip special char

					sprintf(name, "/dev_hdd0/game/%s/", hdd_folder);
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
					sprintf(name, "/dev_hdd0/game/%s/%s", hdd_folder, GAMES_DIR);
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
					sprintf(name, "/dev_hdd0/game/%s/%s/%s", hdd_folder, GAMES_DIR, p);
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				}

			}

			if (dialog_ret == 1) {

				// reset to update datas
				reset_game_list(1 << curr_device, game_sel);
				time_start = time(NULL);

				abort_copy = 0;
				initConsole();
				file_counter = 0;
				new_pad = 0;

#if 0
				DPrintf("Starting... \n copy %s\n to %s\n\n", menu_list[game_sel].path, name);
#endif

				if (curr_device != 0)
					copy_mode = 1;	// break files >= 4GB
				else
					copy_mode = 0;

				copy_is_split = 0;

				my_game_copy((char *)
							 menu_list[game_sel].path, (char *) name);

				cellGcmSetClearSurface(gCellGcmCurrentContext,
									   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
									   CELL_GCM_CLEAR_A);

				int seconds = (int) (time(NULL) - time_start);
				int vflip = 0;

				if (copy_is_split && !abort_copy) {

					if (dest == 0) {
						char *p = strstr(menu_list[game_sel].path,
										 "/" GAMES_DIR) + sizeof(GAMES_DIR) + 1;
						if (p[0] == '_')
							p++;	// skip special char

						sprintf(filename, "/dev_hdd0/game/%s/%s/_%s", hdd_folder, GAMES_DIR,
								strstr(menu_list[game_sel].path, "/" GAMES_DIR) + sizeof(GAMES_DIR) + 1);
					} else {
						sprintf(filename, "/dev_usb00%c/%s/_%s", 47 + dest, GAMES_DIR,
								strstr(menu_list[game_sel].path, "/" GAMES_DIR) + sizeof(GAMES_DIR) + 1);
					}

					// try rename
					ret = rename(name, filename);

					if (dest == 0)
						sprintf(filename, "%s\n\nSplit game copied in HDD0 (non bootable)", menu_list[game_sel].title);
					else
						sprintf(filename, "%s\n\nSplit game copied in USB00%c (non bootable)",
								menu_list[game_sel].title, 47 + curr_device);

					dialog_ret = 0;

					ret = cellMsgDialogOpen2(type_dialog_ok, filename, dialog_fun2, (void *) 0x0000aaab, NULL);

					wait_dialog();

				}

				while (1) {
					if (abort_copy)
						sprintf(string1, "Aborted!!!  Time: %2.2i:%2.2i:%2.2i\n", seconds / 3600, (seconds / 60) % 60,
								seconds % 60);
					else {

						sprintf(string1, "Done! Files Copied: %i Time: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", file_counter,
								seconds / 3600, (seconds / 60) % 60, seconds % 60, ((double)
																					global_device_bytes)
								/ (1024.0 * 1024. * 1024.0));
					}

					cellGcmSetClearSurface(gCellGcmCurrentContext,
										   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
										   CELL_GCM_CLEAR_A);

					draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x000000ff);

					cellDbgFontPrintf(0.07f, 0.07f, 1.2f, 0xffffffff, string1);

					if (vflip & 32)
						cellDbgFontPrintf(0.5f - 0.15f, 1.0f - 0.07 * 2.0f, 1.2f, 0xffffffff, "Press X to Exit");

					vflip++;

					cellDbgFontDrawGcm();

					flip();

					pad_read();
					if (new_pad & BUTTON_CROSS) {
						new_pad = 0;
						break;
					}

				}

				if (abort_copy) {
					if (dest == 0)
						sprintf(filename, "%s\n\n%s HDD0?", menu_list[game_sel].title, text_delfailed[region]);
					else
						sprintf(filename, "%s\n\n%s USB00%c?", menu_list[game_sel].title, text_delfailed[region],
								47 + dest);

					dialog_ret = 0;
					ret = cellMsgDialogOpen2(type_dialog_yes_no, filename, dialog_fun1, (void *) 0x0000aaaa, NULL);

					wait_dialog();

					if (dialog_ret == 1) {

						abort_copy = 0;
						time_start = time(NULL);
						file_counter = 0;

						my_game_delete((char *)
									   name);

						rmdir((char *) name);	// delete this folder

						game_sel = 0;

					} else {
						if (dest == 0) {
							char *p = strstr(menu_list[game_sel].path,
											 "/" GAMES_DIR)
								+ sizeof(GAMES_DIR) + 1;
							if (p[0] == '_')
								p++;	// skip special char

							sprintf(filename, "/dev_hdd0/game/%s/%s/_%s", hdd_folder, GAMES_DIR, p);
						} else {
							sprintf(filename, "/dev_usb00%c/%s/_%s", 47 + dest, GAMES_DIR,
									strstr(menu_list[game_sel].path, "/" GAMES_DIR)
									+ sizeof(GAMES_DIR)
									+ 1);
						}

						ret = rename(name, filename);
						//
					}
				}

				game_sel = 0;
				termConsole();
			}

		}
// copy from bluray

		if ((new_pad & BUTTON_SQUARE) && mode_list == GAME) {
			dialog_ret = 0;
			ret =
				cellMsgDialogOpen2(type_dialog_yes_no, text_cover_msg[region], dialog_fun1, (void *) 0x0000aaaa, NULL);
			wait_dialog();

			if (dialog_ret == 1) {
				int n;
				snprintf(filename, sizeof(filename), "/dev_hdd0/%s", COVERS_DIR);
				mkdir(filename, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				for (n = 0; n < max_menu_list; n++) {
					struct stat s;
					snprintf(filename, sizeof(filename), "/dev_hdd0/%s/%s.PNG", COVERS_DIR, menu_list[n].title_id);
					if (stat(filename, &s) == 0)
						continue;

					download_cover(menu_list[n].title_id, filename);
					// reset to update datas
					reset_game_list(1, 0);
				}
			}
		}
		if ((new_pad & BUTTON_L1) && mode_list == GAME) {
			if (payload_type == 0) {
				patchmode = patchmode == 2 ? 0 : 2;	// toggle patch mode
			} else if (payload_type == 1) {
				disc_less ^= true;
			}
		}
		if ((new_pad & BUTTON_L2) && mode_list == GAME) {
			direct_boot ^= true;
		}
		if (new_pad & BUTTON_R1) {
			if (ftp_flags & 2)
				ftp_off();
			else
				ftp_on();
		}
		if (new_pad & BUTTON_CROSS && game_sel >= 0 && *max_list > 0) {

			if (mode_list == GAME && (menu_list[game_sel].flags & 2048)) {
				flip();
				quit();
			}

			if (mode_list == HOMEBREW) {
				int prio = 1001;
				uint64_t flags = SYS_PROCESS_PRIMARY_STACK_SIZE_64K;

				cleanup();

				sprintf(filename, "%s/EBOOT.BIN", menu_homebrew_list[game_sel].path);

				flip();

// SHOULD DETERMINE USB #?

				restorecall36((char *) "/dev_usb000");	// restore
				ret = unload_modules();

				sys_game_process_exitspawn(filename, NULL, NULL, 0, 0, prio, flags);
				exit(0);
			}

			if (game_sel >= 0 && max_menu_list > 0) {
				if (menu_list[game_sel].title[0] == '_') {
					sprintf(filename, "%s\n\n%s", menu_list[game_sel].title, text_nosplit[region]);

					dialog_ret = 0;
					ret = cellMsgDialogOpen2(type_dialog_ok, filename, dialog_fun2, (void *) 0x0000aaab, NULL);
					wait_dialog();
				} else {
					struct stat s;

					cleanup();

					// Check if game dir permission is 0700 but only on internal HDD
					if ((menu_list[game_sel].flags & 1)
						&& (stat(menu_list[game_sel].path, &s) < 0
							|| (s.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != (S_IRWXU | S_IRWXG | S_IRWXO))) {
						sprintf(string1, "%s\n\n%s\n\n%s", menu_list[game_sel].title, text_notfound[region],
								text_fix_permission[region]);
						dialog_ret = 0;
						ret = cellMsgDialogOpen2(type_dialog_yes_no, string1, dialog_fun1, (void *) 0x0000aaaa, NULL);
						wait_dialog();
						if (dialog_ret == 1) {
							sprintf(filename, "%s", menu_list[game_sel].path);
							fix_perm_recursive(filename);
						}
					}
					snprintf(filename, sizeof(filename), "%s/PS3_GAME/PARAM.SFO", menu_list[game_sel].path);
					change_param_sfo_version(filename);
					sprintf(filename, "%s/PS3_GAME/USRDIR/EBOOT.BIN", menu_list[game_sel].path);
					if (payload_type == 0)
						set_hermes_mode(patchmode);
					syscall36(menu_list[game_sel].path);
					if (direct_boot) {
						ret = unload_modules();
						sys_game_process_exitspawn2(filename, NULL, NULL, NULL, 0, 3071,
													SYS_PROCESS_PRIMARY_STACK_SIZE_1M);
						exit(0);
						break;
					}
					ret = unload_modules();
					exit(0);
					break;
				}
			}

		}

		cellGcmSetClearSurface(gCellGcmCurrentContext,
							   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
							   CELL_GCM_CLEAR_A);

		setRenderColor();
		// square for screen
		set_texture(text_bg, DISPLAY_WIDTH, DISPLAY_HEIGHT);
		setRenderTexture();
		display_png(0, 0, 1920, 1080, 1920, 1080);

		if (game_sel >= 0 && *max_list > 0) {
			int dispy = 152;

			// calculate highlight position

			if (game_sel > 15) {
				dispy = 881;
			} else {
				dispy = 150 + (49 * game_sel - 1);
			}

			if (png_w == 320 && png_h == 176) {
				set_texture(text_bmp, DISPLAY_WIDTH, DISPLAY_HEIGHT);
				setRenderTexture();
				display_png(1446, 140, 320, 176, 320, 176);
			} else if (png_w != 0 && png_h != 0) {
				set_texture(text_bmp, DISPLAY_WIDTH, DISPLAY_HEIGHT);
				setRenderTexture();
				display_png(1446, 140, 260, 300, png_w, png_h);
			}

			set_texture(text_h, DISPLAY_WIDTH, DISPLAY_HEIGHT);
			setRenderTexture();
			display_png(105, dispy, 1315, 49, 1315, 49);

		}

		if (mode_list == GAME) {

			if (game_sel >= 0 && max_menu_list > 0) {
				draw_list(menu_list, max_menu_list, game_sel | (0x10000 * ((menu_list[0].flags & 2048) != 0)));
				cellDbgFontPrintf(0.775, 0.10, 0.8, 0xff00ffff, menu_list[game_sel].title_id);
			}
			//else cellDbgFontPrintf( 0.08f, 0.1f, 1.2f, 0xffffffff, "Put games from BR-DISC");
		} else {
			if (game_sel >= 0 && max_menu_homebrew_list > 0)
				draw_list(menu_homebrew_list, max_menu_homebrew_list, game_sel);
			//else cellDbgFontPrintf( 0.08f, 0.1f, 1.2f, 0xffffffff, "Put the homebrew in USBXXX:/homebrew");
		}

		if (mode_list == GAME)
			draw_device_list((fdevices |
							  ((game_sel >= 0
								&& max_menu_list > 0) ? (menu_list[game_sel].flags << 16) : 0)),
							 payload_type == 1 ? disc_less : !patchmode, payload_type, direct_boot, ftp_flags & 2);
		else
			draw_device_list((fdevices | ((game_sel >= 0 && max_menu_homebrew_list > 0)
										  ? (menu_homebrew_list[game_sel].flags << 16) | (1U << 31)
										  : 1U << 31)), payload_type == 1 ? disc_less : !patchmode, payload_type,
							 direct_boot, ftp_flags & 2);

		cellDbgFontDrawGcm();

		flip();
		cellSysutilCheckCallback();
	}

	quit();
}

/* vim: set ts=4 sw=4 sts=4 tw=120 */
