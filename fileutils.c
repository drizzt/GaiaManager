#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cell/cell_fs.h>
#include <sysutil/sysutil_msgdialog.h>

#include "dialog.h"
#include "fileutils.h"
#include "graphics.h"
#include "main.h"

/****************************************************/
/* FILE UTILS                                       */
/****************************************************/

#define MAX_FAST_FILES 32

typedef struct _t_fast_files {
	int64_t readed;				// global bytes readed
	int64_t writed;				// global bytes writed
	int64_t off_readed;			// offset correction for bigfiles_mode == 2  (joining)
	int64_t len;				// global len of the file (value increased in the case of bigfiles_ mode == 2)

	int giga_counter;			// counter for split files to 1GB for bigfiles_mode == 1 (split)
	uint32_t fl;				// operation control
	int bigfile_mode;
	int pos_path;				// filename position used in bigfiles

	char pathr[1024];			// read path 
	char pathw[1024];			// write path

	int use_doublebuffer;		// if files >= 4MB use_doblebuffer =1;

	void *mem;					// buffer for read/write files ( x2 if use_doublebuffer is fixed)
	int size_mem;				// size of the buffer for read

	int number_frag;			// used to count fragments files i bigfile_mode

	CellFsAio t_read;			// used for async read
	CellFsAio t_write;			// used for async write

} t_fast_files __attribute__ ((aligned(8)));

static t_fast_files *fast_files = NULL;
static int fast_num_files = 0;
static int fast_used_mem = 0;
static int current_fast_file_r = 0;
static int current_fast_file_w = 0;
static int fast_read = 0, fast_writing = 0;
static int files_opened = 0;
int num_directories = 0, num_files_big = 0, num_files_split = 0;
int abort_copy = 0;				// abort process
int file_counter = 0;			// to count files
int copy_mode = 0;				// 0- normal 1-> pack files >= 4GB
int copy_is_split = 0;			// return 1 if files is split
int64_t global_device_bytes = 0, copy_global_bytes = 0;

static char string1[256];

static int fast_copy_async(char *pathr, char *pathw, int enable)
{

	fast_num_files = 0;

	fast_read = 0;
	fast_writing = 0;

	fast_used_mem = 0;
	files_opened = 0;

	current_fast_file_r = current_fast_file_w = 0;

	if (enable) {
		if (cellFsAioInit(pathr) != CELL_FS_SUCCEEDED)
			return -1;
		if (cellFsAioInit(pathw) != CELL_FS_SUCCEEDED)
			return -1;

		fast_files = (t_fast_files *) memalign(8, sizeof(t_fast_files) * MAX_FAST_FILES);
		if (!fast_files)
			return -2;
		return 0;
	} else {
		if (fast_files)
			free(fast_files);
		fast_files = NULL;
		cellFsAioFinish(pathr);
		cellFsAioFinish(pathw);
	}

	return 0;

}

static int fast_copy_process(void);

