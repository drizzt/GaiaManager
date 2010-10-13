
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/timer.h>

#include <cell/gcm.h>

#include <sysutil/sysutil_sysparam.h>

#include "graphics.h"
#include <cell/cell_fs.h>
#include <netex/libnetctl.h>


CellDbgFontConsoleId consoleID = CELL_DBGFONT_STDOUT_ID;

#define ROUNDUP(x, a) (((x)+((a)-1))&(~((a)-1)))

static u32 frame_index = 0;

typedef struct
{
	float x, y, z;
	u32 color; 
} vtx_color;

typedef struct {
	float x, y, z;
	float tx, ty;
} vtx_texture;

u32 screen_width;
u32 screen_height; 
float screen_aspect;

u32 color_pitch;
u32 depth_pitch;
u32 color_offset[2];
u32 depth_offset;

uint32_t blockSize;
uint64_t freeSize;
float freeSpace;


// 				Jap, 		English, 	French, 	Spanish, 	German, 		Italian, 	Dutch, 		Portugues, 	Russian, 	Korean, 	Chinese, 	Chinese, 	Finnish, 	Swedish, 	Danish, 	Norwegian
char text_launch[][64] = {	"X - To Launch","X - Launch",	"X - To Launch","X - Ejecutar",	"X - Starten",	"X - Lancia",	"X - To Launch","X - Iniciar",	"X - To Launch","X - To Launch","X - To Launch","X - To Launch","X - To Launch","X - Kor",	"X - To Launch","X - To Launch"};
char text_copy[][64] = {	"O - Copy",	"O - Copy",	"O - Copy",	"O - Copiar",	"O - Kopieren",		"O - Copia",	"O - Copy",	"O - Copiar",	"O - Copy",	"O - Copy",	"O - Copy",	"O - Copy",	"O - Copy",	"O - Kopiera",	"O - Copy",	"O - Copy"};
char text_exit[][64] = {	"/\\ - Exit",	"/\\ - Exit",	"/\\ - Exit",	"/\\ - Salir",	"/\\ - Beenden",		"/\\ - Esci",	"/\\ - Exit",	"/\\ - Sair",	"/\\ - Exit",	"/\\ - Exit",	"/\\ - Exit",	"/\\ - Exit",	"/\\ - Exit",	"/\\ - Avsluta",	"/\\ - Exit",	"/\\ - Exit"};
char text_delete[][64] = {	"[] - Delete",	"[] - Delete",	"[] - Delete",	"[] - Borrar",	"[] - Loschen",		"[] - Cancella",	"[] - Delete",	"[] - Apagar",	"[] - Delete",	"[] - Delete",	"[] - Delete",	"[] - Delete",	"[] - Delete",	"[] - Ta bort",	"[] - Delete",	"[] - Delete"};
char text_refresh[][64] = {	"[] - Refresh",	"[] - Refresh",	"[] - Refresh",	"[] - Refrescar","[] - Aktualisieren",	"[] - Aggiorna",	"[] - Refresh",	"[] - Actualizar","[] - Refresh",	"[] - Refresh","	[] - Refresh",	"[] - Refresh",	"[] - Refresh",	"[] - Uppdatera","[] - Refresh",	"[] - Refresh"};



extern u32 _binary_vpshader_vpo_start;
extern u32 _binary_vpshader_vpo_end;
extern u32 _binary_fpshader_fpo_start;
extern u32 _binary_fpshader_fpo_end;

static unsigned char *vertex_program_ptr = 
(unsigned char *)&_binary_vpshader_vpo_start;
static unsigned char *fragment_program_ptr = 
(unsigned char *)&_binary_fpshader_fpo_start;

static CGprogram vertex_program;
static CGprogram fragment_program;

extern struct _CGprogram _binary_vpshader2_vpo_start;
extern struct _CGprogram _binary_fpshader2_fpo_start;


static void *vertex_program_ucode;
static void *fragment_program_ucode;
static u32 fragment_offset;

