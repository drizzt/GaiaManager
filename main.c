
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
#include <stddef.h>
#include <sys/stat.h>
#include <sys/process.h>
#include <sys/memory.h>
#include <sys/timer.h>
#include <sys/types.h>
#include <sys/return_code.h>

#include <cell/gcm.h>
#include <cell/pad.h>
#include <cell/keyboard.h>
#include <cell/sysmodule.h>
#include <cell/dbgfont.h>
#include <cell/codec/pngdec.h>
#include <cell/cell_fs.h>

#include <sysutil/sysutil_sysparam.h>
#include <sysutil/sysutil_discgame.h>
#include <sysutil/sysutil_msgdialog.h>
#include <sysutil/sysutil_oskdialog.h>
#include <cell/font.h>
#include <sys/ppu_thread.h>

#include <netex/net.h>
#include <netex/libnetctl.h>
#include <cell/fontFT.h>

#include <libftp.h>

#include "at3plus.h"
#include "graphics.h"
#include "syscall8.h"

sys_ppu_thread_t	soundThread = 0;
volatile int		isRunning = 0;

#define MAX_LIST 512
#define INPUT_FILE  "/dev_bdvd/PS3_GAME/SND0.AT3"
#define THREAD_NUM 4

enum BmModes {
	GAME = 0,
	HOMEBREW = 1
};

static BGMArg bgmArg;

static int fm = -1;


static char hdd_folder[64]="ASDFGHJKLMN"; // folder for games (deafult string is changed the first time it is loaded
static char hdd_folder_home[64]=FOLDER_NAME; // folder for homebrew
// 				Jap, 		English, 	French, 	Spanish, 	German, 	Italian, 	Dutch, 		Portugues, 	Russian, Korean, Chinese, Chinese, Finnish, Swedish, Danish, Norwegian
static const char text_delfailed[][128] = {	"Delete failed dump in",	"Delete failed dump in",	"Delete failed dump in",	"Borrar dump fallido en","Loschen fehlgeschlagener Kopie in",	"Delete failed dump in",	"Delete failed dump in",	"Apagar descarga falhada no",	"Delete failed dump in","Delete failed dump in","Delete failed dump in","Delete failed dump in","Delete failed dump in","Raderingen misslyckades, information i","Delete failed dump in","Delete failed dump in"};
static const char text_nosplit[][128] = {"You cannot launch split games","You cannot launch split games","You cannot launch split games","No puedes jugar a juegos divididos","Sie konnen keine geteilten Spiele starten","Non puoi avviare giochi divisi","You cannot launch split games","Nao pode iniciar jogos divididos","You cannot launch split games","You cannot launch split games","You cannot launch split games","You cannot launch split games","You cannot launch split games","Du kan inte kora delade spel","You cannot launch split games","You cannot launch split games"};

static const char text_wantcopy[][128] = {"Want to copy from","Want to copy from","Want to copy from","Quieres copiar desde","Wollen Sie von","Vuoi copiare da","Want to copy from","Quer copiar de","Want to copy from","Want to copy from","Want to copy from","Want to copy from","Want to copy from","Vill du kopiera fran","Want to copy from","Want to copy from"};
static const char text_to[][12] = {"to","to","to","a","zu kopieren","a","to","para","to","to","to","to","to","till","to","to"};


static const char text_eboot[][96] = {"EBOOT.BIN has been successfully updated","EBOOT.BIN has been successfully updated","EBOOT.BIN has been successfully updated","EBOOT.BIN ha sido parcheado.","EBOOT.BIN wurde erfolgreich aktualisiert","EBOOT.BIN e stato aggiornato con successo","EBOOT.BIN has been successfully updated","EBOOT.BIN foi actualizado com sucesso","EBOOT.BIN has been successfully updated","EBOOT.BIN has been successfully updated","EBOOT.BIN has been successfully updated","EBOOT.BIN has been successfully updated","EBOOT.BIN has been successfully updated","Eboot.bin har uppdaterats utan problem.","EBOOT.BIN has been successfully updated","EBOOT.BIN has been successfully updated"};
static const char text_launcher[][96] = {"You can launch this utility pressing SELECT+START the next time","You can launch this utility pressing SELECT+START the next time","You can launch this utility pressing SELECT+START the next time","Puedes ejecutar esta utilidad presionando SELECT+START la proxima vez.","Sie konnen das Spiel ab jetzt mit SELECT+START starten","Puoi lanciare questa utility premendo SELECT+START la prossima volta","You can launch this utility pressing SELECT+START the next time","Pode iniciar este utilitário pressionando SELECT+START na proxima vez","You can launch this utility pressing SELECT+START the next time","You can launch this utility pressing SELECT+START the next time","You can launch this utility pressing SELECT+START the next time","You can launch this utility pressing SELECT+START the next time","You can launch this utility pressing SELECT+START the next time","Du kan nu starta detta program genom att trycka SELECT+START nasta gang.","You can launch this utility pressing SELECT+START the next time","You can launch this utility pressing SELECT+START the next time"};

static const char text_notfound[][32] = {"EBOOT.BIN not found","EBOOT.BIN not found","EBOOT.BIN not found","EBOOT.BIN no encontrado","EBOOT.BIN wurde nicht gefunden","EBOOT.BIN non trovato","EBOOT.BIN not found","EBOOT.BIN não encontrado","EBOOT.BIN not found","EBOOT.BIN not found","EBOOT.BIN not found","EBOOT.BIN not found","EBOOT.BIN not found","EBOOT.BIN hittades inte","EBOOT.BIN not found","EBOOT.BIN not found"};

static const char text_wantexit[][32] = {"Want to exit?","Want to exit?","Want to exit?","Quieres salir?","Wollen Sie beenden?","Vuoi uscire?","Want to exit?","Quer sair?","Want to exit?","Want to exit?","Want to exit?","Want to exit?","Want to exit?","Vill du avsluta?","Want to exit?","Want to exit?"};
static const char text_wantdel[][32] = {"Want to delete from","Want to delete from","Want to delete from","Quieres borrar desde","Wollen Sie von loschen","Want to delete from","Want to delete from","Quer apagar de","Want to delete from","Want to delete from","Want to delete from","Want to delete from","Want to delete from","Vill du radera fran","Want to delete from","Want to delete from"};

static const char text_wantuse[][32] = {"Want to use","Want to use","Want to use","Quieres usar","Mochten Sie","Vuoi usare","Want to use","Quer usar","Want to use","Want to use","Want to use","Want to use","Want to use","Vill du anvanda","Want to use","Want to use"};
static const char text_toinstall[][64] = {"to install the game?","to install the game?","to install the game?","para instalar el juego?","fur die Spiele-Installation verwenden?","per installare il gioco?","to install the game?","para instalar o jogo?","to install the game?","to install the game?","to install the game?","to install the game?","to install the game?","till att installera spel?","to install the game?","to install the game?"};

static char soundfile[512];
static t_menu_list menu_list[MAX_LIST];
static int max_menu_list=0;

static int hermes = 0;
static int region = 1;
static int direct_boot = 0;

static uint64_t mem_orig = 0x386000014E800020ULL;
static uint64_t mem_patched = 0xE92296887C0802A6ULL; 
	
static t_menu_list menu_homebrew_list[MAX_LIST];
static int max_menu_homebrew_list=0;
static int *max_list=&max_menu_list;

static enum BmModes mode_list=GAME;

static int game_sel=0;

#define	BUTTON_SELECT		(1<<0)
#define	BUTTON_L3			(1<<1)
#define	BUTTON_R3			(1<<2)
#define	BUTTON_START		(1<<3)
#define	BUTTON_UP			(1<<4)
#define	BUTTON_RIGHT		(1<<5)
#define	BUTTON_DOWN			(1<<6)
#define	BUTTON_LEFT			(1<<7)
#define	BUTTON_L2			(1<<8)
#define	BUTTON_R2			(1<<9)
#define	BUTTON_L1			(1<<10)
#define	BUTTON_R1			(1<<11)
#define	BUTTON_TRIANGLE		(1<<12)
#define	BUTTON_CIRCLE		(1<<13)
#define	BUTTON_CROSS		(1<<14)
#define	BUTTON_SQUARE		(1<<15)




static unsigned cmd_pad= 0;

static void *host_addr;

static int up_count=0, down_count=0, left_count=0, right_count=0;

static u32 new_pad=0,old_pad=0;

static int load_libfont_module(void)
{
	int ret;
	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FONT );
	if ( ret == CELL_OK ) {
		ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FREETYPE );
		if ( ret == CELL_OK ) {
			ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FONTFT );
			if ( ret == CELL_OK ) {
			return ret; // Success
			}
			// Error handling as follows (Unload all loads)
			cellSysmoduleUnloadModule( CELL_SYSMODULE_FREETYPE );
		}
		cellSysmoduleUnloadModule( CELL_SYSMODULE_FONT );
	}
	return ret; // Error end
}


static int pad_read( void )
{
	int ret;
	
	u32		padd;

	CellPadData databuf;
	CellPadInfo infobuf;
	static u32 old_info = 0;
	
	
	cmd_pad= 0;

	ret = cellPadGetInfo( &infobuf );
	if ( ret != 0 ) 
		{
		old_pad=new_pad = 0;
		return 1;
		}

	if ( infobuf.status[0] == CELL_PAD_STATUS_DISCONNECTED ) 
		{
		old_pad=new_pad = 0;
		return 1;
		}

	if((infobuf.info & CELL_PAD_INFO_INTERCEPTED) && (!(old_info & CELL_PAD_INFO_INTERCEPTED)))
		{
		old_info = infobuf.info;
		}
	else 
		if((!(infobuf.info & CELL_PAD_INFO_INTERCEPTED)) && (old_info & CELL_PAD_INFO_INTERCEPTED))
			{
			old_info = infobuf.info;
			old_pad=new_pad = 0;
			return 1;
			}

	ret = cellPadGetData( 0, &databuf );

	if (ret != CELL_PAD_OK) 
		{
		old_pad=new_pad = 0;
		return 1;
		}

	if (databuf.len == 0) 
		{
		new_pad = 0;
		return 1;
		}

	padd = ( databuf.button[2] | ( databuf.button[3] << 8 ) );

	new_pad=padd & (~old_pad);
	old_pad= padd;

	return 1;
}

/****************************************************/
/* FTP SECTION                                      */
/****************************************************/

static int ftp_flags=0;

static void ftp_handler(CellFtpServiceEvent event, void * data __attribute__((unused)), size_t datalen __attribute__((unused)))
{

//DPrintf("Event %i %x %i\n", event, (u32) data, datalen);

	switch(event)
	{
	case 0:
	
	break;
	case CELL_FTP_SERVICE_EVENT_SHUTDOWN:
	case CELL_FTP_SERVICE_EVENT_FATAL:
	case CELL_FTP_SERVICE_EVENT_STOPPED:

		ftp_flags|=4;

		break;
	default:
		break;
	}
}

static void ftp_on(void)
{
	int ret;
		
	if(!(ftp_flags & 1))
		{
		ret = sys_net_initialize_network();
		if (ret < 0) return;
		ftp_flags|=1;
		}

	if(!(ftp_flags & 2) && cellFtpServiceRegisterHandler(ftp_handler)>=0)
		{
		ftp_flags|=2;
		cellFtpServiceStart();
		}
}

static void ftp_off(void)
{

	if(ftp_flags & 2)
		{
		uint64_t result;

	    /*if(!(ftp_flags & 4)) */
		cellFtpServiceStop(&result);

		cellFtpServiceUnregisterHandler();

		ftp_flags &= ~6;
		}

	if(ftp_flags & 1)
			{
			sys_net_finalize_network();
			ftp_flags &= ~1;
			}
}

/****************************************************/
/* MODULES SECTION                                  */
/****************************************************/