static int fast_copy_add(char *pathr, char *pathw, char *file)
{
	int size_mem;

	int strl = strlen(file);

	struct stat s;

	if (fast_num_files >= MAX_FAST_FILES || fast_used_mem >= 0x800000) {
		int ret = fast_copy_process();

		if (ret < 0 || abort_copy)
			return ret;

	}

	if (fast_num_files >= MAX_FAST_FILES) {
		return -1;
	}

	fast_files[fast_num_files].bigfile_mode = 0;

	if (strl > 6) {
		char *p = file;
		p += strl - 6;			// adjust for .666xx
		if (p[0] == '.' && p[1] == '6' && p[2] == '6' && p[3] == '6') {
			if (p[4] != '0' || p[5] != '0') {
				return 0;
			}					// ignore this files
			fast_files[fast_num_files].bigfile_mode = 2;	// joining split files

		}

	}
	sprintf(fast_files[fast_num_files].pathr, "%s/%s", pathr, file);

	if (stat(fast_files[fast_num_files].pathr, &s) < 0) {
		abort_copy = 1;
		return -1;
	}

	sprintf(fast_files[fast_num_files].pathw, "%s/%s", pathw, file);

	// zero files
	if ((int64_t) s.st_size == 0LL) {
		int fdw;

		if (cellFsOpen
			(fast_files[fast_num_files].pathw, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdw, 0,
			 0) != CELL_FS_SUCCEEDED) {
			DPrintf("Error Opening0 (write):\n%s\n\n", fast_files[current_fast_file_r].pathw);
			abort_copy = 1;
			return -1;
		}
		cellFsClose(fdw);

		cellFsChmod(fast_files[fast_num_files].pathw, CELL_FS_S_IFMT | 0777);
		DPrintf("Copying %s\nwWrote 0 B\n", fast_files[current_fast_file_r].pathr);
		file_counter++;
		return 0;
	}

	if (fast_files[fast_num_files].bigfile_mode == 2) {
		fast_files[fast_num_files].pathw[strlen(fast_files[fast_num_files].pathw) - 6] = 0;	// truncate the extension
		fast_files[fast_num_files].pos_path = strlen(fast_files[fast_num_files].pathr) - 6;
		fast_files[fast_num_files].pathr[fast_files[fast_num_files].pos_path] = 0;	// truncate the extension
	}

	if (copy_mode == 1) {
		if (((int64_t) s.st_size) >= 0x100000000LL) {
			fast_files[fast_num_files].bigfile_mode = 1;
			fast_files[fast_num_files].pos_path = strlen(fast_files[fast_num_files].pathw);
			fast_files[fast_num_files].giga_counter = 0;

			copy_is_split = 1;
		}

	}

	fast_files[fast_num_files].number_frag = 0;
	fast_files[fast_num_files].fl = 1;

	fast_files[fast_num_files].len = (int64_t) s.st_size;
	fast_files[fast_num_files].use_doublebuffer = 0;
	fast_files[fast_num_files].readed = 0LL;
	fast_files[fast_num_files].writed = 0LL;

	fast_files[fast_num_files].t_read.fd = -1;
	fast_files[fast_num_files].t_write.fd = -1;

	if (((int64_t) s.st_size) >= 0x400000LL) {
		size_mem = 0x400000;
		fast_files[fast_num_files].use_doublebuffer = 1;
	} else
		size_mem = ((int) s.st_size);

	fast_files[fast_num_files].mem =
		memalign(32, size_mem + size_mem * (fast_files[fast_num_files].use_doublebuffer != 0) + 1024);
	fast_files[fast_num_files].size_mem = size_mem;

	if (!fast_files[fast_num_files].mem) {
		abort_copy = 1;
		return -1;
	}

	fast_used_mem += size_mem;

	fast_num_files++;

	return 0;
}

static void fast_func_read(CellFsAio * xaio, CellFsErrno error, int xid __attribute__ ((unused)), uint64_t size)
{
	t_fast_files *fi = (t_fast_files *) (uint32_t) xaio->user_data;

	if (error != 0 || size != xaio->size) {
		fi->readed = -1;
		return;
	} else
		fi->readed += (int64_t) size;

	fast_read = 0;
	fi->fl = 3;

}

static void fast_func_write(CellFsAio * xaio, CellFsErrno error, int xid __attribute__ ((unused)), uint64_t size)
{
	t_fast_files *fi = (t_fast_files *) (uint32_t) xaio->user_data;

	if (error != 0 || size != xaio->size) {
		fi->writed = -1;
	} else {

		fi->writed += (int64_t) size;
		fi->giga_counter += (int) size;
		global_device_bytes += (int64_t) size;
	}

	fast_writing = 2;
}

