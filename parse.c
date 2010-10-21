/* SFO parsing and managing functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysutil/sysutil_msgdialog.h>
#include "parse.h"
#include "dialog.h"

int parse_ps3_disc(char *path, char *id)
{
	FILE *fp;
	int n;

	fp = fopen(path, "rb");
	if (fp != NULL) {
		unsigned len;
		unsigned char *mem = NULL;

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		mem = (unsigned char *) malloc(len + 16);
		if (!mem) {
			fclose(fp);
			return -2;
		}

		memset(mem, 0, len + 16);

		fseek(fp, 0, SEEK_SET);

		fread((void *) mem, len, 1, fp);

		fclose(fp);

		for (n = 0x20; n < 0x200; n += 0x20) {
			if (!strcmp((char *) &mem[n], "TITLE_ID")) {
				n = (mem[n + 0x12] << 8) | mem[n + 0x13];
				memcpy(id, &mem[n], 16);

				return 0;
			}
		}
	}

	return -1;
}

int parse_param_sfo(char *file, const char *field, char *title_name)
{
	FILE *fp;

	fp = fopen(file, "rb");
	if (fp != NULL) {
		unsigned len, pos, str;
		unsigned char *mem = NULL;

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		mem = (unsigned char *) malloc(len + 16);
		if (!mem) {
			fclose(fp);
			return -2;
		}

		memset(mem, 0, len + 16);

		fseek(fp, 0, SEEK_SET);
		fread((void *) mem, len, 1, fp);

		fclose(fp);

		str = (mem[8] + (mem[9] << 8));
		pos = (mem[0xc] + (mem[0xd] << 8));

		int indx = 0;
// liomajor fix
		while (str < len) {
			if (mem[str] == 0)
				break;

			if (!strcmp((char *) &mem[str], field)) {
				strncpy(title_name, (char *) &mem[pos], 63);
				free(mem);
				return 0;
			}
			while (mem[str])
				str++;
			str++;
			pos += (mem[0x1c + indx] + (mem[0x1d + indx] << 8));
			indx += 16;
		}
		if (mem)
			free(mem);
	}

	return -1;
}

void change_param_sfo_version(char *file)
{
	FILE *fp;
	float ver;

	fp = fopen(file, "rb");
	if (fp != NULL) {
		unsigned len, pos, str;
		unsigned char *mem = NULL;

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		mem = (unsigned char *) malloc(len + 16);
		if (!mem) {
			fclose(fp);
			return;
		}

		memset(mem, 0, len + 16);

		fseek(fp, 0, SEEK_SET);
		fread((void *) mem, len, 1, fp);
		fclose(fp);

		str = (mem[8] + (mem[9] << 8));
		pos = (mem[0xc] + (mem[0xd] << 8));

		int indx = 0;

		while (str < len) {
			if (mem[str] == 0)
				break;

			if (!strcmp((char *) &mem[str], "PS3_SYSTEM_VER")) {
				ver = strtof((char *) &mem[pos], NULL);
				if (ver > 3.41) {
					char msg[128];
					snprintf(msg, sizeof(msg),
							 "This game requires PS3_SYSTEM_VER = %s\nDo you want to try fixing PARAM.SFO forcing 3.41 version?",
							 &mem[pos]);
					dialog_ret = 0;
					cellMsgDialogOpen2(type_dialog_yes_no, msg, dialog_fun1, (void *) 0x0000aaaa, NULL);
					wait_dialog();
					if (dialog_ret == 1) {
						memcpy(&mem[pos], "03.410", 6);
						fp = fopen(file, "wb");
						fwrite(mem, len, 1, fp);
						fclose(fp);
					}
				}
				break;
			}
			while (mem[str])
				str++;
			str++;
			pos += (mem[0x1c + indx] + (mem[0x1d + indx] << 8));
			indx += 16;
		}
		if (mem)
			free(mem);
	}
}