static int unload_mod=0;

static int load_modules(void)
{
	int ret;

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	if (ret != CELL_OK) return ret;
	else unload_mod|=1;
	
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_PNGDEC);
	if(ret != CELL_OK) return ret;
	else unload_mod|=2;
		
	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_IO );
	if (ret != CELL_OK) return ret;
	else unload_mod|=4;
		
	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_GCM_SYS );
	if (ret != CELL_OK) return ret;
	else unload_mod|=8;

	host_addr = memalign(0x100000, 0x100000);
	if(cellGcmInit(0x10000, 0x100000, host_addr) != CELL_OK) return -1;
	
	if(initDisplay()!=0) return -1;

	initShader();

	setDrawEnv();
		
	if(setRenderObject()) return -1;

	ret = cellPadInit(1);
	if (ret != 0) return ret;
	
	setRenderTarget();

	initFont();

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
	if (ret < 0) return ret; else unload_mod|=16;

	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_ATRAC3PLUS );
	if (ret != CELL_OK) return ret;
	else unload_mod|=32;

	return ret;
}


static int unload_modules(void)
{

	ftp_off();

	cellPadEnd();
	
	termFont();

	free(host_addr);

	if(unload_mod & 32) cellSysmoduleUnloadModule(CELL_SYSMODULE_ATRAC3PLUS);

	if(unload_mod & 16) cellSysmoduleUnloadModule(CELL_SYSMODULE_NET);

	if(unload_mod & 8) cellSysmoduleUnloadModule(CELL_SYSMODULE_GCM_SYS);

	if(unload_mod & 4) cellSysmoduleUnloadModule(CELL_SYSMODULE_IO);

	if(unload_mod & 2) cellSysmoduleUnloadModule(CELL_SYSMODULE_PNGDEC);
	
	if(unload_mod & 1) cellSysmoduleUnloadModule( CELL_SYSMODULE_FS );
	

	return 0;
}




SYS_PROCESS_PARAM(1001, 0x10000)


volatile int no_video=0;

volatile int dialog_ret=0;

u32 type_dialog_yes_no = CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BG_VISIBLE | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO
					   | CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON | CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NO;


u32 type_dialog_ok = CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BG_VISIBLE | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK
				   | CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF| CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_OK;

static void dialog_fun1( int button_type, void *userData __attribute__((unused)))
{
	
	switch ( button_type ) {
	case CELL_MSGDIALOG_BUTTON_YES:
		dialog_ret=1;
		break;
	case CELL_MSGDIALOG_BUTTON_NO:
	case CELL_MSGDIALOG_BUTTON_ESCAPE:
	case CELL_MSGDIALOG_BUTTON_NONE:
		dialog_ret=2;
		break;
	default:
		break;
	}
}
static void dialog_fun2( int button_type, void *userData __attribute__((unused)))
{
	
	switch ( button_type ) {
		case CELL_MSGDIALOG_BUTTON_OK:
		case CELL_MSGDIALOG_BUTTON_ESCAPE:
		case CELL_MSGDIALOG_BUTTON_NONE:

		dialog_ret=1;
		break;
	default:
		break;
	}
}

static void wait_dialog(void)
{

	while(!dialog_ret)
		{
		cellSysutilCheckCallback();flip();
		}

	cellMsgDialogAbort();
	setRenderColor();
	sys_timer_usleep(100000);
					
}



/****************************************************/
/* PNG SECTION                                      */
/****************************************************/


typedef struct CtrlMallocArg
{
	u32 mallocCallCounts;

} CtrlMallocArg;


typedef struct CtrlFreeArg
{
	u32 freeCallCounts;

} CtrlFreeArg;


static void *png_malloc(u32 size, void * a)
{
    CtrlMallocArg *arg;
    
	arg = (CtrlMallocArg *) a;
    
	arg->mallocCallCounts++;

	return malloc(size);
}


static int png_free(void *ptr, void * a)
{
    CtrlFreeArg *arg;
  
	arg = (CtrlFreeArg *) a;
    
	arg->freeCallCounts++;
	
	free(ptr);
	
	return 0;
}

static int png_out_mapmem(u8 *buffer, size_t buf_size)
{
	int ret;
	u32 offset;

	ret = cellGcmMapMainMemory(buffer, buf_size, &offset);

	if(CELL_OK != ret ) return ret;

	return 0;

}

int png_w=0, png_h=0;

static void playBootSound(uint64_t ui __attribute__((unused)))
{
	int32_t status;

//	stop_atrac3();

//	delete_atrac3plus(&bgmArg);

	cellFsClose(fm);
//	status = cellFsOpen("/dev_hdd0/game/OMAN46756/USRDIR/BOOT.AT3", CELL_FS_O_RDONLY, &fm, NULL, 0);
	status = cellFsOpen(soundfile, CELL_FS_O_RDONLY, &fm, NULL, 0);
	bgmArg.fd = fm;
	status = init_atrac3plus(&bgmArg);
	play_atrac3plus((uintptr_t)&bgmArg);
	cellFsClose(fm);
	sys_ppu_thread_exit(0);	

}


static int load_png_texture(u8 *data, char *name)
{
	int  ret_file, ret, ok=-1;
	

	CellPngDecMainHandle        mHandle;
	CellPngDecSubHandle         sHandle;

	CellPngDecThreadInParam 	InParam;
	CellPngDecThreadOutParam 	OutParam;

	CellPngDecSrc 		        src; 
	CellPngDecOpnInfo 	        opnInfo;
	CellPngDecInfo 		        info;
	CellPngDecDataOutInfo 	    dOutInfo;
	CellPngDecDataCtrlParam     dCtrlParam;
	CellPngDecInParam 	        inParam;
	CellPngDecOutParam 	        outParam;

	CtrlMallocArg               MallocArg;
	CtrlFreeArg                 FreeArg;

	int ret_png=-1;

	InParam.spuThreadEnable   = CELL_PNGDEC_SPU_THREAD_DISABLE;
	InParam.ppuThreadPriority = 512;
	InParam.spuThreadPriority = 200;
	InParam.cbCtrlMallocFunc  = png_malloc;
	InParam.cbCtrlMallocArg   = &MallocArg;
	InParam.cbCtrlFreeFunc    = png_free;
	InParam.cbCtrlFreeArg     = &FreeArg;
	

	ret_png= ret= cellPngDecCreate(&mHandle, &InParam, &OutParam);
	
	memset(data, 0xff, (DISPLAY_WIDTH * DISPLAY_HEIGHT * 4));

	png_w= png_h= 0;

	if(ret_png == CELL_OK)
		{
		
			memset(&src, 0, sizeof(CellPngDecSrc));
			src.srcSelect     = CELL_PNGDEC_FILE;
			src.fileName      = name;

			src.spuThreadEnable  = CELL_PNGDEC_SPU_THREAD_DISABLE;
			
			ret_file=ret = cellPngDecOpen(mHandle, &sHandle, &src, &opnInfo);
			
			if(ret == CELL_OK)
				{
				ret = cellPngDecReadHeader(mHandle, sHandle, &info);
				}

			if(ret == CELL_OK)
				{	
				inParam.commandPtr        = NULL;
				inParam.outputMode        = CELL_PNGDEC_TOP_TO_BOTTOM;
				inParam.outputColorSpace  = CELL_PNGDEC_RGBA;
				inParam.outputBitDepth    = 8;
				inParam.outputPackFlag    = CELL_PNGDEC_1BYTE_PER_1PIXEL;
				
				if((info.colorSpace == CELL_PNGDEC_GRAYSCALE_ALPHA) || (info.colorSpace == CELL_PNGDEC_RGBA) || (info.chunkInformation & 0x10))
					inParam.outputAlphaSelect = CELL_PNGDEC_STREAM_ALPHA;
				else
					inParam.outputAlphaSelect = CELL_PNGDEC_FIX_ALPHA;
				
				inParam.outputColorAlpha  = 0xff;

				ret = cellPngDecSetParameter(mHandle, sHandle, &inParam, &outParam);
				}

			if(ret == CELL_OK)
				{
					dCtrlParam.outputBytesPerLine = DISPLAY_WIDTH* 4;  
					ret = cellPngDecDecodeData(mHandle, sHandle, data, &dCtrlParam, &dOutInfo);
					//sys_timer_usleep(300);

					if((ret == CELL_OK) && (dOutInfo.status == CELL_PNGDEC_DEC_STATUS_FINISH))
						{
						png_w= outParam.outputWidth;
						png_h= outParam.outputHeight;
						ok=0;
						}
				}

			if(ret_file==0)	ret = cellPngDecClose(mHandle, sHandle);
			
			ret = cellPngDecDestroy(mHandle);
			
			}
	
	InParam.spuThreadEnable   = CELL_PNGDEC_SPU_THREAD_DISABLE;


return ok;
}

/****************************************************/
/* syscalls                                         */
/****************************************************/

static void syscall36(char *path)
{
	system_call_1(36, (uint32_t) path);
}


/****************************************************/
/* UTILS                                            */
/****************************************************/

static uint64_t peekq(uint64_t addr)
{
	system_call_1(6,addr);
	return_to_user_prog(uint64_t);
}

static void pokeq(uint64_t addr, uint64_t val)
{
	system_call_2(7, addr, val);
}

static void fix_perm_recursive(const char* start_path)
{
    int dir_fd;
    uint64_t nread;
    char f_name[CELL_FS_MAX_FS_FILE_NAME_LENGTH+1];
    CellFsDirent dir_ent;
    CellFsErrno err;
    if (cellFsOpendir(start_path, &dir_fd) == CELL_FS_SUCCEEDED)
    {
        cellFsChmod(start_path, 0777);
        while (1) {
            err = cellFsReaddir(dir_fd, &dir_ent, &nread);
            if (nread != 0) {
                if (!strcmp(dir_ent.d_name, ".") || !strcmp(dir_ent.d_name, ".."))
                    continue;

                sprintf(f_name, "%s/%s", start_path, dir_ent.d_name);

                if (dir_ent.d_type == CELL_FS_TYPE_DIRECTORY)
                {
                    cellFsChmod(f_name, 0777);
                    fix_perm_recursive(f_name);
                }
                else if (dir_ent.d_type == CELL_FS_TYPE_REGULAR)
                {
                    cellFsChmod(f_name, 0666);
                }
            } else {
                break;
            }
        } 
        err = cellFsClosedir(dir_fd);
    }
}

static char filename[1024];

static int parse_ps3_disc(char *path, char * id)
{
	FILE *fp;
	int n;
	
	fp = fopen(path, "rb");
	if (fp != NULL)
		{
		unsigned len;
		unsigned char *mem=NULL;

		fseek(fp, 0, SEEK_END);
		len=ftell(fp);

		mem= (unsigned char *) malloc(len+16);
		if(!mem) {fclose(fp);return -2;}

		memset(mem, 0, len+16);

		fseek(fp, 0, SEEK_SET);

		fread((void *) mem, len, 1, fp);

		fclose(fp);

		for(n=0x20;n<0x200;n+=0x20)
			{
			if(!strcmp((char *) &mem[n], "TITLE_ID"))
				{
				n= (mem[n+0x12]<<8) | mem[n+0x13];
				memcpy(id, &mem[n], 16);

				return 0;
				}
			}
		}

	return -1;

}

// liomajor fix