static int fast_copy_process()
{

	int n;

	int fdr, fdw;

	static int id_r = -1, id_w = -1;

	static int last_copy_percent, last_file_percent;

	int error = 0;

	int i_reading = 0;

	int64_t write_end = 0, write_size = 0;

//  static int inc;

	while (current_fast_file_w < fast_num_files || fast_writing) {

		if (abort_copy)
			break;

		// open read
		if (current_fast_file_r < fast_num_files && fast_files[current_fast_file_r].fl == 1 && !i_reading && !fast_read) {

			fast_files[current_fast_file_r].readed = 0LL;
			fast_files[current_fast_file_r].writed = 0LL;
			fast_files[current_fast_file_r].off_readed = 0LL;

			fast_files[current_fast_file_r].t_read.fd = -1;
			fast_files[current_fast_file_r].t_write.fd = -1;

			if (fast_files[current_fast_file_r].bigfile_mode == 1) {
				DPrintf("Split file >= 4GB\n %s\n", fast_files[current_fast_file_r].pathr);
				sprintf(&fast_files[current_fast_file_r].pathw[fast_files[current_fast_file_r].pos_path], ".666%2.2i",
						fast_files[current_fast_file_r].number_frag);
			}

			if (fast_files[current_fast_file_r].bigfile_mode == 2) {
				DPrintf("Joining file >= 4GB\n %s\n", fast_files[current_fast_file_r].pathw);
				sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".666%2.2i",
						fast_files[current_fast_file_r].number_frag);
			}
			//DPrintf("Open R: %s\nOpen W: %s, Index %i/%i\n", fast_files[current_fast_file_r].pathr,
			//      fast_files[current_fast_file_r].pathw, current_fast_file_r, fast_num_files);

			if (cellFsOpen(fast_files[current_fast_file_r].pathr, CELL_FS_O_RDONLY, &fdr, 0, 0) != CELL_FS_SUCCEEDED) {
				DPrintf("Error Opening (read):\n%s\n\n", fast_files[current_fast_file_r].pathr);
				error = -1;
				break;
			} else
				files_opened++;
			if (cellFsOpen
				(fast_files[current_fast_file_r].pathw, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdw, 0,
				 0) != CELL_FS_SUCCEEDED) {
				DPrintf("Error Opening (write):\n%s\n\n", fast_files[current_fast_file_r].pathw);
				error = -2;
				break;
			} else
				files_opened++;

			if (fast_files[current_fast_file_r].bigfile_mode == 0) {
			}
			//DPrintf("Copying %s\n", fast_files[current_fast_file_r].pathr);
			if (fast_files[current_fast_file_r].bigfile_mode)
				DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_r].number_frag);

			fast_files[current_fast_file_r].t_read.fd = fdr;

			fast_files[current_fast_file_r].t_read.offset = 0LL;
			fast_files[current_fast_file_r].t_read.buf = fast_files[current_fast_file_r].mem;

			fast_files[current_fast_file_r].t_read.size =
				fast_files[current_fast_file_r].len - fast_files[current_fast_file_r].readed;
			if ((int64_t)
				fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
				fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

			fast_files[current_fast_file_r].t_read.user_data = (uint32_t) & fast_files[current_fast_file_r];

			fast_files[current_fast_file_r].t_write.fd = fdw;
			fast_files[current_fast_file_r].t_write.user_data = (uint32_t) & fast_files[current_fast_file_r];
			fast_files[current_fast_file_r].t_write.offset = 0LL;
			if (fast_files[current_fast_file_r].use_doublebuffer)
				fast_files[current_fast_file_r].t_write.buf = ((char *)
															   fast_files[current_fast_file_r].mem) +
					fast_files[current_fast_file_r].size_mem;
			else
				fast_files[current_fast_file_r].t_write.buf = fast_files[current_fast_file_r].mem;

			fast_read = 1;
			fast_files[current_fast_file_r].fl = 2;
			if (cellFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read) != 0) {
				id_r = -1;
				error = -3;
				DPrintf("Fail to perform Async Read\n\n");
				fast_read = 0;
				break;
			}

			i_reading = 1;

		}
		// fast read end

		if (current_fast_file_r < fast_num_files && fast_files[current_fast_file_r].fl == 3 && !fast_writing) {
			id_r = -1;
			//fast_read=0;

			if (fast_files[current_fast_file_r].readed < 0LL) {
				DPrintf("Error Reading %s\n", fast_files[current_fast_file_r].pathr);
				error = -3;
				break;
			}
			// double buffer

			if (fast_files[current_fast_file_r].use_doublebuffer) {
				//DPrintf("Double Buff Write\n");

				current_fast_file_w = current_fast_file_r;

				memcpy(((char *)
						fast_files[current_fast_file_r].mem) + fast_files[current_fast_file_r].size_mem,
					   fast_files[current_fast_file_r].mem, fast_files[current_fast_file_r].size_mem);

				fast_files[current_fast_file_w].t_write.size = fast_files[current_fast_file_r].t_read.size;

				if (fast_files[current_fast_file_w].bigfile_mode == 1)
					fast_files[current_fast_file_w].t_write.offset = (int64_t)
						fast_files[current_fast_file_w].giga_counter;
				else
					fast_files[current_fast_file_w].t_write.offset = fast_files[current_fast_file_w].writed;

				fast_writing = 1;

				if (cellFsAioWrite(&fast_files[current_fast_file_w].t_write, &id_w, fast_func_write) != 0) {
					id_w = -1;
					error = -4;
					DPrintf("Fail to perform Async Write\n\n");
					fast_writing = 0;
					break;
				}

				if (fast_files[current_fast_file_r].readed < fast_files[current_fast_file_r].len) {
					fast_files[current_fast_file_r].t_read.size =
						fast_files[current_fast_file_r].len - fast_files[current_fast_file_r].readed;
					if ((int64_t)
						fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
						fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

					fast_files[current_fast_file_r].fl = 2;
					fast_files[current_fast_file_r].t_read.offset =
						fast_files[current_fast_file_r].readed - fast_files[current_fast_file_r].off_readed;

					fast_read = 1;
					if (cellFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read) != 0) {
						id_r = -1;
						error = -3;
						DPrintf("Fail to perform Async Read\n\n");
						fast_read = 0;
						break;
					}
				} else {
					if (fast_files[current_fast_file_r].bigfile_mode == 2) {
						struct stat s;

						fast_files[current_fast_file_r].number_frag++;

						fast_files[current_fast_file_r].off_readed = fast_files[current_fast_file_r].readed;

						DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_r].number_frag);
						sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path],
								".666%2.2i", fast_files[current_fast_file_r].number_frag);

						if (stat(fast_files[current_fast_file_r].pathr, &s) < 0) {
							current_fast_file_r++;
							i_reading = 0;
						} else {
							if (fast_files[current_fast_file_r].t_read.fd >= 0) {
								cellFsClose(fast_files[current_fast_file_r].t_read.fd);
								files_opened--;
							}
							fast_files[current_fast_file_r].t_read.fd = -1;

							if (cellFsOpen(fast_files[current_fast_file_r].pathr, CELL_FS_O_RDONLY, &fdr, 0, 0) !=
								CELL_FS_SUCCEEDED) {
								DPrintf("Error Opening (read):\n%s\n\n", fast_files[current_fast_file_r].pathr);
								error = -1;
								break;
							} else
								files_opened++;

							fast_files[current_fast_file_r].t_read.fd = fdr;

							fast_files[current_fast_file_r].len += (int64_t)
								s.st_size;

							fast_files[current_fast_file_r].t_read.offset = 0LL;
							fast_files[current_fast_file_r].t_read.buf = fast_files[current_fast_file_r].mem;

							fast_files[current_fast_file_r].t_read.size =
								fast_files[current_fast_file_r].len - fast_files[current_fast_file_r].readed;
							if ((int64_t)
								fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
								fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

							fast_files[current_fast_file_r].t_read.user_data =
								(uint32_t) & fast_files[current_fast_file_r];

							fast_read = 1;
							if (cellFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read)
								!= 0) {
								id_r = -1;
								error = -3;
								DPrintf("Fail to perform Async Read\n\n");
								fast_read = 0;
								break;
							}

							fast_files[current_fast_file_r].fl = 2;

						}
					} else {
						fast_files[current_fast_file_r].fl = 5;
						current_fast_file_r++;
						i_reading = 0;
					}

				}

			} else
				// single buffer        
			{
				//DPrintf("Single Buff Write\n");

				current_fast_file_w = current_fast_file_r;
				fast_files[current_fast_file_w].t_write.size = fast_files[current_fast_file_r].t_read.size;

				fast_files[current_fast_file_w].t_write.offset = fast_files[current_fast_file_w].writed;

				fast_writing = 1;

				if (cellFsAioWrite(&fast_files[current_fast_file_w].t_write, &id_w, fast_func_write) != 0) {
					id_w = -1;
					error = -4;
					DPrintf("Fail to perform Async Write\n\n");
					fast_writing = 0;
					break;
				}

				current_fast_file_r++;
				i_reading = 0;
			}
		}
		// fast write end
		if (fast_writing > 1) {
			fast_writing = 0;
			id_w = -1;

			if (fast_files[current_fast_file_w].writed < 0LL) {
				DPrintf("Error Writing %s\n", fast_files[current_fast_file_w].pathw);
				error = -4;
				break;
			}

			write_end = fast_files[current_fast_file_w].writed;
			write_size = fast_files[current_fast_file_w].len;

			if (fast_files[current_fast_file_w].writed >= fast_files[current_fast_file_w].len) {
				if (fast_files[current_fast_file_w].t_read.fd >= 0) {
					cellFsClose(fast_files[current_fast_file_w].t_read.fd);
					files_opened--;
				}
				fast_files[current_fast_file_w].t_read.fd = -1;
				if (fast_files[current_fast_file_w].t_write.fd >= 0) {
					cellFsClose(fast_files[current_fast_file_w].t_write.fd);
					files_opened--;
				}
				fast_files[current_fast_file_w].t_write.fd = -1;

				cellFsChmod(fast_files[current_fast_file_w].pathw, CELL_FS_S_IFMT | 0777);

				if (fast_files[current_fast_file_w].bigfile_mode == 1) {
					fast_files[current_fast_file_w].pathw[fast_files[current_fast_file_w].pos_path]
						= 0;
				}
#if 0
				if (write_size < 1024LL) {
				}
				//DPrintf("Wrote %lli B %s\n\n", write_size, fast_files[current_fast_file_w].pathw);
				else if (write_size < 0x100000LL)
					DPrintf("Wrote %lli KB %s\n\n", write_size / 1024LL, fast_files[current_fast_file_w].pathw);
				else
					DPrintf("Wrote %lli MB %s\n\n", write_size / 0x100000LL, fast_files[current_fast_file_w].pathw);
#endif

				fast_files[current_fast_file_w].fl = 4;	//end of proccess

				fast_files[current_fast_file_w].writed = -1LL;
				current_fast_file_w++;
				//if(current_fast_file_r<current_fast_file_w) current_fast_file_w=current_fast_file_r;
				file_counter++;
				cellMsgDialogProgressBarReset(CELL_MSGDIALOG_PROGRESSBAR_INDEX_DOUBLE_UPPER);
			} else
				// split big files
			if (fast_files[current_fast_file_w].bigfile_mode == 1
					&& fast_files[current_fast_file_w].giga_counter >= 0x40000000) {
				if (fast_files[current_fast_file_w].t_write.fd >= 0) {
					cellFsClose(fast_files[current_fast_file_w].t_write.fd);
					files_opened--;
				}
				fast_files[current_fast_file_w].t_write.fd = -1;

				cellFsChmod(fast_files[current_fast_file_w].pathw, CELL_FS_S_IFMT | 0777);

				fast_files[current_fast_file_w].giga_counter = 0;
				fast_files[current_fast_file_w].number_frag++;
				sprintf(&fast_files[current_fast_file_w].pathw[fast_files[current_fast_file_w].pos_path], ".666%2.2i",
						fast_files[current_fast_file_w].number_frag);
				DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_w].number_frag);

				if (cellFsOpen
					(fast_files[current_fast_file_w].pathw, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdw,
					 0, 0) != CELL_FS_SUCCEEDED) {
					DPrintf("Error Opening2 (write):\n%s\n\n", fast_files[current_fast_file_w].pathw);
					error = -2;
					break;
				} else
					files_opened++;

				fast_files[current_fast_file_w].t_write.fd = fdw;
			}

		}

		sprintf(string1, "copying %s to %s", fast_files[current_fast_file_r].pathr, fast_files[current_fast_file_r].pathw);
		cellMsgDialogProgressBarSetMsg(CELL_MSGDIALOG_PROGRESSBAR_INDEX_DOUBLE_UPPER, string1);

		//int seconds = (int) (time(NULL) - time_start);
		
		int copy_percent = (int) (global_device_bytes * 100 / copy_global_bytes);
		int file_percent = (int) (write_end * 100 / write_size);

		/*sprintf(string1, "Copying. File: %i Time: %2.2i:%2.2i:%2.2i %2.2i/100 Vol: %1.2f/%1.2f GB %d\n", file_counter,
				seconds / 3600, (seconds / 60) % 60, seconds % 60, (int) (write_end * 100ULL / write_size),
				((double) global_device_bytes) / (1024.0 * 1024. * 1024.0),
				(double) copy_global_bytes / (1024.0 * 1024. * 1024.0), copy_percent);
		*/

		/* New file or new copy session*/
		if (copy_percent < last_copy_percent) {
			last_copy_percent = 0;
		} else if (copy_percent > last_copy_percent) {
			last_copy_percent = copy_percent;
			cellMsgDialogProgressBarInc(CELL_MSGDIALOG_PROGRESSBAR_INDEX_DOUBLE_UPPER, 1);
		}

		if (file_percent < last_file_percent) {
			last_file_percent = 0;
		} else if (file_percent > last_file_percent) {
			last_file_percent = file_percent;
			cellMsgDialogProgressBarInc(CELL_MSGDIALOG_PROGRESSBAR_INDEX_DOUBLE_LOWER, 1);
		}