static void *text_vertex_prg_ucode;
static void *text_fragment_prg_ucode;
static u32 text_fragment_offset;

static u32 vertex_offset[2];
static u32 color_index;
static u32 position_index;

static vtx_color *vertex_color;
static int vert_indx=0;

static int vert_texture_indx=0;


static u32 text_width;
static u32 text_height;

static u32 text_colorp;
static u32 text_depthp;

static vtx_texture *vertex_text;

static u32 vertex_text_offset;

static u32 text_obj_coord_indx;
static u32 text_tex_coord_indx;

static CGresource tindex;
static CGprogram vertex_prg;
static CGprogram fragment_prg;
static CellGcmTexture text_param;

static u32 local_heap = 0;

static void *localAlloc(const u32 size) 
{
	u32 align = (size + 1023) & (~1023);
	u32 base = local_heap;
	
	local_heap += align;
	return (void*)base;
}

static void *localAllocAlign(const u32 alignment, const u32 size)
{
	local_heap = (local_heap + alignment-1) & (~(alignment-1));
	
	return (void*)localAlloc(size);
}


void draw_square(float x, float y, float w, float h, float z, u32 color)
{
	
	vertex_color[vert_indx].x = x; 
	vertex_color[vert_indx].y = y; 
	vertex_color[vert_indx].z = z; 
	vertex_color[vert_indx].color=color;

	vertex_color[vert_indx+1].x = x+w; 
	vertex_color[vert_indx+1].y = y; 
	vertex_color[vert_indx+1].z = z; 
	vertex_color[vert_indx+1].color=color;

	vertex_color[vert_indx+2].x = x+w; 
	vertex_color[vert_indx+2].y = y-h; 
	vertex_color[vert_indx+2].z = z; 
	vertex_color[vert_indx+2].color=color;

	vertex_color[vert_indx+3].x = x; 
	vertex_color[vert_indx+3].y = y-h; 
	vertex_color[vert_indx+3].z = z; 
	vertex_color[vert_indx+3].color=color;

	cellGcmSetDrawArrays( gCellGcmCurrentContext, CELL_GCM_PRIMITIVE_QUADS, vert_indx, 4);
	vert_indx+=4;

}

void setRenderTarget(void)
{
	CellGcmSurface surface;
	
	surface.colorFormat 	 = CELL_GCM_SURFACE_A8R8G8B8;
	surface.colorTarget		 = CELL_GCM_SURFACE_TARGET_0;
	surface.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
	surface.colorOffset[0] 	 = color_offset[frame_index];
	surface.colorPitch[0] 	 = color_pitch;

	surface.colorLocation[1] = CELL_GCM_LOCATION_LOCAL;
	surface.colorLocation[2] = CELL_GCM_LOCATION_LOCAL;
	surface.colorLocation[3] = CELL_GCM_LOCATION_LOCAL;

	surface.colorOffset[1] 	 = 0;
	surface.colorOffset[2] 	 = 0;
	surface.colorOffset[3] 	 = 0;
	surface.colorPitch[1]	 = 64;
	surface.colorPitch[2]	 = 64;
	surface.colorPitch[3]	 = 64;

	surface.depthFormat 	 = CELL_GCM_SURFACE_Z24S8;
	surface.depthLocation	 = CELL_GCM_LOCATION_LOCAL;
	surface.depthOffset	     = depth_offset;
	surface.depthPitch 	     = depth_pitch;

	surface.type		     = CELL_GCM_SURFACE_PITCH;
	surface.antialias	     = CELL_GCM_SURFACE_CENTER_1;

	surface.width 		     = screen_width;
	surface.height 	 	     = screen_height;
	surface.x 		         = 0;
	surface.y 		         = 0;

	cellGcmSetSurface(gCellGcmCurrentContext, &surface);
}


static void waitFlip(void)
{
	
	while (cellGcmGetFlipStatus()!=0) sys_timer_usleep(500);
	
	cellGcmResetFlipStatus();
}