static int parse_param_sfo_id(char * file, char *title_name)
{
	FILE *fp;
	
	fp = fopen(file, "rb");
	if (fp != NULL)
		{
		unsigned len, pos, str;
		unsigned char *mem=NULL;

		
		fseek(fp, 0, SEEK_END);
		len=ftell(fp);

		mem= (unsigned char *) malloc(len+16);
		if(!mem) {fclose(fp);return -2;}

		memset(mem, 0, len+16);

		fseek(fp, 0, SEEK_SET);
		fread((void *) mem,len, 1, fp);

		fclose(fp); 

		str= (mem[8]+(mem[9]<<8));
		pos=(mem[0xc]+(mem[0xd]<<8));

		int indx=0;

		while(str<len)
			{
			if(mem[str]==0) break;
			
			if(!strcmp((char *) &mem[str], "TITLE_ID"))
				{
				strncpy(title_name, (char *) &mem[pos], 63);
				free(mem);
				return 0;
				}
			while(mem[str]) str++;str++;
			pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8));
			indx+=16;
			}
		if(mem) free(mem);
		}

	
	return -1;

}

static int parse_param_sfo(char * file, char *title_name)
{
	FILE *fp;
	
	fp = fopen(file, "rb");
	if (fp != NULL)
		{
		unsigned len, pos, str;
		unsigned char *mem=NULL;

		
		fseek(fp, 0, SEEK_END);
		len=ftell(fp);

		mem= (unsigned char *) malloc(len+16);
		if(!mem) {fclose(fp);return -2;}

		memset(mem, 0, len+16);

		fseek(fp, 0, SEEK_SET);
		fread((void *) mem,len, 1, fp);

		fclose(fp); 

		str= (mem[8]+(mem[9]<<8));
		pos=(mem[0xc]+(mem[0xd]<<8));

		int indx=0;
// liomajor fix
		while(str<len)
			{
			if(mem[str]==0) break;
			
			if(!strcmp((char *) &mem[str], "TITLE"))
				{
				strncpy(title_name, (char *) &mem[pos], 63);
				free(mem);
				return 0;
				}
			while(mem[str]) str++;str++;
			pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8));
			indx+=16;
			}
		if(mem) free(mem);
		}

	
	return -1;

}


static void sort_entries(t_menu_list *list, int *max)
{
	int n,m;
	int fi= (*max);
	

	for(n=0; n< (fi -1);n++)
		for(m=n+1; m< fi ;m++)
			{
			if((strcasecmp(list[n].title, list[m].title)>0  && ((list[n].flags | list[m].flags) & 2048)==0) || 
				((list[m].flags & 2048) && n==0))
				{
				t_menu_list swap;
					swap=list[n];list[n]=list[m];list[m]=swap;
				}
			}
}

static void delete_entries(t_menu_list *list, int *max, u32 flag)
{
	int n;

	n=0;

	while(n<(*max) )
		{
		if(list[n].flags & flag)
			{
			if((*max) >1)
				{
				list[n].flags=0;
				list[n]=list[(*max) -1];
				(*max) --;
				}
			else  {if((*max) == 1)(*max) --; break;}
			
			}
		else n++;

		}
}

static void fill_entries_from_device(char *path, t_menu_list *list, int *max, u32 flag, int sel)
{
	DIR  *dir;
	char file[1024];

	
	delete_entries(list, max, flag);

	if((*max) <0) *max =0;


	dir=opendir (path);
	if(!dir) return;

	while(1)
		{
		struct dirent *entry=readdir (dir);
		
			
		if(!entry) break;
		if(entry->d_name[0]=='.') continue;

		if(!(entry->d_type & DT_DIR)) continue;
		
		list[*max ].flags=flag;

		strncpy(list[*max ].title, entry->d_name, 63);
		list[*max ].title[63]=0;

		sprintf(list[*max ].path, "%s/%s", path, entry->d_name);

		if(sel==0)
			{
			// read name in PARAM.SFO
			sprintf(file, "%s/PS3_GAME/PARAM.SFO", list[*max ].path);

			parse_param_sfo(file, list[*max ].title+1*(list[*max ].title[0]=='_')); // move +1 with '_' 
			list[*max ].title[63]=0;
			parse_param_sfo_id(file, list[*max ].title_id);
			list[*max ].title_id[63]=0;
			}
		else
			{
			struct stat s;
			sprintf(file, "%s/EBOOT.BIN", list[*max ].path);
			if(stat(file, &s)<0) continue;
			}

		(*max) ++;

		if(*max >=MAX_LIST) break;

		}
	
	closedir (dir);
	
}


/****************************************************/
/* FILE UTILS                                       */
/****************************************************/

int file_counter=0; // to count files

time_t time_start; // time counter init

char string1[256];

int abort_copy=0; // abort process


int copy_mode=0; // 0- normal 1-> pack files >= 4GB

int copy_is_split=0; // return 1 if files is split

int64_t global_device_bytes=0;


#define MAX_FAST_FILES 32

typedef struct _t_fast_files
{
	int64_t readed; // global bytes readed
	int64_t writed; // global bytes writed
	int64_t off_readed; // offset correction for bigfiles_mode == 2  (joining)
	int64_t len;    // global len of the file (value increased in the case of bigfiles_ mode == 2)

	int giga_counter; // counter for split files to 1GB for bigfiles_mode == 1 (split)
	u32 fl; // operation control
	int bigfile_mode;
	int pos_path; // filename position used in bigfiles

	char pathr[1024]; // read path 
	char pathw[1024]; // write path


	int use_doublebuffer; // if files >= 4MB use_doblebuffer =1;

	void *mem; // buffer for read/write files ( x2 if use_doublebuffer is fixed)
	int size_mem; // size of the buffer for read

	int number_frag; // used to count fragments files i bigfile_mode

	CellFsAio t_read;  // used for async read
	CellFsAio t_write; // used for async write

} t_fast_files __attribute__((aligned(8)));

t_fast_files *fast_files=NULL;

int fast_num_files=0;

int fast_used_mem=0;

int current_fast_file_r=0;
int current_fast_file_w=0;

int fast_read=0, fast_writing=0;

int files_opened=0;

static int fast_copy_async(char *pathr, char *pathw, int enable)
{

	fast_num_files=0;

	fast_read=0;
	fast_writing=0;

	fast_used_mem=0;
	files_opened=0;

	current_fast_file_r= current_fast_file_w= 0;

	if(enable)
		{
		if(cellFsAioInit(pathr)!=CELL_FS_SUCCEEDED)  return -1;
		if(cellFsAioInit(pathw)!=CELL_FS_SUCCEEDED)  return -1;

		fast_files= (t_fast_files *) memalign(8, sizeof(t_fast_files)*MAX_FAST_FILES);
		if(!fast_files) return -2;
		return 0;
		}
	else
		{
		if(fast_files) free(fast_files); fast_files=NULL;
		cellFsAioFinish(pathr);
		cellFsAioFinish(pathw);
		}

return 0;

}


static int fast_copy_process(void);

static int fast_copy_add(char *pathr, char *pathw, char *file)
{
	int size_mem;

	int strl= strlen(file);

	struct stat s;
				
				
	if(fast_num_files>=MAX_FAST_FILES || fast_used_mem>=0x800000)
	{
	int ret=fast_copy_process();

		if(ret<0 || abort_copy) return ret;

	}

	if(fast_num_files>= MAX_FAST_FILES) {return -1;}
	
	fast_files[fast_num_files].bigfile_mode=0;

	if(strl>6)
		{
		char *p= file;
		p+= strl-6; // adjust for .666xx
		if(p[0]== '.' && p[1]== '6' && p[2]== '6' && p[3]== '6')
			{  
			if(p[4]!='0' ||  p[5]!='0')  {return 0;} // ignore this files
   			fast_files[fast_num_files].bigfile_mode=2; // joining split files
					
			}
				
		}
	sprintf(fast_files[fast_num_files].pathr, "%s/%s", pathr, file);

	if(stat(fast_files[fast_num_files].pathr, &s)<0) {abort_copy=1;return -1;}

	sprintf(fast_files[fast_num_files].pathw, "%s/%s", pathw, file);

	// zero files
	if((int64_t) s.st_size==0LL)
		{
		int fdw;

		if(cellFsOpen(fast_files[fast_num_files].pathw, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdw, 0,0)!=CELL_FS_SUCCEEDED)
			{
			DPrintf("Error Opening0 (write):\n%s\n\n", fast_files[current_fast_file_r].pathw);
			abort_copy=1;
			return -1;
			}
		cellFsClose(fdw);

		cellFsChmod(fast_files[fast_num_files].pathw, CELL_FS_S_IFMT | 0777);
		DPrintf("Copying %s\nwWrote 0 B\n", fast_files[current_fast_file_r].pathr);
		file_counter++;
		return 0;
		}

	if(fast_files[fast_num_files].bigfile_mode==2)
		{
		fast_files[fast_num_files].pathw[strlen(fast_files[fast_num_files].pathw)-6]=0; // truncate the extension
		fast_files[fast_num_files].pos_path=strlen(fast_files[fast_num_files].pathr)-6;
		fast_files[fast_num_files].pathr[fast_files[fast_num_files].pos_path]=0; // truncate the extension
		}
	
	if(copy_mode==1)
		{
		if(((int64_t) s.st_size)>= 0x100000000LL)
			{
			fast_files[fast_num_files].bigfile_mode=1;
			fast_files[fast_num_files].pos_path= strlen(fast_files[fast_num_files].pathw);
			fast_files[fast_num_files].giga_counter=0;
			
			copy_is_split=1;
			}
		
		}
	

	fast_files[fast_num_files].number_frag=0;
	fast_files[fast_num_files].fl=1;

	fast_files[fast_num_files].len= (int64_t) s.st_size;
	fast_files[fast_num_files].use_doublebuffer=0;
	fast_files[fast_num_files].readed= 0LL;
	fast_files[fast_num_files].writed= 0LL;

	fast_files[fast_num_files].t_read.fd= -1;
	fast_files[fast_num_files].t_write.fd= -1;

	if(((int64_t) s.st_size)>=0x400000LL)
		{
		size_mem= 0x400000;
		fast_files[fast_num_files].use_doublebuffer=1;
		}
	else size_mem= ((int) s.st_size);

	fast_files[fast_num_files].mem= memalign(32, size_mem+size_mem*(fast_files[fast_num_files].use_doublebuffer!=0)+1024);
	fast_files[fast_num_files].size_mem= size_mem;

	if(!fast_files[fast_num_files].mem) {abort_copy=1;return -1;}

	fast_used_mem+= size_mem;

	fast_num_files++;

	return 0;
}

static void fast_func_read(CellFsAio *xaio, CellFsErrno error, int xid __attribute__((unused)), uint64_t size)
{
	t_fast_files* fi = (t_fast_files *) (uint32_t) xaio->user_data;

	if(error!=0 || size!= xaio->size)
		{
		fi->readed=-1;return;
		}
    else
		fi->readed+=(int64_t) size;

	fast_read=0;fi->fl=3;
	
}

static void fast_func_write(CellFsAio *xaio, CellFsErrno error, int xid __attribute__((unused)), uint64_t size)
{
	t_fast_files* fi = (t_fast_files *) (uint32_t) xaio->user_data;

	if(error!=0 || size!= xaio->size)
		{
		fi->writed=-1;
		}
	else
		{

		fi->writed+=(int64_t) size;
		fi->giga_counter+= (int) size;
		global_device_bytes+=(int64_t) size;
		}

	fast_writing=2;
}