//		cellMsgDialogProgressBarSetMsg(CELL_MSGDIALOG_PROGRESSBAR_INDEX_DOUBLE_UPPER, string1);
#if 1
		cellGcmSetClearSurface(gCellGcmCurrentContext,
							   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
							   CELL_GCM_CLEAR_A);

		draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x200020ff);

//		cellDbgFontPrintf(0.07f, 0.07f, 1.2f, 0xffffffff, string1);

		cellDbgFontPrintf(0.5f - 0.15f, 1.0f - 0.07 * 2.0f, 1.2f, 0xffffffff, "Hold /\\ to Abort");

		cellDbgFontDrawGcm();

		flip();
#endif
		pad_read();
//		dialog_ret = 0;
//		cellSysutilCheckCallback();
		if (new_pad & BUTTON_TRIANGLE) {
			abort_copy = 1;
			DPrintf("Aborted by user \n");
			error = -666;
			break;
		}

	}

	if (error && error != -666) {
		DPrintf("Error!!!!!!!!!!!!!!!!!!!!!!!!!\nFiles Opened %i\n Waiting 20 seconds to display fatal error\n",
				files_opened);
		cellGcmSetClearSurface(gCellGcmCurrentContext,
							   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
							   CELL_GCM_CLEAR_A);

		draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x200020ff);

		cellDbgFontPrintf(0.07f, 0.07f, 1.2f, 0xffffffff, string1);

		cellDbgFontPrintf(0.5f - 0.15f, 1.0f - 0.07 * 2.0f, 1.2f, 0xffffffff, "Hold /\\ to Abort");

		cellDbgFontDrawGcm();

		flip();

		sys_timer_usleep(20 * 1000000);
	}

	if (fast_writing == 1 && id_w >= 0) {
		cellFsAioCancel(id_w);
		id_w = -1;
		sys_timer_usleep(200000);
	}

	fast_writing = 0;

	if (fast_read == 1 && id_r >= 0) {
		cellFsAioCancel(id_r);
		id_r = -1;
		sys_timer_usleep(200000);
	}

	fast_read = 0;

	for (n = 0; n < fast_num_files; n++) {
		if (fast_files[n].t_read.fd >= 0) {

			cellFsClose(fast_files[n].t_read.fd);
			fast_files[n].t_read.fd = -1;
			files_opened--;
		}
		if (fast_files[n].t_write.fd >= 0) {

			cellFsClose(fast_files[n].t_write.fd);
			fast_files[n].t_write.fd = -1;
			files_opened--;
		}

		if (fast_files[n].mem)
			free(fast_files[n].mem);
		fast_files[n].mem = NULL;
	}

	fast_num_files = 0;

	fast_writing = 0;

	fast_used_mem = 0;

	current_fast_file_r = current_fast_file_w = 0;

	if (error)
		abort_copy = 666;

	return error;
}

