#ifndef _DIALOG_H
#define _DIALOG_H

extern uint32_t type_dialog_yes_no;
extern uint32_t type_dialog_ok;
extern int dialog_ret;

extern void dialog_fun1(int button_type, void *userData __attribute__ ((unused)));
extern void dialog_fun2(int button_type, void *userData __attribute__ ((unused)));
extern void wait_dialog(void);

#endif