static int fast_copy_process()
{

	int n;

	int fdr, fdw;

	static int id_r=-1, id_w=-1;

	int error=0;

	int i_reading=0;

	int64_t write_end=0, write_size=0;

	
	while(current_fast_file_w<fast_num_files || fast_writing)
	{


		if(abort_copy) break;

		
		// open read
		if(current_fast_file_r<fast_num_files && fast_files[current_fast_file_r].fl==1 && !i_reading && !fast_read)
			{
			
				fast_files[current_fast_file_r].readed= 0LL;
				fast_files[current_fast_file_r].writed= 0LL;
				fast_files[current_fast_file_r].off_readed= 0LL;

				fast_files[current_fast_file_r].t_read.fd= -1;
				fast_files[current_fast_file_r].t_write.fd= -1;
				
				if(fast_files[current_fast_file_r].bigfile_mode==1)
					{
					DPrintf("Split file >= 4GB\n %s\n", fast_files[current_fast_file_r].pathr);
					sprintf(&fast_files[current_fast_file_r].pathw[fast_files[current_fast_file_r].pos_path],".666%2.2i",
						fast_files[current_fast_file_r].number_frag);
					}
				
				if(fast_files[current_fast_file_r].bigfile_mode==2)
					{
					DPrintf("Joining file >= 4GB\n %s\n", fast_files[current_fast_file_r].pathw);
					sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path],".666%2.2i",
						fast_files[current_fast_file_r].number_frag);
					}
			
				//DPrintf("Open R: %s\nOpen W: %s, Index %i/%i\n", fast_files[current_fast_file_r].pathr,
				//	fast_files[current_fast_file_r].pathw, current_fast_file_r, fast_num_files);

					
				if(cellFsOpen(fast_files[current_fast_file_r].pathr, CELL_FS_O_RDONLY, &fdr, 0,0)!=CELL_FS_SUCCEEDED) 
					{
					DPrintf("Error Opening (read):\n%s\n\n", fast_files[current_fast_file_r].pathr);
					error=-1;
					break;
					}else files_opened++;
				if(cellFsOpen(fast_files[current_fast_file_r].pathw, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdw, 0,0)!=CELL_FS_SUCCEEDED)
					{
					DPrintf("Error Opening (write):\n%s\n\n", fast_files[current_fast_file_r].pathw);
					error=-2;
					break;
					}else files_opened++;

				if(fast_files[current_fast_file_r].bigfile_mode==0) { }
					//DPrintf("Copying %s\n", fast_files[current_fast_file_r].pathr);
				if(fast_files[current_fast_file_r].bigfile_mode)
					DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_r].number_frag);

				fast_files[current_fast_file_r].t_read.fd= fdr;

				fast_files[current_fast_file_r].t_read.offset= 0LL;
				fast_files[current_fast_file_r].t_read.buf= fast_files[current_fast_file_r].mem;
			
				fast_files[current_fast_file_r].t_read.size=fast_files[current_fast_file_r].len-fast_files[current_fast_file_r].readed;
				if((int64_t) fast_files[current_fast_file_r].t_read.size> fast_files[current_fast_file_r].size_mem)
					fast_files[current_fast_file_r].t_read.size=fast_files[current_fast_file_r].size_mem;

				fast_files[current_fast_file_r].t_read.user_data= (uint32_t )&fast_files[current_fast_file_r];

				fast_files[current_fast_file_r].t_write.fd= fdw;
				fast_files[current_fast_file_r].t_write.user_data= (uint32_t )&fast_files[current_fast_file_r];
				fast_files[current_fast_file_r].t_write.offset= 0LL;
				if(fast_files[current_fast_file_r].use_doublebuffer)
					fast_files[current_fast_file_r].t_write.buf= ((char *) fast_files[current_fast_file_r].mem) + fast_files[current_fast_file_r].size_mem;
				else
					fast_files[current_fast_file_r].t_write.buf= fast_files[current_fast_file_r].mem;

				fast_read=1;fast_files[current_fast_file_r].fl=2;
				if(cellFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read)!=0)
					{
					id_r=-1;
					error=-3;
					DPrintf("Fail to perform Async Read\n\n");
					fast_read=0;
					break;
					}

				i_reading=1;

			}

		// fast read end

		if(current_fast_file_r<fast_num_files && fast_files[current_fast_file_r].fl==3 && !fast_writing)
			{
			id_r=-1;
			//fast_read=0;

			if(fast_files[current_fast_file_r].readed<0LL)
				{
					DPrintf("Error Reading %s\n", fast_files[current_fast_file_r].pathr);
					error=-3;
					break;
				}
            
			// double buffer

			if(fast_files[current_fast_file_r].use_doublebuffer)
				{
					//DPrintf("Double Buff Write\n");
					
					current_fast_file_w=current_fast_file_r;
					
					memcpy(((char *) fast_files[current_fast_file_r].mem)+fast_files[current_fast_file_r].size_mem,
					fast_files[current_fast_file_r].mem, fast_files[current_fast_file_r].size_mem);

					fast_files[current_fast_file_w].t_write.size= fast_files[current_fast_file_r].t_read.size;
					
					if(fast_files[current_fast_file_w].bigfile_mode==1)
						fast_files[current_fast_file_w].t_write.offset= (int64_t) fast_files[current_fast_file_w].giga_counter;
					else
						fast_files[current_fast_file_w].t_write.offset= fast_files[current_fast_file_w].writed;
					
					fast_writing=1;



					if(cellFsAioWrite(&fast_files[current_fast_file_w].t_write, &id_w, fast_func_write)!=0)
						{
						id_w=-1;
						error=-4;
						DPrintf("Fail to perform Async Write\n\n");
						fast_writing=0;
						break;
						}

					if(fast_files[current_fast_file_r].readed<fast_files[current_fast_file_r].len)
						{
						fast_files[current_fast_file_r].t_read.size=fast_files[current_fast_file_r].len-fast_files[current_fast_file_r].readed;
						if((int64_t) fast_files[current_fast_file_r].t_read.size> fast_files[current_fast_file_r].size_mem)
							fast_files[current_fast_file_r].t_read.size=fast_files[current_fast_file_r].size_mem;

						fast_files[current_fast_file_r].fl=2;
						fast_files[current_fast_file_r].t_read.offset= fast_files[current_fast_file_r].readed-fast_files[current_fast_file_r].off_readed;

						fast_read=1;
						if(cellFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read)!=0)
							{
							id_r=-1;
							error=-3;
							DPrintf("Fail to perform Async Read\n\n");
							fast_read=0;
							break;
							}
						}
					else 
						{
						if(fast_files[current_fast_file_r].bigfile_mode==2)
							{
							struct stat s;
							
							fast_files[current_fast_file_r].number_frag++;

							fast_files[current_fast_file_r].off_readed= fast_files[current_fast_file_r].readed;

							DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_r].number_frag);
							sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path],".666%2.2i",
								fast_files[current_fast_file_r].number_frag);

							if(stat(fast_files[current_fast_file_r].pathr, &s)<0) {current_fast_file_r++;i_reading=0;}
							else
								{
								if(fast_files[current_fast_file_r].t_read.fd>=0) 
									{cellFsClose(fast_files[current_fast_file_r].t_read.fd);files_opened--;}fast_files[current_fast_file_r].t_read.fd=-1;
								
								if(cellFsOpen(fast_files[current_fast_file_r].pathr, CELL_FS_O_RDONLY, &fdr, 0,0)!=CELL_FS_SUCCEEDED) 
									{
									DPrintf("Error Opening (read):\n%s\n\n", fast_files[current_fast_file_r].pathr);
									error=-1;
									break;
									}else files_opened++;

								fast_files[current_fast_file_r].t_read.fd= fdr;

								fast_files[current_fast_file_r].len += (int64_t) s.st_size;

								fast_files[current_fast_file_r].t_read.offset= 0LL;
								fast_files[current_fast_file_r].t_read.buf= fast_files[current_fast_file_r].mem;
							

								fast_files[current_fast_file_r].t_read.size=fast_files[current_fast_file_r].len-fast_files[current_fast_file_r].readed;
								if((int64_t) fast_files[current_fast_file_r].t_read.size> fast_files[current_fast_file_r].size_mem)
									fast_files[current_fast_file_r].t_read.size=fast_files[current_fast_file_r].size_mem;

								fast_files[current_fast_file_r].t_read.user_data= (uint32_t )&fast_files[current_fast_file_r];

								fast_read=1;
								if(cellFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read)!=0)
									{
									id_r=-1;
									error=-3;
									DPrintf("Fail to perform Async Read\n\n");
									fast_read=0;
									break;
									}

								fast_files[current_fast_file_r].fl=2;

								}
							}
						else
							{fast_files[current_fast_file_r].fl=5;current_fast_file_r++;i_reading=0;}
						
						
						}

				}
			else
			// single buffer	
				{
					//DPrintf("Single Buff Write\n");

					current_fast_file_w=current_fast_file_r;
					fast_files[current_fast_file_w].t_write.size= fast_files[current_fast_file_r].t_read.size;

					fast_files[current_fast_file_w].t_write.offset= fast_files[current_fast_file_w].writed;
					
					fast_writing=1;

					if(cellFsAioWrite(&fast_files[current_fast_file_w].t_write, &id_w, fast_func_write)!=0)
						{
						id_w=-1;
						error=-4;
						DPrintf("Fail to perform Async Write\n\n");
						fast_writing=0;
						break;
						}
					
					
					current_fast_file_r++;
					i_reading=0;
				}
			}
		
		// fast write end
		if(fast_writing>1)
			{
				fast_writing=0;
				id_w=-1;

				if(fast_files[current_fast_file_w].writed<0LL)
				{
					DPrintf("Error Writing %s\n", fast_files[current_fast_file_w].pathw);
					error=-4;
					break;
				}

				write_end=fast_files[current_fast_file_w].writed;
				write_size=fast_files[current_fast_file_w].len;

				if(fast_files[current_fast_file_w].writed>=fast_files[current_fast_file_w].len)
					{
				        if(fast_files[current_fast_file_w].t_read.fd>=0)
							{cellFsClose(fast_files[current_fast_file_w].t_read.fd);files_opened--;}fast_files[current_fast_file_w].t_read.fd=-1;
						if(fast_files[current_fast_file_w].t_write.fd>=0)
							{cellFsClose(fast_files[current_fast_file_w].t_write.fd);files_opened--;}fast_files[current_fast_file_w].t_write.fd=-1;
						
						cellFsChmod(fast_files[current_fast_file_w].pathw, CELL_FS_S_IFMT | 0777);

						if(fast_files[current_fast_file_w].bigfile_mode==1)
							{
							fast_files[current_fast_file_w].pathw[fast_files[current_fast_file_w].pos_path]=0;
							}

						if(write_size<1024LL) { }
								//DPrintf("Wrote %lli B %s\n\n", write_size, fast_files[current_fast_file_w].pathw);
						else
							if(write_size<0x100000LL) DPrintf("Wrote %lli KB %s\n\n", write_size/1024LL, fast_files[current_fast_file_w].pathw);
								else  DPrintf("Wrote %lli MB %s\n\n", write_size/0x100000LL, fast_files[current_fast_file_w].pathw);

						fast_files[current_fast_file_w].fl=4; //end of proccess
						
						fast_files[current_fast_file_w].writed=-1LL;
						current_fast_file_w++;
						//if(current_fast_file_r<current_fast_file_w) current_fast_file_w=current_fast_file_r;
						file_counter++;
					}
				else
				// split big files
				if(fast_files[current_fast_file_w].bigfile_mode==1 && fast_files[current_fast_file_w].giga_counter>=0x40000000)
					{
						if(fast_files[current_fast_file_w].t_write.fd>=0)
							{cellFsClose(fast_files[current_fast_file_w].t_write.fd);files_opened--;}fast_files[current_fast_file_w].t_write.fd=-1;

						cellFsChmod(fast_files[current_fast_file_w].pathw, CELL_FS_S_IFMT | 0777);
						
						fast_files[current_fast_file_w].giga_counter=0;
						fast_files[current_fast_file_w].number_frag++;
						sprintf(&fast_files[current_fast_file_w].pathw[fast_files[current_fast_file_w].pos_path],".666%2.2i",
							fast_files[current_fast_file_w].number_frag);
						DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_w].number_frag);
						
						if(cellFsOpen(fast_files[current_fast_file_w].pathw, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdw, 0,0)!=CELL_FS_SUCCEEDED)
							{
							DPrintf("Error Opening2 (write):\n%s\n\n", fast_files[current_fast_file_w].pathw);
							error=-2;
							break;
							}else files_opened++;

						fast_files[current_fast_file_w].t_write.fd=fdw;
					}

				
			}

		
	int seconds= (int) (time(NULL)-time_start);

	sprintf(string1,"Copying. File: %i Time: %2.2i:%2.2i:%2.2i %2.2i/100 Vol: %1.2f GB\n", file_counter, seconds/3600, (seconds/60) % 60, seconds % 60,
						(int)(write_end*100ULL/write_size), ((double) global_device_bytes)/(1024.0*1024.*1024.0));
		
	cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

	draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x200020ff);

	cellDbgFontPrintf( 0.07f, 0.07f, 1.2f, 0xffffffff, string1);

	
	cellDbgFontPrintf( 0.5f-0.15f, 1.0f-0.07*2.0f, 1.2f, 0xffffffff, "Hold /\\ to Abort");

	
	cellDbgFontDrawGcm();

	flip();

	pad_read();
	if ( new_pad & BUTTON_TRIANGLE) 
		{
		abort_copy=1;
		DPrintf("Aborted by user \n");
		error=-666;
		break;
		}
	
	}

	if(error && error!=-666)
		{
		DPrintf("Error!!!!!!!!!!!!!!!!!!!!!!!!!\nFiles Opened %i\n Waiting 20 seconds to display fatal error\n", files_opened);
		cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

		draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x200020ff);

		cellDbgFontPrintf( 0.07f, 0.07f, 1.2f, 0xffffffff, string1);

		
		cellDbgFontPrintf( 0.5f-0.15f, 1.0f-0.07*2.0f, 1.2f, 0xffffffff, "Hold /\\ to Abort");

		
		cellDbgFontDrawGcm();

		flip();


		sys_timer_usleep(20*1000000);
		}


	if(fast_writing==1 && id_w>=0)
		{
		cellFsAioCancel(id_w);
		id_w=-1;
		sys_timer_usleep(200000);
		}
	
	fast_writing=0;

	if(fast_read==1 && id_r>=0)
		{
		cellFsAioCancel(id_r);
		id_r=-1;
		sys_timer_usleep(200000);
		}
	
	fast_read=0;

	for(n=0;n<fast_num_files;n++)
	{
		if(fast_files[n].t_read.fd>=0) 
			{
			
			cellFsClose(fast_files[n].t_read.fd);fast_files[n].t_read.fd=-1;
			files_opened--;
			}
		if(fast_files[n].t_write.fd>=0)	
			{
			
			cellFsClose(fast_files[n].t_write.fd);fast_files[n].t_write.fd=-1;
			files_opened--;
			}

		if(fast_files[n].mem) free(fast_files[n].mem); fast_files[n].mem=NULL;
	}

	fast_num_files=0;

	fast_writing=0;

	fast_used_mem=0;

	current_fast_file_r= current_fast_file_w= 0;

	if(error) abort_copy=666;

	return error;
}

