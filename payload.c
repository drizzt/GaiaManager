/*
 * Copyright (C) 2010 drizzt
 *
 * Authors:
 * drizzt <drizzt@ibeglab.org>
 * flukes1
 * kmeaw
 * TheAnswer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "payload.h"
#include "hvcall.h"
#include "mm.h"

uint64_t mmap_lpar_addr;
/* Provide first 8 bytes just to be safe if we cannot read the payload */
static unsigned char payload_bin[1460] = { 0x38, 0x60, 0x00, 0x01, 0x4e, 0x80, 0x00, 0x20, 0, };

static int poke_syscall = 7;

static const char *search_dirs[] = {
	"/dev_usb000/lv2/",
	"/dev_usb001/lv2/",
	"/dev_usb002/lv2/",
	"/dev_usb003/lv2/",
	"/dev_usb004/lv2/",
	"/dev_usb005/lv2/",
	"/dev_usb006/lv2/",
	"/dev_usb007/lv2/",

	"/dev_cf/lv2/",
	"/dev_sd/lv2/",
	"/dev_ms/lv2/",

	"/dev_hdd0/game/" FOLDER_NAME "/USRDIR/",

	NULL
};

static FILE *search_file(const char *filename)
{
	FILE *result;
	const char **search_dir = search_dirs;
	char buf[256];

	if (*filename == '/')
		return fopen(filename, "r");

	while (*search_dir) {
		strcpy(buf, *search_dir);
		strcat(buf, filename);
		result = fopen(buf, "r");
		if (result)
			return result;
		else
			search_dir++;
	}

	return NULL;
}

unsigned char *read_file(FILE * f, size_t * sz)
{
	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);
	*sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	unsigned char *userlandBuffer = malloc(*sz);
	if (!userlandBuffer)
		return NULL;

	fread(userlandBuffer, 1, *sz, f);
	fclose(f);

	return userlandBuffer;
}

void go(u64 addr)
{
	u64 syscall11_ptr = lv2_peek(SYSCALL_PTR(11));
	u64 old_syscall11 = lv2_peek(syscall11_ptr);
	lv2_poke(syscall11_ptr, addr);
	Lv2Syscall0(11);
	lv2_poke(syscall11_ptr, old_syscall11);
}

#ifdef USE_MEMCPY_SYSCALL
void install_lv2_memcpy()
{
	PRINTF("installing memcpy...\n");
	/* install memcpy */
	lv2_poke(NEW_POKE_SYSCALL_ADDR, 0x4800000428250000ULL);
	lv2_poke(NEW_POKE_SYSCALL_ADDR + 8, 0x4182001438a5ffffULL);
	lv2_poke(NEW_POKE_SYSCALL_ADDR + 16, 0x7cc428ae7cc329aeULL);
	lv2_poke(NEW_POKE_SYSCALL_ADDR + 24, 0x4bffffec4e800020ULL);

}

void remove_lv2_memcpy()
{
	PRINTF("uninstalling memcpy...\n");
	/* restore syscall */
	remove_new_poke();
	lv2_poke(NEW_POKE_SYSCALL_ADDR + 16, 0xebc2fe287c7f1b78);
	lv2_poke(NEW_POKE_SYSCALL_ADDR + 24, 0x3860032dfba100e8);
}

inline static void lv2_memcpy(void *to, const void *from, size_t sz)
{
	Lv2Syscall3(NEW_POKE_SYSCALL, to, (unsigned long long) from, sz);
}
#else
void install_lv2_memcpy() {}
void remove_lv2_memcpy() {}
void lv2_memcpy(void *to, const void *from, size_t sz)
{
	/* WARNING!! It supports only payload with a size multiple of 4 */
	uint64_t *source64 = (uint64_t *) from;
	uint64_t *target64 = (uint64_t *) to;

	for (i = 0; i < sz / sizeof(uint64_t); i++) {
		pokeq(target64++, *source64++);
	}
	if (sz % sizeof(uint64_t)) {
		pokeq32(target64, (uint32_t) *source64);
	}
}
#endif

uint64_t peekq(uint64_t addr)
{
	system_call_1(6, addr);
	return_to_user_prog(uint64_t);
}

void pokeq(uint64_t addr, uint64_t val)
{
	system_call_2(poke_syscall, addr, val);
}

void pokeq32(uint64_t addr, uint32_t val)
{
	uint32_t next = peekq(addr) & 0xffffffffUL;
	pokeq(addr, (uint64_t) val << 32 | next);
}

static void lv1_poke(uint64_t addr, uint64_t val)
{
	system_call_2(7, HV_BASE + addr, val);
}

static inline void _poke(uint64_t addr, uint64_t val)
{
	pokeq(0x8000000000000000ULL + addr, val);
}

static inline void _poke32(uint64_t addr, uint32_t val)
{
	pokeq32(0x8000000000000000ULL + addr, val);
}

bool is_payload_loaded(void)
{
	uint64_t *tmp = (uint64_t *) payload_bin;

	return peekq(0x800000000000ef48ULL) == *tmp;
}