void flip(void)
{
	static int one=1;

	if (one!=1) waitFlip();
	else cellGcmResetFlipStatus();

	if(cellGcmSetFlip(gCellGcmCurrentContext, frame_index) != CELL_OK) return;
	
	one=0;

	cellGcmFlush(gCellGcmCurrentContext);

	setDrawEnv();
	setRenderColor();

	cellGcmSetWaitFlip(gCellGcmCurrentContext);

	frame_index = (frame_index+1) & 1;
	
	setRenderTarget();

	vert_indx=0; // reset the vertex index
	vert_texture_indx=0;

}

void initShader(void)
{
	vertex_program   = (CGprogram)vertex_program_ptr;
	fragment_program = (CGprogram)fragment_program_ptr;

	cellGcmCgInitProgram(vertex_program);
	cellGcmCgInitProgram(fragment_program);

	u32 ucode_size;
	void *ucode;
	cellGcmCgGetUCode(fragment_program, &ucode, &ucode_size);
	
	void *ret = localAllocAlign(64, ucode_size);
	fragment_program_ucode = ret;
	memcpy(fragment_program_ucode, ucode, ucode_size); 

	cellGcmCgGetUCode(vertex_program, &ucode, &ucode_size);
	vertex_program_ucode = ucode;
}


int text_create( u32 xsize, u32 ysize );

int initDisplay(void)
{
	int ret, i;
	u32 color_size, depth_size, color_depth= 4, z_depth= 4;

	void *color_base_addr, *depth_base_addr, *color_addr[2];

	CellVideoOutResolution resolution;

	CellVideoOutState videoState;
	cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);
	
	ret = cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);
	if (ret != CELL_OK)	return -1;

	cellVideoOutGetResolution(videoState.displayMode.resolutionId, &resolution);

	screen_width = resolution.width;
	screen_height = resolution.height;
	color_pitch = screen_width*color_depth;
	depth_pitch = screen_width*z_depth;
	color_size =   color_pitch*screen_height;
	depth_size =  depth_pitch*screen_height;

	switch (videoState.displayMode.aspect)
		{
		case CELL_VIDEO_OUT_ASPECT_4_3:
			screen_aspect=4.0f/3.0f;
			break;
		case CELL_VIDEO_OUT_ASPECT_16_9:
			screen_aspect=16.0f/9.0f;
			 break;
		default:
			screen_aspect=16.0f/9.0f;
		}

	CellVideoOutConfiguration videocfg;
	memset(&videocfg, 0, sizeof(CellVideoOutConfiguration));
	videocfg.resolutionId = videoState.displayMode.resolutionId;
	videocfg.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	videocfg.pitch = color_pitch;

	ret = cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &videocfg, NULL, 0);
	if (ret != CELL_OK) return -1;

	cellGcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);

	CellGcmConfig config;
	cellGcmGetConfiguration(&config);
	
	local_heap = (u32) config.localAddress;

	color_base_addr = localAllocAlign(16, 2*color_size);

	for (i = 0; i < 2; i++) 
		{
		color_addr[i]= (void *)((u32)color_base_addr+ (i*color_size));
		ret = cellGcmAddressToOffset(color_addr[i], &color_offset[i]);
		if(ret != CELL_OK) return -1;
		ret = cellGcmSetDisplayBuffer(i, color_offset[i], color_pitch, screen_width, screen_height);
		if(ret != CELL_OK) return -1;
		}
		
	depth_base_addr = localAllocAlign(16, depth_size);
	ret = cellGcmAddressToOffset(depth_base_addr, &depth_offset);
	if(ret != CELL_OK) return -1;

	text_create( 1920,1080 );

	return 0;
}