static int _my_game_copy(char *path, char *path2)
{
	DIR  *dir;

	dir=opendir (path);
	if(!dir) {abort_copy=7;return -1;}

	while(1)
		{
		struct dirent *entry=readdir (dir);
		if(!entry) break;

		if(entry->d_name[0]=='.' && entry->d_name[1]==0) continue;
		if(entry->d_name[0]=='.' && entry->d_name[1]=='.' && entry->d_name[2]==0) continue;

		if((entry->d_type & DT_DIR))
			{

			if(abort_copy) break;

			char *d1= (char *) malloc(1024);
			char *d2= (char *) malloc(1024);
			if(!d1 || !d2) {if(d1) free(d1); if(d2) free(d2);closedir (dir);DPrintf("malloc() Error!!!\n\n");abort_copy=2;closedir(dir);return -1;}
			sprintf(d1,"%s/%s", path, entry->d_name);
			sprintf(d2,"%s/%s", path2, entry->d_name);
			DPrintf("D1: %s\nD2: %s", path, path2);

			//if(strcmp(path2, "/dev_bdvd/PS3_UPDATE") == 0)
			//{
			mkdir(d2, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
			_my_game_copy(d1, d2);
			//}

			free(d1);free(d2);
			if(abort_copy) break;
			}
		else
			{
				if(strcmp(entry->d_name, "PS3UPDAT.PUP") == 0)
				{
				}
				else
				{
					DPrintf("EPATH: %s\nEPATH2: %s\nENTRY %s", path, path2, entry->d_name);
					if(fast_copy_add(path, path2, entry->d_name)<0)
					{
						abort_copy=666;
						closedir(dir);
						return -1;
					}
				}
	
			}
		
		if(abort_copy) break;
		}

	closedir(dir);
	if(abort_copy) return -1;

	return 0;

}

static int my_game_copy(char *path, char *path2)
{
	global_device_bytes=0;

	if(fast_copy_async(path, path2, 1)<0) {abort_copy=665;return -1;}

	int ret=_my_game_copy(path, path2);

	int ret2= fast_copy_process();

	fast_copy_async(path, path2, 0);

	if(ret<0 || ret2<0) return -1;

	return 0;
}



int num_directories=0, num_files_big=0, num_files_split=0;

// test if files >= 4GB

static int my_game_test(char *path)
{
	DIR  *dir;
   dir=opendir (path);
   if(!dir) return -1;

   while(1)
		{
		struct dirent *entry=readdir (dir);
		if(!entry) break;

		if(entry->d_name[0]=='.' && entry->d_name[1]==0) continue;
		if(entry->d_name[0]=='.' && entry->d_name[1]=='.' && entry->d_name[2]==0) continue;

		if((entry->d_type & DT_DIR))
			{
			
				char *d1= (char *) malloc(1024);
				
				num_directories++;

				if(!d1) {closedir (dir);DPrintf("malloc() Error!!!\n\n");abort_copy=2;return -1;}
				sprintf(d1,"%s/%s", path, entry->d_name);
				my_game_test(d1);
				free(d1);

				if(abort_copy) break;
			}
		else
			{
				char *f= (char *) malloc(1024);

				struct stat s;
					
				off64_t size=0LL;

				if(!f) {DPrintf("malloc() Error!!!\n\n");abort_copy=2;closedir (dir);return -1;}
				sprintf(f,"%s/%s", path, entry->d_name);

				if(stat(f, &s)<0) {abort_copy=3;DPrintf("File error!!!\n -> %s\n\n", f);if(f) free(f);break;}

				size= s.st_size;

				if(strlen(entry->d_name)>6)
					{
					char *p= f;
					p+= strlen(f)-6; // adjust for .666xx
					if(p[0]== '.' && p[1]== '6' && p[2]== '6' && p[3]== '6')
						{  
						DPrintf("Split file %lli MB %s\n\n", size/0x100000LL, f);
						num_files_split++;
								
						}
							
					}
			
				if(size>=0x100000000LL)	{DPrintf("Big file %lli MB %s\n\n", size/0x100000LL, f);num_files_big++;}

				if(f) free(f);

				int seconds= (int) (time(NULL)-time_start);
				
			    file_counter++;
				
				global_device_bytes+=size;
				
				sprintf(string1,"Test File: %i Time: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", file_counter, seconds/3600, 
					(seconds/60) % 60, seconds % 60, ((double) global_device_bytes)/(1024.0*1024.*1024.0));
			
				cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

				draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x200020ff);

				cellDbgFontPrintf( 0.07f, 0.07f, 1.2f, 0xffffffff, string1);

				cellDbgFontPrintf( 0.5f-0.15f, 1.0f-0.07*2.0f, 1.2f, 0xffffffff, "Hold /\\ to Abort");
			
				cellDbgFontDrawGcm();

				flip();

				pad_read();

				if(abort_copy) break;

				if (new_pad & BUTTON_TRIANGLE) 
					{
					abort_copy=1;
					}
			
			if(abort_copy) break;

			}

		}

	closedir (dir);

return 0;
}

static int my_game_delete(char *path)
{
	DIR  *dir;
	char *f= NULL;

   dir=opendir (path);
   if(!dir) return -1;
   
   while(1)
		{
		struct dirent *entry=readdir (dir);
		if(!entry) break;
		
		if(entry->d_name[0]=='.' && entry->d_name[1]==0) continue;
		if(entry->d_name[0]=='.' && entry->d_name[1]=='.' && entry->d_name[2]==0) continue;

		if((entry->d_type & DT_DIR))
			{
			
			char *d1= (char *) malloc(1024);

			if(!d1) {closedir (dir);DPrintf("malloc() Error!!!\n\n");abort_copy=2;return -1;}
			sprintf(d1,"%s/%s", path, entry->d_name);
			my_game_delete(d1);
            DPrintf("Deleting <%s>\n\n", path);
			if(rmdir(d1)) {abort_copy=3;DPrintf("Deleting Error!!!\n -> <%s>\n\n", entry->d_name);if(d1) free(d1);break;}
			free(d1);
			if(abort_copy) break;
			file_counter--;

			goto display_mess;
			}
		else
			{
			int seconds;
			f=(char *) malloc(1024);
			
			if(!f) {DPrintf("malloc() Error!!!\n\n");abort_copy=2;closedir (dir);return -1;}
			sprintf(f,"%s/%s", path, entry->d_name);
			
			if(remove(f)) {abort_copy=3;DPrintf("Deleting Error!!!\n -> %s\n\n", f);if(f) free(f);break;}

			DPrintf("Deleted %s\n\n", f);

			if(f) free(f);

			display_mess:

			seconds= (int) (time(NULL)-time_start);
			sprintf(string1,"Deleting... File: %i Time: %2.2i:%2.2i:%2.2i\n", file_counter, seconds/3600, (seconds/60) % 60, seconds % 60);

			file_counter++;
		
			cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

			draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x200020ff);

			cellDbgFontPrintf( 0.07f, 0.07f, 1.2f, 0xffffffff, string1);

			cellDbgFontPrintf( 0.5f-0.15f, 1.0f-0.07*2.0f, 1.2f, 0xffffffff, "Hold /\\ to Abort");
			
			cellDbgFontDrawGcm();

			flip();

			pad_read();

			if(abort_copy) break;

			if (new_pad & BUTTON_TRIANGLE) 
				{
				abort_copy=1;
				}
					
			if(abort_copy) break;

			}

		}

	closedir (dir);

	return 0;
}

/*****************************************************/
/* CALLBACK                                          */
/*****************************************************/

static void gfxSysutilCallback(uint64_t status, uint64_t param, void* userdata)
{
     (void) param;
     (void) userdata;
     switch (status)
     {
       case CELL_SYSUTIL_REQUEST_EXITGAME:
		if(mode_list==GAME)
			syscall36( (char *) "/dev_bdvd"); // restore bluray
		else
			syscall36( (char *) "/dev_usb000"); // restore

		unload_modules();
	
		sys_process_exit(1);
       break;
       case CELL_SYSUTIL_DRAWING_BEGIN:
       case CELL_SYSUTIL_DRAWING_END:
       break;
       default:
         printf( "Graphics common: Unknown status received: 0x%llx\n", status );
      }
}


/****************************************************/
/* MAIN                                             */
/****************************************************/

static char bluray_game[64]; // name of the game

