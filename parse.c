/*
 * Copyright (C) 2010 drizzt
 *
 * Authors:
 * drizzt <drizzt@ibeglab.org>
 * The original OpenBM author
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

/* SFO parsing and managing functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysutil/sysutil_msgdialog.h>
#include "parse.h"
#include "dialog.h"

#define IDX(val, i) ((unsigned int) ((unsigned char *) &val)[i])
#define LE16_TO_BE(val) ( (unsigned short) (    \
          IDX(val, 0) +                         \
          (IDX(val, 1) << 8)))
#define LE32_TO_BE(val) ( (unsigned int) (      \
          IDX(val, 0) +                         \
          (IDX(val, 1) << 8) +                  \
          (IDX(val, 2) << 16) +                 \
          (IDX(val, 3) << 24)))

/* Header of a SFO file */
typedef struct {
  int magic;
  int version;
  int key_table_offset;
  int data_table_offset;
  int items;
} sfo_header_t;

/* Item of a SFO file */
typedef struct {
  short key_offset;
  char unknown;
  char type;
  int size;
  int padded_size;
  int data_offset;
} sfo_key_item_t;

/** Gets the version of the firmware installed in the ps3 were we are running.
  * @return A double with the version number (eg: 3.15)
  */
static double get_system_version(void)
{
	FILE *fp;
	//we read the version from the file version.txt in the flash
	fp = fopen("/dev_flash/vsh/etc/version.txt", "rb");
	if (fp != NULL) {
		char buf[20];
		//read the first line which is the only one we are interested into.
		fgets(buf, 20, fp);
		fclose(fp);
		//skip "version:" and start parsing from there.
		return strtod(buf + 8, NULL);
	}
	//if there was an error return version 0 (shouldn't happen)
	return 0;
}

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
				double ver;		///< Used to store the version required in the param.sfo
				double sys_ver;	///< Used to store the version currently installed in the ps3
				ver = strtod((char *) &mem[pos], NULL);
				sys_ver = get_system_version();
				if (sys_ver && ver > sys_ver) {
					char msg[170];
					snprintf(msg, sizeof(msg),
							 "This game requires PS3_SYSTEM_VER %.2f.\nDo you want to try fixing PARAM.SFO by forcing %.2f version?\n"
							 "WARNING: It will edit PARAM.SFO without doing any backup.", ver, sys_ver);
					dialog_ret = 0;
					cellMsgDialogOpen2(type_dialog_yes_no, msg, dialog_fun1, (void *) 0x0000aaaa, NULL);
					wait_dialog();
					if (dialog_ret == 1) {
						char ver_patch[10];
						//format the version to be patched so it is xx.xxx
						snprintf(ver_patch, sizeof(ver_patch), "%06.3f", sys_ver);
						memcpy(&mem[pos], ver_patch, 6);
						fp = fopen(file, "wb");
						fwrite(mem, len, 1, fp);
						fclose(fp);
					}
				}
				if (!strcmp((char *) &mem[str], "ATTRIBUTE")) {
					if (mem[pos] != 0x5) {
						mem[pos] = 0x5;
						fp = fopen(file, "wb");
						fwrite(mem, len, 1, fp);
						fclose(fp);
					}
					break;
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

/* vim: set ts=4 sw=4 sts=4 tw=120 */