void setDrawEnv(void)
{
	u16 x,y,w,h;
	float min, max;
	float scale[4],offset[4];

	x = 0;
	y = 0;
	w = screen_width;
	h = screen_height;
	min = 0.0f;
	max = 1.0f;
	scale[0] = w * 0.5f;
	scale[1] = h * -0.5f;
	scale[2] = (max - min) * 0.5f;
	scale[3] = 0.0f;
	offset[0] = x + scale[0];
	offset[1] = h - y + scale[1];
	offset[2] = (max + min) * 0.5f;
	offset[3] = 0.0f;

	cellGcmSetColorMask(gCellGcmCurrentContext, CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_A);
	cellGcmSetColorMaskMrt(gCellGcmCurrentContext, 0);

	cellGcmSetViewport(gCellGcmCurrentContext, x, y, w, h, min, max, scale, offset);
	cellGcmSetClearColor(gCellGcmCurrentContext, 0xff000000);

	cellGcmSetDepthTestEnable(gCellGcmCurrentContext, CELL_GCM_TRUE);
	cellGcmSetDepthFunc(gCellGcmCurrentContext, CELL_GCM_LESS);

}

void setRenderColor(void)
{
	cellGcmSetVertexProgram(gCellGcmCurrentContext, vertex_program, vertex_program_ucode);
	cellGcmSetVertexDataArray(gCellGcmCurrentContext, position_index, 0, sizeof(vtx_color), 3, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, (u32)vertex_offset[0]);
	cellGcmSetVertexDataArray(gCellGcmCurrentContext, color_index, 0, sizeof(vtx_color), 4,	CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_LOCAL, (u32)vertex_offset[1]);

	cellGcmSetFragmentProgram(gCellGcmCurrentContext, fragment_program, fragment_offset);

}

int setRenderObject(void)
{

	vertex_color = (vtx_color*) localAllocAlign(128*1024, 1024*sizeof(vtx_color));

	CGparameter position = cellGcmCgGetNamedParameter(vertex_program, "position");
	CGparameter color = cellGcmCgGetNamedParameter(vertex_program, "color");

	position_index = cellGcmCgGetParameterResource(vertex_program, position) - CG_ATTR0;
	color_index = cellGcmCgGetParameterResource(vertex_program, color) - CG_ATTR0;


	if(cellGcmAddressToOffset(fragment_program_ucode, &fragment_offset) != CELL_OK) return -1;

	if (cellGcmAddressToOffset(&vertex_color->x, &vertex_offset[0]) != CELL_OK)	return -1;
	if (cellGcmAddressToOffset(&vertex_color->color, &vertex_offset[1]) != CELL_OK)	return -1;

	return 0;
}

int initFont()
{
	CellDbgFontConfigGcm config;

	int size = CELL_DBGFONT_FRAGMENT_SIZE + CELL_DBGFONT_VERTEX_SIZE * CONSOLE_WIDTH * CONSOLE_HEIGHT + CELL_DBGFONT_TEXTURE_SIZE;
	
	int ret = 0;

	void*localmem = localAllocAlign(128, size);
	if( localmem == NULL ) return -1;	
	
	memset(&config, 0, sizeof(CellDbgFontConfigGcm));

	config.localBufAddr = (sys_addr_t)localmem; 
	config.localBufSize = size;
	config.mainBufAddr = NULL;
	config.mainBufSize  = 0;
	config.option = CELL_DBGFONT_VERTEX_LOCAL;
	config.option |= CELL_DBGFONT_TEXTURE_LOCAL;
	config.option |= CELL_DBGFONT_SYNC_ON;

	ret = cellDbgFontInitGcm(&config);
	if(ret < 0) return ret;

	return 0;
}

int initConsole()
{
	CellDbgFontConsoleConfig config;
	config.posLeft     = 0.086f;
	config.posTop      = 0.16f;
	config.cnsWidth    = CONSOLE_WIDTH;
	config.cnsHeight   = CONSOLE_HEIGHT;
	config.scale       = 0.72f;
	config.color       = 0xffA0A0A0;
	consoleID = cellDbgFontConsoleOpen(&config);

	if (consoleID < 0) return -1; 

return 0;
}

int termConsole(void)
{
	int ret;
	ret = cellDbgFontConsoleClose(consoleID);

	if(ret) return -1;

	consoleID = CELL_DBGFONT_STDOUT_ID;
	
	return ret;
}