static int _my_game_copy(char *path, char *path2)
{
	DIR *dir;

	dir = opendir(path);
	if (!dir) {
		abort_copy = 7;
		return -1;
	}

	while (1) {
		struct dirent *entry = readdir(dir);
		if (!entry)
			break;

		if (entry->d_name[0] == '.' && entry->d_name[1] == 0)
			continue;
		if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == 0)
			continue;

		if ((entry->d_type & DT_DIR)) {

			if (abort_copy)
				break;

			char *d1 = (char *) malloc(1024);
			char *d2 = (char *) malloc(1024);
			if (!d1 || !d2) {
				if (d1)
					free(d1);
				if (d2)
					free(d2);
				closedir(dir);
				DPrintf("malloc() Error!!!\n\n");
				abort_copy = 2;
				closedir(dir);
				return -1;
			}
			sprintf(d1, "%s/%s", path, entry->d_name);
			sprintf(d2, "%s/%s", path2, entry->d_name);
#if 0
			DPrintf("D1: %s\nD2: %s", path, path2);
#endif

			//if(strcmp(path2, "/dev_bdvd/PS3_UPDATE") == 0)
			//{
			mkdir(d2, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
			_my_game_copy(d1, d2);
			//}

			free(d1);
			free(d2);
			if (abort_copy)
				break;
		} else {
			if (strcmp(entry->d_name, "PS3UPDAT.PUP") == 0) {
			} else {
#if 0
				DPrintf("EPATH: %s\nEPATH2: %s\nENTRY %s", path, path2, entry->d_name);
#endif
				if (fast_copy_add(path, path2, entry->d_name) < 0) {
					abort_copy = 666;
					closedir(dir);
					return -1;
				}
			}

		}

		if (abort_copy)
			break;
	}

	closedir(dir);
	if (abort_copy)
		return -1;

	return 0;

}

