#ifndef _MAIN_H
#define _MAIN_H

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

extern time_t time_start;
extern uint32_t new_pad, old_pad;

extern int pad_read(void);

#endif