int main(int argc, char **argv)
{
	sys_spu_initialize(2, 0);
	//BGMArg bgmArg;
	int ret;
	int dir_fixed=0;
	//int    fm = -1;
	int one_time=1;

	int counter_png=0;
	uint64_t patchmode = 2;  //0 -> PS3 perms normally, 1-> Psjailbreak by default, 2-> Special for games as F1 2010 (option by default)	
	u8 *text_bmp=NULL;
	u8 *text_h=NULL;
	u8 *text_bg=NULL;
	//char INPUT_FILE[] = "/dev_bdvd/PS3_GAME/SND0.AT3";
	ret = load_modules();
	load_libfont_module();


	sprintf(soundfile,"/dev_hdd0/game/%s/USRDIR/BOOT.AT3", hdd_folder_home);
	sys_ppu_thread_create(&soundThread, playBootSound, NULL, 100, 0x8000, 0, (char *)"sound thread");

	//fix_perm_recursive("/dev_hdd0/game/OMAN46756/cache2/");
	cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_LANG, &region);


	cellNetCtlInit();
	//ftp_off();
	u32 frame_buf_size = DISPLAY_HEIGHT * DISPLAY_WIDTH * 4;

	frame_buf_size = ( frame_buf_size + 0xfffff ) & ( ~0xfffff );
	text_bmp = (u8 *) memalign(0x100000, frame_buf_size);
	text_bg = (u8 *) memalign(0x100000, frame_buf_size);
	text_h = (u8 *) memalign(0x100000, frame_buf_size);
	if(!text_bmp) exit(-1);

	if(png_out_mapmem( text_bmp, frame_buf_size)) exit(-1);
	if(png_out_mapmem( text_bg, frame_buf_size)) exit(-1);
	if(png_out_mapmem( text_h, frame_buf_size)) exit(-1);

	cellSysutilRegisterCallback(0, gfxSysutilCallback, NULL);

	sprintf(filename, "/dev_hdd0/game/%s/USRDIR/BG.png", hdd_folder_home);
	load_png_texture(text_bg, filename);

	sprintf(filename, "/dev_hdd0/game/%s/USRDIR/HIGHLIGHT.png", hdd_folder_home);
	load_png_texture(text_h, filename);

	setRenderColor();
	
	if(sys8_enable(0) > 0)
	{
		sys8_perm_mode(patchmode);
	}
	else
	{
		pokeq(0x80000000000505d0ULL, mem_orig);
	}

	if(!memcmp(hdd_folder,"ASDFGHJKLM",10) && hdd_folder[10]=='N')