int termFont()
{
	int ret;

	ret = cellDbgFontExitGcm();
	
	if(ret) return -1;

	return ret;
}


int DPrintf( const char *string, ... )
{
	int ret;
	va_list argp;
	
	va_start(argp, string);

	ret = cellDbgFontConsoleVprintf(consoleID, string, argp);
	
	va_end(argp);

	return ret;
}

static void utf8_to_ansi(char *utf8, char *ansi, int len)
{
	u8 *ch= (u8 *) utf8;
	u8 c;

	while(*ch!=0 && len>0){

	// 3, 4 bytes utf-8 code 
	if(((*ch & 0xF1)==0xF0 || (*ch & 0xF0)==0xe0) && (*(ch+1) & 0xc0)==0x80){

	*ansi++=' '; // ignore
	len--;
	ch+=2+1*((*ch & 0xF1)==0xF0);
	
	}
	else 
	// 2 bytes utf-8 code	
	if((*ch & 0xE0)==0xc0 && (*(ch+1) & 0xc0)==0x80){
	
	c= (((*ch & 3)<<6) | (*(ch+1) & 63));

	if(c>=0xC0 && c<=0xC5) c='A';
	else if(c==0xc7) c='C';
	else if(c>=0xc8 && c<=0xcb) c='E';
	else if(c>=0xcc && c<=0xcf) c='I';
	else if(c==0xd1) c='N';
	else if(c>=0xd2 && c<=0xd6) c='O';
	else if(c>=0xd9 && c<=0xdc) c='U';
	else if(c==0xdd) c='Y';
	else if(c>=0xe0 && c<=0xe5) c='a';
	else if(c==0xe7) c='c';
	else if(c>=0xe8 && c<=0xeb) c='e';
	else if(c>=0xec && c<=0xef) c='i';
	else if(c==0xf1) c='n';
	else if(c>=0xf2 && c<=0xf6) c='o';
	else if(c>=0xf9 && c<=0xfc) c='u';
	else if(c==0xfd || c==0xff) c='y';
	else if(c>127) c=*(++ch+1); //' ';

	*ansi++=c;
	len--;
	ch++;
	
	}
	else {
	
	if(*ch<32) *ch=32;
	*ansi++=*ch;
	
	len--;

	}

	ch++;
	}
	while(len>0) {
	*ansi++=0;
	len--;
	}
}

static void draw_text_stroke(float x, float y, float size, u32 color, const char *str)
{

		cellDbgFontPrintf( x-.001f, y+.001f,size, 0xff000000, str);
		cellDbgFontPrintf( x-.001f, y-.001f,size, 0xff000000, str);

		cellDbgFontPrintf( x+.001f, y+.001f,size, 0xff000000, str);
		cellDbgFontPrintf( x+.001f, y-.001f,size, 0xff000000, str);
		cellDbgFontPrintf( x+.002f, y+.002f,size, 0xff000000, str);


		cellDbgFontPrintf( x, y,size, color, str);
}

static void draw_text_bool(float x, float y, float size, bool on)
{
	u32 color = on ? 0xff00ff00 : 0xff0000ff;
	cellDbgFontPrintf(x, y, size, color, on ? "ON" : "OFF");
}


