/*                                                                                                                                                                                 
 * Copyright (C) 2010 drizzt                                                                                                                                                       
 *                                                                                                                                                                                 
 * Authors:                                                                                                                                                                        
 * drizzt <drizzt@ibeglab.org>                                                                                                                                                     
 * flukes1
 * kmeaw
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

#ifdef WITH_PL3
extern uint32_t _binary_payload_pl3_patch_txt_start;
extern uint32_t _binary_payload_pl3_patch_txt_size;
extern uint32_t _binary_payload_pl3_payload_bin_start;
extern uint32_t _binary_payload_pl3_payload_bin_size;
#else
extern uint32_t _binary_payload_syscall36_patch_txt_start;
extern uint32_t _binary_payload_syscall36_patch_txt_size;
extern uint32_t _binary_payload_syscall36_payload_bin1_start;
extern uint32_t _binary_payload_syscall36_payload_bin1_size;
extern uint32_t _binary_payload_syscall36_payload_bin2_start;
extern uint32_t _binary_payload_syscall36_payload_bin2_size;
#endif

uint64_t mmap_lpar_addr;
static int poke_syscall = 7;

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
#ifdef WITH_PL3
	uint64_t *tmp = (uint64_t *) (uint64_t) & _binary_payload_pl3_payload_bin_start;

	return peekq(0x800000000000ef48ULL) == *tmp;
#else
	uint64_t *tmp = (uint64_t *) (uint64_t) & _binary_payload_syscall36_payload_bin1_start;

	return peekq(0x80000000002be4a0ULL) == *tmp;
#endif
}

void load_payload(void)
{
	char *ptr, *ptr2;
	unsigned long long addr, value;
	int patches = 0;

#ifdef USE_MEMCPY_SYSCALL
	/* This does not work on some PS3s */
	pokeq(NEW_POKE_SYSCALL_ADDR, 0x4800000428250000ULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4182001438a5ffffULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0x7cc428ae7cc329aeULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x4bffffec4e800020ULL);

#ifdef WITH_PL3
	system_call_3(NEW_POKE_SYSCALL, 0x800000000000ef48ULL, (unsigned long long) &&_binary_payload_pl3_payload_bin_start,
				  (uint64_t) & _binary_payload_pl3_payload_bin_size);
#else
	system_call_3(new_poke_syscall, 0x80000000002be4a0ULL,
				  (unsigned long long) &_binary_payload_syscall36_payload_bin1_start,
				  (uint64_t) & _binary_payload_syscall36_payload_bin1_size);
	system_call_3(new_poke_syscall, 0x80000000002d8430ULL,
				  (unsigned long long) &_binary_payload_syscall36_payload_bin2_start,
				  (uint64_t) & _binary_payload_syscall36_payload_bin2_size);
#endif

	/* restore syscall */
	remove_new_poke();
	pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0xebc2fe287c7f1b78);
	pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x3860032dfba100e8);
#else
	/* WARNING!! It supports only payload with a size multiple of 4 */
	uint32_t i;

#ifdef WITH_PL3
	uint64_t *pl64 = (uint64_t *) (uint64_t) & _binary_payload_pl3_payload_bin_start;
	for (i = 0; i < (uint64_t) & _binary_payload_pl3_payload_bin_size / sizeof(uint64_t); i++) {
		pokeq(0x800000000000ef48ULL + i * sizeof(uint64_t), *pl64++);
	}
	if ((uint64_t) & _binary_payload_pl3_payload_bin_size % sizeof(uint64_t)) {
		pokeq32(0x800000000000ef48ULL + i * sizeof(uint64_t), (uint32_t) * pl64);
	}
#else
	uint64_t *pl64 = (uint64_t *) (uint64_t) & _binary_payload_syscall36_payload_bin1_start;

	for (i = 0; i < (uint64_t) & _binary_payload_syscall36_payload_bin1_size / sizeof(uint64_t); i++) {
		pokeq(0x80000000002be4a0ULL + i * sizeof(uint64_t), *pl64++);
	}

	pl64 = (uint64_t *) (uint64_t) & _binary_payload_syscall36_payload_bin2_start;
	for (i = 0; i < (uint64_t) & _binary_payload_syscall36_payload_bin2_size / sizeof(uint64_t); i++) {
		pokeq(0x80000000002d8430ULL + i * sizeof(uint64_t), *pl64++);
	}
#endif
#endif

#ifdef WITH_PL3
	char *tmp = strtok((char *) &_binary_payload_pl3_patch_txt_start, "\n");
#else
	char *tmp = strtok((char *) &_binary_payload_syscall36_patch_txt_start, "\n");
#endif

	do {
		ptr = strchr(tmp, '#');
		if (ptr)
			*ptr = 0;
		ptr = tmp;

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
		if (!strchr("0123456789abcdefABCDEF", *ptr))
			continue;
		ptr2 = ptr;
		value = strtoull(ptr, &ptr, 16);

		patches++;

		if (ptr - ptr2 == 8) {
			_poke32(addr, value);
		} else if (ptr - ptr2 == 16) {
			_poke(addr, value);
		} else
			patches--;
	}
	while ((tmp = strtok(NULL, "\n")));
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