void load_payload(void)
{
#define PATCH_FLAG_EXEC		1
#define PATCH_FLAG_BACKUP	2
	char buf[512], *ptr, *ptr2;
	unsigned long long addr, value, backup_addr;
	int patches = 0, payloads = 0;
	int flags = 0;

	FILE *payload;
	FILE *patch = search_file("patch.txt");

	if (patch) {
		while (!feof(patch)) {
			if (!fgets(buf, sizeof(buf), patch))
				break;
			if (!buf[0])
				break;

			/*
			 * address: [@[x][b]] { payload_name | poke32 | poke64 | "go" }
			 *
			 * 472461: xb payload.bin
			 * 28ca70: 37719461
			 * 7f918a: 16380059372ab00a
			 */

			ptr = strchr(buf, '#');
			if (ptr)
				*ptr = 0;
			ptr = buf;
			while (*ptr == ' ' || *ptr == '\t')
				ptr++;
			if (!strchr("0123456789abcdefABCDEF", *ptr))
				continue;
			addr = strtoull(ptr, &ptr, 16);
			if (*ptr != ':')
				continue;
			else
				ptr++;
			while (*ptr == ' ' || *ptr == '\t')
				ptr++;
			flags = 0;
			if (*ptr == '@') {
				ptr++;
				if (*ptr == 'x') {
					flags |= PATCH_FLAG_EXEC;
					ptr++;
				}
				if (*ptr == 'b') {
					flags |= PATCH_FLAG_BACKUP;
					ptr++;
				}
				while (*ptr == ' ' || *ptr == '\t')
					ptr++;
			}
			if (ptr[0] == 'g' && ptr[1] == 'o') {

			} else if (!strchr("0123456789abcdefABCDEF", *ptr))
				do {
					ptr2 = strchr(ptr, '\n');
					if (ptr2)
						*ptr2 = 0;
					ptr2 = strchr(ptr, '\r');
					if (ptr2)
						*ptr2 = 0;
					ptr2 = strchr(ptr, ' ');
					if (ptr2)
						*ptr2 = 0;
					ptr2 = strchr(ptr, '\t');
					if (ptr2)
						*ptr2 = 0;
					payload = search_file(ptr);

					if (!payload) {
						PRINTF
						    ("Cannot open file \"%s\".\n",
						     ptr);
						break;
					}

					PRINTF("reading payload...\n");
					size_t sz;
					unsigned char *payload_bin =
					    read_file(payload, &sz);

					backup_addr = 0;
					if (!addr)
						addr = lv2_alloc(sz, 0x27);
					else
						backup_addr =
						    lv2_alloc(sz, 0x27);

					install_lv2_memcpy();

					if (flags & PATCH_FLAG_BACKUP) {
						/* backup */
						PRINTF
						    ("backing up the data...\n");
						lv2_memcpy(backup_addr, addr, sz);
					}

					/* copy the payload */
					PRINTF("copying the payload...\n");
					lv2_memcpy((void*) (0x8000000000000000ULL + addr), payload_bin, sz);
					remove_lv2_memcpy();

					if (flags & PATCH_FLAG_EXEC) {
						PRINTF
						    ("Executing the payload...\n");
						go(addr);
					}

					if (flags & PATCH_FLAG_BACKUP) {
						PRINTF
						    ("Restoring LV2 memory...\n");
						install_lv2_memcpy();
						Lv2Syscall3(NEW_POKE_SYSCALL,
							    addr, backup_addr,
							    sz);
						remove_lv2_memcpy();
					}

					PRINTF("Done.\n");

					payloads++;
				} while (0);
			else {
				ptr2 = ptr;
				value = strtoull(ptr, &ptr, 16);

				patches++;

				if (ptr - ptr2 == 8) {
					_poke32(addr, value);
					PRINTF("poke32 %p %08llX\n",
					       (void *)addr, value);
				} else if (ptr - ptr2 == 16) {
					_poke(addr, value);
					PRINTF("poke64 %p %16llX\n",
					       (void *)addr, value);
				} else
					patches--;
			}
		}

		fclose(patch);
	}
}

int map_lv1(void)
{
	int result = lv1_undocumented_function_114(0, 0xC, HV_SIZE, &mmap_lpar_addr);
	if (result != 0) {
		return 0;
	}

	result = mm_map_lpar_memory_region(mmap_lpar_addr, HV_BASE, HV_SIZE, 0xC, 0);
	if (result) {
		return 0;
	}

	return 1;
}

void unmap_lv1(void)
{
	if (mmap_lpar_addr != 0)
		lv1_undocumented_function_115(mmap_lpar_addr);

	if (search_file("nopp.txt")) {
		/* Disable peek/poke */
		_poke32(0x1933c, 0x3C608001);
		_poke(0x19340, 0x606300034E800020);
		_poke(0x19348, 0x3C60800160630003);
	}
}

void patch_lv2_protection(void)
{
	// changes protected area of lv2 to first byte only
	lv1_poke(0x363a78, 0x0000000000000001ULL);
	lv1_poke(0x363a80, 0xe0d251b556c59f05ULL);
	lv1_poke(0x363a88, 0xc232fcad552c80d7ULL);
	lv1_poke(0x363a90, 0x65140cd200000000ULL);
}

void install_new_poke(void)
{
	// install poke with icbi instruction
	pokeq(NEW_POKE_SYSCALL_ADDR, 0xF88300007C001FACULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4C00012C4E800020ULL);
	poke_syscall = NEW_POKE_SYSCALL;
}

void remove_new_poke(void)
{
	poke_syscall = 7;
	pokeq(NEW_POKE_SYSCALL_ADDR, 0xF821FF017C0802A6ULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0xFBC100F0FBE100F8ULL);
}

/* vim: set ts=4 sw=4 sts=4 tw=120 */
