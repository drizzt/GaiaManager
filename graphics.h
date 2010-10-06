
#include <cell/gcm.h>
#include <cell/dbgfont.h>


typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define CONSOLE_WIDTH		(76+16)
#define CONSOLE_HEIGHT		(31)

#define DISPLAY_WIDTH  1920	
#define DISPLAY_HEIGHT 1080


typedef struct
{
	unsigned flags;
	char title[64];
	char title_id[64];
	char path[768];
}
t_menu_list;

extern char bluray_game[64];

extern unsigned icon_raw[8192];

void put_vertex(float x, float y, float z, u32 color);
void put_texture_vertex(float x, float y, float z, float tx, float ty);

void draw_square(float x, float y, float w, float h, float z, u32 rgba);

int set_texture( u8 *buffer, u32 x_size, u32 y_size );

void display_png(int x, int y, int width, int height, int tx, int ty);
void display_pngh(int x, int y, int width, int height, int tx, int ty);
void display_pngbg(int x, int y, int width, int height, int tx, int ty);

void draw_device_list(u32 flags, int region, int hermes, int direct_boot);

int initConsole();
int termConsole();
int initFont(void);
int termFont(void);

void initShader(void);
int initDisplay(void);
void setDrawEnv(void);
void setRenderTarget(void);

int setRenderObject(void);

void setRenderColor(void);
void setRenderTexture(void);
void setRenderTextureh(void);
void setRenderTexturebg(void);

void flip(void);

void draw_list( t_menu_list *menu, int menu_size, int selected );
void drawResultWindow( int result, int busy );

int DPrintf( const char *string, ... );