void draw_device_list(u32 flags, int hermes, int direct_boot, int ftp)
{
	float y = 0.84f;
	char str[256];

	int n,ok=0;
	float len;
	float x=0.06;
	
	char sizer[255];
	char path[255];
	char ipaddr[64];
	union CellNetCtlInfo info;
	
	for(n=0;n<12;n++)
		{
		
		ok=0;
		if((flags>>n) & 1) ok=1;
	   
		if(ok)
			{
			switch(n)
				{
				case 0:
					sprintf(str, "%s", "hdd0");
					sprintf(path, "/dev_hdd0/");
					break;
				case 11:
					sprintf(str, "%s", "bdvd");
					sprintf(path, "/dev_bdvd/");
					break;
				default:
					sprintf(str, "usb%i", n-1);
					sprintf(path, "/dev_usb00%d/", n-1);
					break;
				}

			len=(float) 0.017*strlen(str);

			cellFsGetFreeSize(path, &blockSize, &freeSize);
			freeSpace = ( ((uint64_t)blockSize * freeSize));
			freeSpace = freeSpace / 1073741824.00;
			sprintf(sizer, "%.2fGB", freeSpace);
			
			draw_text_stroke( x, y+0.064, 1.0f, ((flags>>(n+16)) & 1) ? 0xff00ffff : 0xffffffff, str);
			x+=len-0.01;
			draw_text_stroke( x, y+0.064, 1.0f, ((flags>>(n+16)) & 1) ? 0xff999999 : 0xff999999, sizer);
			len=(float) 0.017*strlen(sizer);

			x+=len;
 
			}
		}

		cellNetCtlGetInfo(CELL_NET_CTL_INFO_IP_ADDRESS, &info);
		sprintf(ipaddr, "%16s", info.ip_address);

		cellDbgFontPrintf(0.775, 0.432, 0.8, 0xffffffff, "Launch Selected");

		// on GAME mode
		if (((flags>>31) & 1) == 0)
		{
			cellDbgFontPrintf(0.775, 0.472, 0.8, 0xffffffff, "Backup BDVD");
			cellDbgFontPrintf(0.775, 0.515, 0.8, 0xffffffff, "Copy Selected");
			cellDbgFontPrintf(0.775, 0.555, 0.8, 0xffffffff, "Delete Selected");
			cellDbgFontPrintf(0.775, 0.595, 0.8, 0xffffffff, "Refresh List");
			cellDbgFontPrintf(0.775, 0.635, 0.8, 0xffffffff, "Check Selected");
			//cellDbgFontPrintf(0.775, 0.68, 0.8, 0xffffffff, "Mem Patch:");
			cellDbgFontPrintf(0.775, 0.68, 0.8, 0xffffffff, "Disc Less:");
			draw_text_bool	 (0.895, 0.68, 0.8, hermes);
			cellDbgFontPrintf(0.775, 0.72, 0.8, 0xffffffff, "Direct Boot:");
			draw_text_bool   (0.895, 0.72, 0.8, direct_boot);
		}
		else
		{
			cellDbgFontPrintf(0.775, 0.472, 0.8, 0xffffffff, "Delete Selected");
			cellDbgFontPrintf(0.775, 0.515, 0.8, 0xffffffff, "Refresh List");
		}
		
		cellDbgFontPrintf(0.775, 0.76, 0.8, 0xffffffff, "FTP Server:");
		draw_text_bool   (0.895, 0.76, 0.8, ftp);
		cellDbgFontPrintf(0.775, 0.80, 0.8, 0xffffffff, "OM Mode:");
		cellDbgFontPrintf(0.86, 0.80, 0.8, 0xffffffff, (flags>>31) & 1 ? "HOMEBREW" : "GAME");
		cellDbgFontPrintf(0.775, 0.84, 0.8, 0xffffffff, "IP");
		cellDbgFontPrintf(0.771, 0.84, 0.8, 0xff00ffff, ipaddr);
}
		

void draw_list( t_menu_list *menu, int menu_size, int selected )
{
	float y = 0.145f;
	int i = 0, c=0;
	char str[256];
	char ansi[256];

	u32 color;
	u32 black;
	black = 0xff000000;
	int flagb= selected & 0x10000;

	selected&= 0xffff;
	

	if(selected>15) i=selected-15;

	while( selected < menu_size && c<16 ) 
		{

		int grey=0;
		
		if(i<menu_size)
			{
			utf8_to_ansi(menu[i].title, ansi, 65);
			ansi[65]=0;
			//sprintf(str, "%i) %s", i+1, ansi);
			sprintf(str, "%s", ansi);
			grey=(menu[i].title[0]=='_');
			}
		else
			{
			sprintf(str, " ");
			}
		
		color= 0xff606060;

		if(i==selected)
			color= (flagb && i==0) ? 0xfff0e000 : ((grey==0) ? 0xff00ffff : 0xff008080);
		else
			color= (flagb && i==0)? 0xff807000 : ((grey==0) ?  0xffffffff : 0xff606060);
		
		draw_text_stroke(0.06f, y, 1.05f, color, str);

		y += 0.0455f;
		i++; c++;
		}
}