update_game_folder:
	{


		DIR  *dir,*dir2;
		dir=opendir ("/dev_hdd0/game");
		
		if(dir)
			{
			while(1)
				{
				struct dirent *entry=readdir (dir);
				
				if(!entry) break;
				if(entry->d_name[0]=='.') continue;

				if(!(entry->d_type & DT_DIR)) continue;

                sprintf(filename, "/dev_hdd0/game/%s/GAMEZ", entry->d_name);
			//sprintf(filename, "/dev_hdd0/GAMES");
                
				dir2=opendir (filename);

				if(dir2)
					{
					closedir (dir2);

					dialog_ret=0;
		
					sprintf(filename, "%s /%s %s", text_wantuse[region], entry->d_name, text_toinstall[region]); 

					ret = cellMsgDialogOpen2( type_dialog_yes_no, filename, dialog_fun1, (void*)0x0000aaaa, NULL );
					
					wait_dialog();
		
					if(dialog_ret==1)  
						{
						strncpy(hdd_folder, entry->d_name, 64);
						dir_fixed=1;
						break;
						}
					}
					
				}
			closedir (dir);
			}
	
	
	if(!dir_fixed)
		{
		if(argc>=1)
			{
			// use the path of EBOOT.BIN
			if(!strncmp( argv[0], "/dev_hdd0/game/", 15))
				{
				char *s;
				int n=0;

				s= ((char *) argv[0])+15;
				while(*s!=0 && *s!='/' && n<63) {hdd_folder[n]=*s++; n++;} hdd_folder[n]=0;

				dir_fixed=1;

				// create the folder
				sprintf(filename, "/dev_hdd0/game/%s/GAMEZ",hdd_folder);
				//sprintf(filename, "/dev_hdd0/GAMES");
				mkdir(filename, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

				dialog_ret=0;
				sprintf(filename, "/%s\nis the new folder for games", hdd_folder); 
				ret = cellMsgDialogOpen2( type_dialog_ok, filename, dialog_fun2, (void*)0x0000aaab, NULL );
				wait_dialog();

				}
			else
				{
				dialog_ret=0;
				ret = cellMsgDialogOpen2( type_dialog_ok, "Panic!!!\nI cannot find the folder to install games!!!", dialog_fun2, (void*)0x0000aaab, NULL );
				wait_dialog();
				syscall36( (char *) "/dev_bdvd"); // restore bluray
				ret = unload_modules();
				exit(0);
				}
			}
		}
	
	// modify EBOOT.BIN
	if(dir_fixed && argc>=1)
		{
			FILE *fp;
			int n;
		
			fp = fopen(argv[0], "r+");
			if (fp != NULL)
				{
				int len;
				char *mem=NULL;

				fseek(fp, 0, SEEK_END);
				len=ftell(fp);

				mem= (char *) malloc(len+16);
				if(!mem) {fclose(fp);return -2;}

				fseek(fp, 0, SEEK_SET);

				fread((void *) mem, len, 1, fp);

				for(n=0;n<(len-10);n++)
					{
				
					if(!memcmp(&mem[n], "ASDFGHJKLM",10) && mem[n+10]=='N')
						{
						strncpy(&mem[n], hdd_folder, 64);
						
						fseek(fp, 0, SEEK_SET);
						fwrite((void *) mem, len, 1, fp);
						dialog_ret=0;
						sprintf(filename, "%s\n%s", text_eboot[region], text_launcher[region]);
						ret = cellMsgDialogOpen2( type_dialog_ok, filename, dialog_fun2, (void*)0x0000aaab, NULL );
						wait_dialog();

						break;
						}
				}
				
			fclose(fp);
			}
		}
	}

	// create homebrew folder
	if(argc>=1)
		{
		// creates homebrew folder
		if(!strncmp( argv[0], "/dev_hdd0/game/", 15))
			{
			char *s;
			int n=0;

			s= ((char *) argv[0])+15;
			while(*s!=0 && *s!='/' && n<63) {hdd_folder_home[n]=*s++; n++;} hdd_folder_home[n]=0;

			// create the folder
			sprintf(filename, "/dev_hdd0/game/%s/homebrew",hdd_folder_home);
			mkdir(filename, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
			}
		}

	max_menu_list=0;
	max_menu_homebrew_list=0;

	u32 fdevices=0;
	u32 fdevices_old=0;
	u32 forcedevices=0;
	int find_device=0;


	syscall36( (char *) "/dev_bdvd"); // select bluray

	/* main loop */
	while( pad_read() != 0)
	{
	static int old_fi=-1;
    
	// scan for plug/unplug devices
	
	int count_devices=0;

	if(one_time) {one_time=0;goto skip_find_device;}

	for(find_device=0;find_device<12;find_device++)
		{
			CellFsStat fstatus;
			if(find_device==11) sprintf(filename, "/dev_bdvd");
			else if(find_device==0) sprintf(filename, "/dev_hdd0");
			else sprintf(filename, "/dev_usb00%c", 47+find_device);
			
			if (cellFsStat(filename, &fstatus) == CELL_FS_SUCCEEDED)
				{
				fdevices|= 1<<find_device;
				}
			else
				fdevices&= ~ (1<<find_device);

			// limit to 3 the devices selectables
			if(((fdevices>>find_device) & 1) && find_device!=11)
			{
			count_devices++;

				if(count_devices>3) fdevices&= ~ (1<<find_device);

			}

			// bdvd
			if(find_device==11)
				{
				
					if(fdevices!=fdevices_old || ((forcedevices>>find_device) & 1))
						{
							game_sel=0;
							sprintf(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");
							bluray_game[0]=0;
							parse_param_sfo(filename, bluray_game);
							bluray_game[63]=0;
							
							if((fdevices>>11) & 1)
								{

								if(max_menu_list>=MAX_LIST) max_menu_list= MAX_LIST-1;
								
								sprintf(menu_list[max_menu_list].path, "/dev_bdvd");
								
								memcpy(menu_list[max_menu_list].title, bluray_game, 63);
								menu_list[max_menu_list].title[63]=0;
								menu_list[max_menu_list].flags=(1<<11);

								max_menu_list++;

								}
							else
								delete_entries(menu_list, &max_menu_list, (1<<11));

							sort_entries(menu_list, &max_menu_list );
						}
				
					forcedevices &= ~ (1<<find_device);
					fdevices_old&= ~ (1<<find_device);
					fdevices_old|= fdevices & (1<<find_device);
				}
			else
			// refresh list 
			if(fdevices!=fdevices_old || ((forcedevices>>find_device) & 1))
				{
					game_sel=0;
					
					forcedevices &= ~ (1<<find_device);

					if(find_device==0) sprintf(filename, "/dev_hdd0/game/%s/GAMEZ",hdd_folder);
						else sprintf(filename, "/dev_usb00%c/GAMEZ", 47+find_device);

					if((fdevices>>find_device) & 1)
						fill_entries_from_device(filename, menu_list, &max_menu_list, (1<<find_device), 0);
					else
					   delete_entries(menu_list, &max_menu_list, (1<<find_device));
					sort_entries(menu_list, &max_menu_list );

					if(find_device>=0)
						{
						if(find_device==0) sprintf(filename, "/dev_hdd0/game/%s/homebrew",hdd_folder_home);
						else 
							sprintf(filename, "/dev_usb00%c/homebrew", 47+find_device);

						if((fdevices>>find_device) & 1)
							fill_entries_from_device(filename, menu_homebrew_list, &max_menu_homebrew_list, (1<<find_device), 1);
						else
						   delete_entries(menu_homebrew_list, &max_menu_homebrew_list, (1<<find_device));
						sort_entries(menu_homebrew_list, &max_menu_homebrew_list );
						}
					fdevices_old&= ~ (1<<find_device);
					fdevices_old|= fdevices & (1<<find_device);
				}
		}

	
	// load png file (with delay)
	if(no_video) goto skip_1;

	if(old_fi!=game_sel && game_sel>=0 && counter_png==0)
		{
			old_fi=game_sel;
			if(mode_list==GAME)
			{
				sprintf(filename, "%s/PS3_GAME/ICON0.PNG", menu_list[game_sel].path);
			}
			else
				sprintf(filename, "%s/ICON0.PNG", menu_homebrew_list[game_sel].path);
			load_png_texture(text_bmp, filename);
			
			counter_png=20;
		}

	if(counter_png) counter_png--;
	
skip_find_device:
		
	if ( old_pad & BUTTON_UP ) {
		if (up_count>7){
		  up_count=0;
			game_sel--;
			if(game_sel<0)  
				{
					game_sel=*max_list-1;
				}
		} else up_count++;
	
	} else up_count=8;

	if ( old_pad & BUTTON_LEFT ) {
		if (left_count>7){
		  left_count=0;
			if(game_sel == 0)
			{
				game_sel=*max_list-1;
			}
			else
			{
				game_sel = game_sel-16;
				if(game_sel<0)  
					game_sel = 0;
			}
		} else left_count++;
	
	} else left_count=8;

	
	if ( old_pad & BUTTON_DOWN ) {
		if (down_count>7){
			down_count=0;
			game_sel++;
			if(game_sel>=*max_list) game_sel=0;
		} else down_count++;
			
	} else down_count=8;
	if (old_pad & BUTTON_R3 ) {
  	//  bad_perm_recursive(menu_list[game_sel].path);
		sprintf(string1, "Fix permissions in all directories?\n\nNot necessary with latest hermes, but may still be useful\n(may appear to freeze, give it time)"); 
		dialog_ret=0;
		ret = cellMsgDialogOpen2( type_dialog_yes_no, string1, dialog_fun1, (void*)0x0000aaaa, NULL );
		wait_dialog();
               	if(ret==1)
		{
			sprintf(filename, "/dev_hdd0/game/%s/GAMEZ/",hdd_folder);
			fix_perm_recursive(filename);
		}
	}

	if ( old_pad & BUTTON_RIGHT ) {

		if (right_count>7){
			right_count=0;
			if(game_sel == *max_list-1)
			{
				game_sel = 0;
			}
			else
			{
				game_sel = game_sel + 16;
				if(game_sel>=*max_list) game_sel=*max_list-1;
			}

		} else right_count++;
			
	} else right_count=8;


    // update the game folder

	if ((new_pad & BUTTON_START) && (old_pad & BUTTON_SELECT)){
		
		dir_fixed=0; goto update_game_folder;
		}

	 if ( new_pad & BUTTON_R2){
		game_sel=0;
	if (mode_list==GAME)
	{
		mode_list = HOMEBREW;
		max_list=&max_menu_homebrew_list;
	}
	else
	{
		mode_list = GAME;
		max_list=&max_menu_list;
	}
		old_fi=-1;
		counter_png=0;
		}

	if ((new_pad & BUTTON_L3) && game_sel>=0 && max_menu_list>0 && mode_list==GAME){

		time_start= time(NULL);
			
		abort_copy=0;

		initConsole();

		file_counter=0;
		new_pad=0;
		
		global_device_bytes=0;

		num_directories= file_counter= num_files_big= num_files_split= 0;

		my_game_test( menu_list[game_sel].path);

		DPrintf("Directories: %i Files: %i\nBig files: %i Split files: %i\n\n", num_directories, file_counter, num_files_big, num_files_split);

		int seconds= (int) (time(NULL)-time_start);
		int vflip=0;

		while(1){

			if(abort_copy) sprintf(string1,"Aborted!!!  Time: %2.2i:%2.2i:%2.2i\n", seconds/3600, (seconds/60) % 60, seconds % 60);
			else
				sprintf(string1,"Files Tested: %i Time: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", file_counter, seconds/3600, (seconds/60) % 60, seconds % 60, ((double) global_device_bytes)/(1024.0*1024.*1024.0));
			

			cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

			draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x000000ff);

			cellDbgFontPrintf( 0.07f, 0.07f, 1.2f,0xffffffff,string1);
			
			if(vflip & 32)
			cellDbgFontPrintf( 0.5f-0.15f, 1.0f-0.07*2.0f, 1.2f, 0xffffffff, "Press X to Exit");
			vflip++;

			cellDbgFontDrawGcm();


			flip();

			pad_read();
			if (new_pad & BUTTON_CROSS) 
				{
				new_pad=0;
				break;
				}

			}
		termConsole();
			
		}
 
// delete from devices	

	if ( (new_pad & BUTTON_TRIANGLE) && game_sel>=0 && max_menu_list>0 && mode_list==GAME && (!(menu_list[game_sel].flags & 2048))){
		int n;

			for(n=0;n<11;n++){
				if((menu_list[game_sel].flags>>n) & 1) break;
				}

			if(n==0)
				sprintf(filename, "%s\n\n%s HDD0?", menu_list[game_sel].title, text_wantdel[region]); 
			else
				sprintf(filename, "%s\n\n%s USB00%c?", menu_list[game_sel].title, text_wantdel[region], 47+n); 

				dialog_ret=0;
				ret = cellMsgDialogOpen2( type_dialog_yes_no, filename, dialog_fun1, (void*)0x0000aaaa, NULL );
					
				wait_dialog();
				
			if(dialog_ret==1){

				time_start= time(NULL);
				
				// reset to update datas
				
				old_fi=-1;
				counter_png=0;
				forcedevices=(1<<n);
				
				abort_copy=0;
				initConsole();
				file_counter=0;
				new_pad=0;
				
				DPrintf("Starting... \n delete %s\n\n", menu_list[game_sel].path);

				my_game_delete((char *) menu_list[game_sel].path);
				
				rmdir((char *) menu_list[game_sel].path); // delete this folder

				game_sel=0;

				int seconds= (int) (time(NULL)-time_start);
				int vflip=0;

				while(1){

					if(abort_copy) sprintf(string1,"Aborted!!!  Time: %2.2i:%2.2i:%2.2i\n", seconds/3600, (seconds/60) % 60, seconds % 60);
					else
						sprintf(string1,"Done!  Files Deleted: %i Time: %2.2i:%2.2i:%2.2i\n", file_counter, seconds/3600, (seconds/60) % 60, seconds % 60);

					cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

					draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x000000ff);

					cellDbgFontPrintf( 0.07f, 0.07f, 1.2f, 0xffffffff, string1);
					
					if(vflip & 32)
						cellDbgFontPrintf( 0.5f-0.15f, 1.0f-0.07*2.0f, 1.2f, 0xffffffff, "Press X to Exit");

					vflip++;

					cellDbgFontDrawGcm();

					
					flip();

					pad_read();
					if (new_pad & BUTTON_CROSS) 
						{
						new_pad=0;
						break;
						}

				}
			termConsole();
			}
		}


	if ((new_pad & BUTTON_TRIANGLE) && mode_list==HOMEBREW)
		{
		// reset to update datas
		old_fi=-1;
		counter_png=0;
		forcedevices=(1);
		game_sel=0;
		}
// copy from devices

	if ((new_pad & BUTTON_CIRCLE) && game_sel>=0 && max_menu_list>0 && mode_list==GAME)
		{
		if(menu_list[game_sel].flags & 2048) goto copy_from_bluray;

		int n;
		int curr_device=0;
		char name[1024];
		int dest=0;

		dialog_ret=0;
		if(menu_list[game_sel].flags & 1) // is hdd0
			{
				
				for(n=1;n<11;n++)
				{
				dialog_ret=0;
				
				if((fdevices>>n) & 1)
					{

					sprintf(filename, "%s\n\n%s HDD0 %s USB00%c?", menu_list[game_sel].title, text_wantcopy[region], text_to[region], 47+n); 

					ret = cellMsgDialogOpen2( type_dialog_yes_no, filename, dialog_fun1, (void*)0x0000aaaa, NULL );
					
					wait_dialog();
					

					if(dialog_ret==1)  {curr_device=n;break;} // exit
					}
				}
			
		  
		   dest=n;
           if(dialog_ret==1)
				{

			    sprintf(name, "/dev_usb00%c/GAMEZ", 47+curr_device);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				sprintf(name, "/dev_usb00%c/GAMEZ", 47+curr_device);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				sprintf(name, "/dev_usb00%c/GAMEZ/%s", 47+curr_device, strstr(menu_list[game_sel].path, "/GAMEZ")+7);
				mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
					
				}

			}
		else
		if(fdevices & 1)
			{

				for(n=1;n<11;n++)
					{
					if((menu_list[game_sel].flags>>n) & 1) break;
					}

				if(n==11) continue;

				curr_device=0;
				
				dest=0;
				sprintf(filename, "%s\n\n%s USB00%c %s HDD0?",menu_list[game_sel].title, text_wantcopy[region], 47+n, text_to[region]); 
				dialog_ret=0;
				ret = cellMsgDialogOpen2( type_dialog_yes_no, filename, dialog_fun1, (void*)0x0000aaaa, NULL );
					
				wait_dialog();

				if(dialog_ret==1)
				{
					char *p=strstr(menu_list[game_sel].path, "/GAMEZ")+7;
					
					if(p[0]=='_') p++; // skip special char

					sprintf(name, "/dev_hdd0/game/%s", hdd_folder);	
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
					sprintf(name, "/dev_hdd0/game/%s/GAMEZ", hdd_folder);	
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
					sprintf(name, "/dev_hdd0/game/%s/GAMEZ/%s", hdd_folder, p);	
					mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
				}
			
			}
		
		if(dialog_ret==1)
			{

				// reset to update datas
				old_fi=-1;
				counter_png=0;
				forcedevices=(1<<curr_device);
				time_start= time(NULL);
	
				abort_copy=0;
				initConsole();
				file_counter=0;
				new_pad=0;

				DPrintf("Starting... \n copy %s\n to %s\n\n", menu_list[game_sel].path, name);

				if(curr_device!=0) copy_mode=1; // break files >= 4GB
						else copy_mode=0;

				copy_is_split=0;

				my_game_copy((char *) menu_list[game_sel].path, (char *) name);

				cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

				int seconds= (int) (time(NULL)-time_start);
				int vflip=0;
				
				if(copy_is_split && !abort_copy)
					{

					if(dest==0) 
						{
						char *p=strstr(menu_list[game_sel].path, "/GAMEZ")+7;
						if(p[0]=='_') p++; // skip special char

						sprintf(filename, "/dev_hdd0/game/%s/GAMEZ/_%s", hdd_folder, strstr(menu_list[game_sel].path, "/GAMEZ")+7);
						}
					else
						{
						sprintf(filename, "/dev_usb00%c/GAMEZ/_%s", 47+dest, strstr(menu_list[game_sel].path, "/GAMEZ")+7);
						}

					// try rename
					ret=rename(name, filename);

					if(dest==0)   sprintf(filename, "%s\n\nSplit game copied in HDD0 (non bootable)", menu_list[game_sel].title);
					else
						sprintf(filename, "%s\n\nSplit game copied in USB00%c (non bootable)", menu_list[game_sel].title, 47+curr_device);
					
					dialog_ret=0;

					ret = cellMsgDialogOpen2( type_dialog_ok, filename, dialog_fun2, (void*)0x0000aaab, NULL );					
					
					wait_dialog();

					}

				while(1)
				{
					if(abort_copy) sprintf(string1,"Aborted!!!  Time: %2.2i:%2.2i:%2.2i\n", seconds/3600, (seconds/60) % 60, seconds % 60);
					else
						{
						
						sprintf(string1,"Done! Files Copied: %i Time: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", file_counter, seconds/3600, (seconds/60) % 60, seconds % 60, ((double) global_device_bytes)/(1024.0*1024.*1024.0));
						}

				
					cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

					draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x000000ff);

					cellDbgFontPrintf( 0.07f, 0.07f, 1.2f, 0xffffffff, string1);
					
					if(vflip & 32)
					      cellDbgFontPrintf( 0.5f-0.15f, 1.0f-0.07*2.0f, 1.2f, 0xffffffff, "Press X to Exit");

					vflip++;

					cellDbgFontDrawGcm();

					flip();

					pad_read();
					if (new_pad & BUTTON_CROSS) 
						{
						new_pad=0;
						break;
						}

				}
		
			if(abort_copy )
				{
					if(dest==0)   sprintf(filename, "%s\n\n%s HDD0?", menu_list[game_sel].title, text_delfailed[region]);
						else sprintf(filename, "%s\n\n%s USB00%c?", menu_list[game_sel].title, text_delfailed[region], 47+dest);

					dialog_ret=0;
					ret = cellMsgDialogOpen2( type_dialog_yes_no, filename, dialog_fun1, (void*)0x0000aaaa, NULL );
							
					wait_dialog();

					if(dialog_ret==1)
						{

						abort_copy=0;
						time_start= time(NULL);
						file_counter=0;

						my_game_delete((char *) name);
			
						rmdir((char *) name); // delete this folder

						game_sel=0;

						}
					else
						{
						if(dest==0) 
							{
							char *p=strstr(menu_list[game_sel].path, "/GAMEZ")+7;
							if(p[0]=='_') p++; // skip special char

							//sprintf(filename, "/dev_hdd0/game/%s/GAMEZ/_%s", hdd_folder, p);	
							sprintf(filename, "/dev_hdd0/game/%s/GAMEZ/_%s", hdd_folder, p);
							}
						else
							{
							sprintf(filename, "/dev_usb00%c/GAMEZ/_%s", 47+dest, strstr(menu_list[game_sel].path, "/GAMEZ")+7);
							}
						
						ret=rename(name, filename);
						//
						}
				}

			game_sel=0;
			termConsole();
			}
	
		}

// copy from bluray

	    if ( (new_pad & BUTTON_SQUARE) && ((fdevices>>11) & 1) && mode_list==GAME)
copy_from_bluray:
		{

		char name[1024];
		int curr_device=0;
		CellFsStat fstatus;
		char id[16];
		
		int n;

		for(n=0;n<11;n++)
			{
			dialog_ret=0;
			
			if((fdevices>>n) & 1)
				{

				if(n==0) sprintf(filename, "%s\n\n%s BDVD %s HDD0?", bluray_game, text_wantcopy[region], text_to[region]); 
				else sprintf(filename, "%s\n\n%s BDVD %s USB00%c?",  bluray_game, text_wantcopy[region], text_to[region],47+n); 

				ret = cellMsgDialogOpen2( type_dialog_yes_no, filename, dialog_fun1, (void*)0x0000aaaa, NULL );
				
				wait_dialog();
	
				if(dialog_ret==1)  {curr_device=n;break;} // exit
				}
			}
			
         if(dialog_ret==1)
			{
				

				if(curr_device==0) sprintf(name, "/dev_hdd0");
				else sprintf(name, "/dev_usb00%c", 47+curr_device);

				
				if (cellFsStat(name, &fstatus) == CELL_FS_SUCCEEDED && !parse_ps3_disc((char *) "/dev_bdvd/PS3_DISC.SFB", id))
					{
					
					// reset to update datas
					game_sel=0;
					old_fi=-1;
					counter_png=0;
					forcedevices=(1<<curr_device);
					
					if(curr_device==0) 
						{
						sprintf(name, "/dev_hdd0/game/%s", hdd_folder);	
						mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
						sprintf(name, "/dev_hdd0/game/%s/GAMEZ", hdd_folder);	
						mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
						sprintf(name, "/dev_hdd0/game/%s/GAMEZ/%s", hdd_folder, id);	
						mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
						}
					else
						{
						sprintf(name, "/dev_usb00%c/GAMEZ", 47+curr_device);
						mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
						sprintf(name, "/dev_usb00%c/GAMEZ", 47+curr_device);
						mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
						sprintf(name, "/dev_usb00%c/GAMEZ/%s", 47+curr_device, id);
						mkdir(name, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
						}

					time_start= time(NULL);
					abort_copy=0;
					initConsole();
					file_counter=0;
					new_pad=0;
					
					if(curr_device!=0) copy_mode=1; // break files >= 4GB
						else copy_mode=0;

					copy_is_split=0;

					my_game_copy((char *) "/dev_bdvd", (char *) name);

					int seconds= (int) (time(NULL)-time_start);
					int vflip=0;

					if(copy_is_split && !abort_copy)
						{

						if(curr_device==0) 
							{
							sprintf(filename, "/dev_hdd0/game/%s/GAMEZ/_%s", hdd_folder, id);	
							}
						else
							{
							sprintf(filename, "/dev_usb00%c/GAMEZ/_%s", 47+curr_device, id);
							}
						
						ret=rename(name, filename);

						if(curr_device==0)   sprintf(filename, "%s\n\nSplit game copied in HDD0 (non bootable)", id);
						else
							sprintf(filename, "%s\n\nSplit game copied in USB00%c (non bootable)", id, 47+curr_device);

						dialog_ret=0;
						ret = cellMsgDialogOpen2( type_dialog_ok, filename, dialog_fun2, (void*)0x0000aaab, NULL );
						wait_dialog();
						
						}

					while(1)
					{

						if(abort_copy) sprintf(string1,"Aborted!!!  Time: %2.2i:%2.2i:%2.2i\n", seconds/3600, (seconds/60) % 60, seconds % 60);
						else
							{
							sprintf(string1,"Done! Files Copied: %i Time: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", file_counter, seconds/3600, (seconds/60) % 60, seconds % 60, ((double) global_device_bytes)/(1024.0*1024.*1024.0));
							}

						cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G |	CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

						draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x000000ff);

						cellDbgFontPrintf( 0.07f, 0.07f, 1.2f, 0xffffffff, string1);
						
						if(vflip & 32)
						cellDbgFontPrintf( 0.5f-0.15f, 1.0f-0.07*2.0f, 1.2f, 0xffffffff, "Press X to Exit");
						vflip++;

						cellDbgFontDrawGcm();

						
						flip();

						pad_read();
						if (new_pad & BUTTON_CROSS) 
							{
							new_pad=0;
							break;
							}

					}
				
			
				if(abort_copy)
					{
						if(curr_device==0)   sprintf(filename, "%s\n\n%s HDD0?", id, text_delfailed[region]);
							else sprintf(filename, "%s\n\n%s USB00%c?", id, text_delfailed[region], 47+curr_device);

						dialog_ret=0;
						ret = cellMsgDialogOpen2( type_dialog_yes_no, filename, dialog_fun1, (void*)0x0000aaaa, NULL );
								
						wait_dialog();
						

						if(dialog_ret==1)
							{
							time_start= time(NULL);
							file_counter=0;
							abort_copy=0;
							my_game_delete((char *) name);
				
							rmdir((char *) name); // delete this folder

							}
						else
						{
						if(curr_device==0) 
							{
							sprintf(filename, "/dev_hdd0/game/%s/GAMEZ/_%s", hdd_folder, id);	
							}
						else
							{
							sprintf(filename, "/dev_usb00%c/GAMEZ/_%s", 47+curr_device, id);
							}
						
						ret=rename(name, filename);
					
						}
					}
			

				termConsole();
				game_sel=0;

				}
			}
		}
	if (new_pad & BUTTON_L1)
	{
		if(sys8_enable(0) < 0)
		{
			if(peekq(0x80000000000505d0ULL) == mem_orig)
			{
				pokeq(0x80000000000505d0ULL, mem_patched);
				patchmode = 0; //patched
			}
			else
			{
			pokeq(0x80000000000505d0ULL, mem_orig);
			patchmode = 2; //normal
			//reset game list
			old_fi=-1;
			counter_png=0;
			forcedevices=(1);
			game_sel=0;
			} 
		}
		else
		{
			if (patchmode == 2)
			{
				patchmode = 0; //patched
				sys8_perm_mode(patchmode);
			}
			else
			{
				patchmode = 2; //normal
				sys8_perm_mode(patchmode);
			}
		}
		hermes = !patchmode;
	}
	if (new_pad & BUTTON_L2)
	{
		direct_boot ^= 1;
	}
	if (new_pad & BUTTON_R1)
	{
		if(ftp_flags & 2) ftp_off(); else ftp_on();
	}
	if (new_pad & BUTTON_CROSS && game_sel>=0 && *max_list>0) {
		
		if(menu_list[game_sel].flags & 2048)
			{
			flip();
			syscall36( (char *) "/dev_bdvd"); // restore bdvd
			ret = unload_modules();
			exit(0);
			}

		if(mode_list==HOMEBREW)	
			{
			int prio = 1001;
		    uint64_t flags = SYS_PROCESS_PRIMARY_STACK_SIZE_64K;

			sprintf(filename, "%s/EBOOT.BIN", menu_homebrew_list[game_sel].path);
			
			flip();

// SHOULD DETERMINE USB #?

			syscall36( (char *) "/dev_usb000"); // restore
			ret = unload_modules();

			sys_game_process_exitspawn(filename, NULL, NULL, 0, 0, prio, flags);
			exit(0);
			}

	   if(game_sel>=0 && max_menu_list>0)
			{
			if(menu_list[game_sel].title[0]=='_')
				{
					sprintf(filename, "%s\n\n%s", menu_list[game_sel].title, text_nosplit[region]);

					dialog_ret=0;
					ret = cellMsgDialogOpen2( type_dialog_ok, filename, dialog_fun2, (void*)0x0000aaab, NULL );
					wait_dialog();
				}
			else
				{
				struct stat s;
				sprintf(filename, "%s/PS3_GAME/USRDIR/EBOOT.BIN", menu_list[game_sel].path);

				if(stat(filename, &s)>=0)
					{
					syscall36( menu_list[game_sel].path );
					//sprintf(string1, "::: %s :::\n\nDo you want to start the game now,\nwithout exiting to XMB?\n\nWarning: Some games do not support such launch mode!",  menu_list[game_sel].title); 
					//dialog_ret=0;
					//ret = cellMsgDialogOpen2( type_dialog_yes_no, string1, dialog_fun1, (void*)0x0000aaaa, NULL );
					//wait_dialog();
					if(direct_boot==1)
					{
						ret = unload_modules();
						sys_game_process_exitspawn2(filename, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_PRIMARY_STACK_SIZE_1M);
						exit(0);
						break;
					}
					ret = unload_modules();
					exit(0);
					break;
					
					}
				else
				{
					sprintf(filename, "%s\n\n%s", menu_list[game_sel].title, text_notfound[region]);
					dialog_ret=0;
					ret = cellMsgDialogOpen2( type_dialog_ok, filename, dialog_fun2, (void*)0x0000aaab, NULL );
					wait_dialog();
					}
				}
			}
	
	}

skip_1:
		cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);
		
		if(!no_video)
		{



			if(game_sel>=0 && *max_list>0 && png_w!=0 && png_h!=0)
				{
				int dispy = 152;
				//sprintf(filename, "GAME_SEL IS %d  MAX IS %d", game_sel, max_menu_list);
				//cellDbgFontPrintf( 0.08f, 0.1f, 1.2f, 0xffffffff, filename);
				
				// calculate highlight position
				
				if(game_sel > 15)
				{
					dispy = 881;
				}
				else
				{
					dispy = 150 + (49 * game_sel-1);
				}
				
				set_texture( text_bmp, DISPLAY_WIDTH, DISPLAY_HEIGHT);
				setRenderTexture();
				display_png(1470, 153, 320, 176, 320, 176);

				set_texture( text_h, DISPLAY_WIDTH, DISPLAY_HEIGHT);
				setRenderTexture();
				display_png(105,dispy, 1315, 49, 1315, 49);

				}

			setRenderColor();


			// square for screen
			set_texture( text_bg, DISPLAY_WIDTH, DISPLAY_HEIGHT);
			setRenderTexture();
			display_png(0,0,1920,1080,1920,1080);
			

			if(mode_list==GAME)
				{
	
				if(game_sel>=0 && max_menu_list>0)
					draw_list( menu_list, max_menu_list, game_sel | (0x10000 * ((menu_list[0].flags & 2048)!=0)));
				//else cellDbgFontPrintf( 0.08f, 0.1f, 1.2f, 0xffffffff, "Put games from BR-DISC");
				}
			else
				{
				if(game_sel>=0 && max_menu_homebrew_list>0)
					draw_list(menu_homebrew_list, max_menu_homebrew_list, game_sel );
				//else cellDbgFontPrintf( 0.08f, 0.1f, 1.2f, 0xffffffff, "Put the homebrew in USBXXX:/homebrew");
				}
		

			if(mode_list==GAME)
				draw_device_list((fdevices | ((game_sel>=0 && max_menu_list>0) ? (menu_list[game_sel].flags<<16) : 0)), hermes, direct_boot, ftp_flags & 2);
			else
				draw_device_list((fdevices | ((game_sel>=0 && max_menu_list>0) ? (menu_list[game_sel].flags<<16) | (1<<31) : 0)), hermes, direct_boot, ftp_flags & 2);
			
		}

		cellDbgFontDrawGcm();
		
		flip();
		cellSysutilCheckCallback();
	}
	if(mode_list==GAME)
		syscall36( (char *) "/dev_bdvd"); // restore bluray
	else
		syscall36( (char *) "/dev_usb000"); // restore

	ret = unload_modules();
	
	sys_process_exit(1);
}