int my_game_copy(char *path, char *path2)
{
	if (fast_copy_async(path, path2, 1) < 0) {
		abort_copy = 665;
		return -1;
	}

	int ret = my_game_test(path, false);

	copy_global_bytes = global_device_bytes;
	global_device_bytes = 0;

	sprintf(string1, "Copying %s\n to %s...", path, path2);
	cellMsgDialogOpen2(CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE |
					   CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF | CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NONE |
					   CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE, string1, NULL, NULL, NULL);

	cellMsgDialogProgressBarReset(CELL_MSGDIALOG_PROGRESSBAR_INDEX_DOUBLE_LOWER);
	cellMsgDialogProgressBarReset(CELL_MSGDIALOG_PROGRESSBAR_INDEX_DOUBLE_UPPER);

	int ret2 = _my_game_copy(path, path2);

	int ret3 = fast_copy_process();

	fast_copy_async(path, path2, 0);

	cellMsgDialogAbort();

	if (ret < 0 || ret2 < 0 || ret3 < 0)
		return -1;

	return 0;
}

// test if files >= 4GB

int my_game_test(char *path, bool interactive)
{
	DIR *dir;
	dir = opendir(path);

	if (!dir)
		return -1;

	while (1) {
		struct dirent *entry = readdir(dir);
		if (!entry)
			break;

		if (entry->d_name[0] == '.' && entry->d_name[1] == 0)
			continue;
		if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == 0)
			continue;

		if ((entry->d_type & DT_DIR)) {

			char *d1 = (char *) malloc(1024);

			num_directories++;

			if (!d1) {
				closedir(dir);
				DPrintf("malloc() Error!!!\n\n");
				abort_copy = 2;
				return -1;
			}
			sprintf(d1, "%s/%s", path, entry->d_name);
			my_game_test(d1, interactive);
			free(d1);

			if (abort_copy)
				break;
		} else {
			char *f = (char *) malloc(1024);

			struct stat s;

			off64_t size = 0LL;

			if (!f) {
				DPrintf("malloc() Error!!!\n\n");
				abort_copy = 2;
				closedir(dir);
				return -1;
			}
			sprintf(f, "%s/%s", path, entry->d_name);

			if (stat(f, &s) < 0) {
				abort_copy = 3;
				DPrintf("File error!!!\n -> %s\n\n", f);
				if (f)
					free(f);
				break;
			}

			size = s.st_size;

			if (strlen(entry->d_name) > 6) {
				char *p = f;
				p += strlen(f) - 6;	// adjust for .666xx
				if (p[0] == '.' && p[1] == '6' && p[2] == '6' && p[3] == '6') {
					DPrintf("Split file %lli MB %s\n\n", size / 0x100000LL, f);
					num_files_split++;

				}

			}

			if (size >= 0x100000000LL) {
				DPrintf("Big file %lli MB %s\n\n", size / 0x100000LL, f);
				num_files_big++;
			}

			if (f)
				free(f);

			int seconds = (int) (time(NULL) - time_start);

			file_counter++;

			global_device_bytes += size;

			if (interactive) {
				sprintf(string1, "Test File: %i Time: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", file_counter, seconds / 3600,
						(seconds / 60) % 60, seconds % 60, ((double) global_device_bytes) / (1024.0 * 1024. * 1024.0));

				cellGcmSetClearSurface(gCellGcmCurrentContext,
									   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
									   CELL_GCM_CLEAR_A);

				draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x200020ff);

				cellDbgFontPrintf(0.07f, 0.07f, 1.2f, 0xffffffff, string1);

				cellDbgFontPrintf(0.5f - 0.15f, 1.0f - 0.07 * 2.0f, 1.2f, 0xffffffff, "Hold /\\ to Abort");

				cellDbgFontDrawGcm();

				flip();

				pad_read();

				if (abort_copy)
					break;

				if (new_pad & BUTTON_TRIANGLE) {
					abort_copy = 1;
				}
			}
			if (abort_copy)
				break;

		}

	}

	closedir(dir);

	return 0;
}