static void init_text_shader( void )
{

	void *ucode;
	u32 ucode_size;

	vertex_prg = &_binary_vpshader2_vpo_start;
	fragment_prg = &_binary_fpshader2_fpo_start;

	cellGcmCgInitProgram( vertex_prg );
	cellGcmCgInitProgram( fragment_prg );
	
	cellGcmCgGetUCode( fragment_prg, &ucode, &ucode_size );

	text_fragment_prg_ucode = localAllocAlign(64, ucode_size );

	cellGcmAddressToOffset( text_fragment_prg_ucode, &text_fragment_offset );

	memcpy( text_fragment_prg_ucode, ucode, ucode_size );

	cellGcmCgGetUCode( vertex_prg, &text_vertex_prg_ucode, &ucode_size );

}


int text_create( u32 xsize, u32 ysize )
{
	u32 color_size;
	u32 color_limit;
	u32 depth_size;
	u32 depth_limit;
	u32 buffer_width;

	
	text_width = xsize;
	text_height =ysize;

	buffer_width = ROUNDUP( text_width, 64 );


	text_colorp = cellGcmGetTiledPitchSize( buffer_width*4 );
	if( text_colorp == 0 ) return -1;
	
	text_depthp = cellGcmGetTiledPitchSize( buffer_width*4 );
	if( text_depthp == 0 ) return -1;
	
	color_size = text_colorp*ROUNDUP( text_height, 64 );
	color_limit = ROUNDUP( 2*color_size, 0x10000 );
	
	depth_size = text_depthp*ROUNDUP( text_height, 64 );
	depth_limit = ROUNDUP( depth_size, 0x10000 );

	init_text_shader();


	vertex_text = (vtx_texture*) localAllocAlign(128*1024, 1024*sizeof(vtx_texture));

	cellGcmAddressToOffset( (void*)vertex_text,&vertex_text_offset );

	text_param.format  = CELL_GCM_TEXTURE_A8R8G8B8;
	text_param.format |= CELL_GCM_TEXTURE_LN;

	text_param.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 | CELL_GCM_TEXTURE_REMAP_REMAP << 12 | CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
		CELL_GCM_TEXTURE_REMAP_REMAP <<  8 | CELL_GCM_TEXTURE_REMAP_FROM_G << 6 | CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
		CELL_GCM_TEXTURE_REMAP_FROM_A << 2 | CELL_GCM_TEXTURE_REMAP_FROM_B;

	text_param.mipmap = 1;
	text_param.cubemap = CELL_GCM_FALSE;
	text_param.dimension = CELL_GCM_TEXTURE_DIMENSION_2;

	CGparameter objCoord = cellGcmCgGetNamedParameter( vertex_prg, "a2v.objCoord" );
	if( objCoord == 0 ) return -1;

	CGparameter texCoord = cellGcmCgGetNamedParameter( vertex_prg, "a2v.texCoord" );
	if( texCoord == 0) return -1;

	CGparameter texture = cellGcmCgGetNamedParameter( fragment_prg, "texture" );

	if( texture == 0 ) return -1;
	
	text_obj_coord_indx =  cellGcmCgGetParameterResource( vertex_prg, objCoord) - CG_ATTR0;
	text_tex_coord_indx = cellGcmCgGetParameterResource( vertex_prg, texCoord) - CG_ATTR0;
	tindex = (CGresource) (cellGcmCgGetParameterResource( fragment_prg, texture ) - CG_TEXUNIT0 );

	return 0;

}

