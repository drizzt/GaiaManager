#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <cell/gcm.h>
#include <cell/dbgfont.h>


typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define CONSOLE_WIDTH		(76+16)
#define CONSOLE_HEIGHT		(31)

#define DISPLAY_WIDTH  1920
#define DISPLAY_HEIGHT 1080


typedef struct {
	unsigned flags;
	char title[64];
	char title_id[64];
	char path[768];
} t_menu_list;

extern void draw_square(float x, float y, float w, float h, float z,
			u32 rgba);

extern int set_texture(u8 * buffer, u32 x_size, u32 y_size);

extern void display_png(int x, int y, int width, int height, int tx,
			int ty);

extern void draw_device_list(u32 flags, int hermes, int payload_type,
			     int direct_boot, int ftp);

extern int initConsole(void);
extern int termConsole(void);
extern int initFont(void);
extern int termFont(void);

extern void initShader(void);
extern int initDisplay(void);
extern void setDrawEnv(void);
extern void setRenderTarget(void);

extern int setRenderObject(void);

extern void setRenderColor(void);
extern void setRenderTexture(void);

extern void flip(void);

extern void draw_list(t_menu_list * menu, int menu_size, int selected);
extern void drawResultWindow(int result, int busy);

extern int DPrintf(const char *string, ...);

#endif