int my_game_delete(char *path)
{
	DIR *dir;
	char *f = NULL;

	dir = opendir(path);
	if (!dir)
		return -1;

	while (1) {
		struct dirent *entry = readdir(dir);
		int seconds;
		if (!entry)
			break;

		if (entry->d_name[0] == '.' && entry->d_name[1] == 0)
			continue;
		if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == 0)
			continue;

		if ((entry->d_type & DT_DIR)) {

			char *d1 = (char *) malloc(1024);

			if (!d1) {
				closedir(dir);
				DPrintf("malloc() Error!!!\n\n");
				abort_copy = 2;
				return -1;
			}
			sprintf(d1, "%s/%s", path, entry->d_name);
			my_game_delete(d1);
			DPrintf("Deleting <%s>\n\n", path);
			if (rmdir(d1)) {
				abort_copy = 3;
				DPrintf("Deleting Error!!!\n -> <%s>\n\n", entry->d_name);
				if (d1)
					free(d1);
				break;
			}
			free(d1);
			if (abort_copy)
				break;
			file_counter--;
		} else {
			f = (char *) malloc(1024);

			if (!f) {
				DPrintf("malloc() Error!!!\n\n");
				abort_copy = 2;
				closedir(dir);
				return -1;
			}
			sprintf(f, "%s/%s", path, entry->d_name);

			if (remove(f)) {
				abort_copy = 3;
				DPrintf("Deleting Error!!!\n -> %s\n\n", f);
				if (f)
					free(f);
				break;
			}

			DPrintf("Deleted %s\n\n", f);

			if (f)
				free(f);
		}

		seconds = (int) (time(NULL) - time_start);
		sprintf(string1, "Deleting... File: %i Time: %2.2i:%2.2i:%2.2i\n", file_counter, seconds / 3600,
				(seconds / 60) % 60, seconds % 60);

		file_counter++;

		cellGcmSetClearSurface(gCellGcmCurrentContext,
							   CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
							   CELL_GCM_CLEAR_A);

		draw_square(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, 0x200020ff);

		cellDbgFontPrintf(0.07f, 0.07f, 1.2f, 0xffffffff, string1);

		cellDbgFontPrintf(0.5f - 0.15f, 1.0f - 0.07 * 2.0f, 1.2f, 0xffffffff, "Hold /\\ to Abort");

		cellDbgFontDrawGcm();

		flip();

		pad_read();

		if (abort_copy)
			break;

		if (new_pad & BUTTON_TRIANGLE) {
			abort_copy = 1;
		}

		if (abort_copy)
			break;

	}

	closedir(dir);

	return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=120 */