int set_texture( u8 *buffer, u32 x_size, u32 y_size )
{

	int ret;
	u32 buf_offs;

	ret = cellGcmAddressToOffset( buffer, &buf_offs );
	if( CELL_OK != ret ) return ret;

	text_param.depth  = 1;
	text_param.width  = x_size;
	text_param.height = y_size;
	text_param.pitch  = x_size*4;
	text_param.offset = buf_offs;
	text_param.location = CELL_GCM_LOCATION_MAIN;

	
	cellGcmSetTexture( gCellGcmCurrentContext, tindex, &text_param );

	cellGcmSetTextureControl( gCellGcmCurrentContext, tindex, 1, 0, 15, 1 );

	cellGcmSetTextureAddress( gCellGcmCurrentContext, tindex, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, 
		CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, CELL_GCM_TEXTURE_ZFUNC_LESS, 0 );

	cellGcmSetTextureFilter( gCellGcmCurrentContext, tindex, 0, CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX );

	return ret;

}

void setRenderTexture( void )
{
	cellGcmSetVertexProgram( gCellGcmCurrentContext, vertex_prg, text_vertex_prg_ucode );
	cellGcmSetFragmentProgram( gCellGcmCurrentContext, fragment_prg, text_fragment_offset);
	cellGcmSetInvalidateTextureCache( gCellGcmCurrentContext, CELL_GCM_INVALIDATE_TEXTURE );
	cellGcmSetVertexDataArray( gCellGcmCurrentContext, text_obj_coord_indx, 0, sizeof(vtx_texture), 3, CELL_GCM_VERTEX_F,
							   CELL_GCM_LOCATION_LOCAL, vertex_text_offset );
	cellGcmSetVertexDataArray( gCellGcmCurrentContext, text_tex_coord_indx, 0, sizeof(vtx_texture), 2, CELL_GCM_VERTEX_F,
	                           CELL_GCM_LOCATION_LOCAL, ( vertex_text_offset + sizeof(float)*3 ) );
	return;
}


//
// fixed by w4r10ck
//
void display_png(int x, int y, int width, int height, int tx, int ty)
{
    vertex_text[vert_texture_indx].x= ((float) ((x)*2))/((float) 1920)-1.0f;
    vertex_text[vert_texture_indx].y= ((float) ((y)*-2))/((float) 1080)+1.0f;
    vertex_text[vert_texture_indx].z= 0.0f;
    vertex_text[vert_texture_indx].tx= 0.0f;
    vertex_text[vert_texture_indx].ty= 0.0f;

    vertex_text[vert_texture_indx+1].x= ((float) ((x+width)*2))/((float) 1920)-1.0f;
    vertex_text[vert_texture_indx+1].y= ((float) ((y)*-2))/((float) 1080)+1.0f;
    vertex_text[vert_texture_indx+1].z= 0.0f;
    vertex_text[vert_texture_indx+1].tx= ((float) tx)/1920.0f;
    vertex_text[vert_texture_indx+1].ty= 0.0f;

    vertex_text[vert_texture_indx+2].x= ((float) ((x)*2))/((float) 1920)-1.0f;
    vertex_text[vert_texture_indx+2].y= ((float) ((y+height)*-2))/((float) 1080)+1.0f;
    vertex_text[vert_texture_indx+2].z= 0.0f;
    vertex_text[vert_texture_indx+2].tx= 0.0f;
    vertex_text[vert_texture_indx+2].ty= ((float) ty)/1080.0f;

    vertex_text[vert_texture_indx+3].x= ((float) ((x+width)*2))/((float) 1920)-1.0f;
    vertex_text[vert_texture_indx+3].y= ((float) ((y+height)*-2))/((float) 1080)+1.0f;
    vertex_text[vert_texture_indx+3].z= 0.0f;
    vertex_text[vert_texture_indx+3].tx= ((float) tx)/1920.0f;
    vertex_text[vert_texture_indx+3].ty=((float) ty)/1080.0f;

    cellGcmSetDrawArrays( gCellGcmCurrentContext, CELL_GCM_PRIMITIVE_TRIANGLE_STRIP, vert_texture_indx, 4 );
    vert_texture_indx+=4;
}
