/* Functions to manage dialogs */

#include <sysutil/sysutil_msgdialog.h>
#include "dialog.h"
#include "graphics.h"

uint32_t type_dialog_yes_no =
	CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BG_VISIBLE | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO |
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON | CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NO;

uint32_t type_dialog_ok =
	CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BG_VISIBLE | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK |
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF | CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_OK;

int dialog_ret = 0;

void dialog_fun1(int button_type, void *userData __attribute__ ((unused)))
{

	switch (button_type) {
	case CELL_MSGDIALOG_BUTTON_YES:
		dialog_ret = 1;
		break;
	case CELL_MSGDIALOG_BUTTON_NO:
	case CELL_MSGDIALOG_BUTTON_ESCAPE:
	case CELL_MSGDIALOG_BUTTON_NONE:
		dialog_ret = 2;
		break;
	default:
		break;
	}
}

void dialog_fun2(int button_type, void *userData __attribute__ ((unused)))
{

	switch (button_type) {
	case CELL_MSGDIALOG_BUTTON_OK:
	case CELL_MSGDIALOG_BUTTON_ESCAPE:
	case CELL_MSGDIALOG_BUTTON_NONE:

		dialog_ret = 1;
		break;
	default:
		break;
	}
}

void wait_dialog(void)
{

	while (!dialog_ret) {
		cellSysutilCheckCallback();
		flip();
	}

	cellMsgDialogAbort();
	setRenderColor();
	sys_timer_usleep(100000);

}
