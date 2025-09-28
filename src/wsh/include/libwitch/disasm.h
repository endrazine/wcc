/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2025 Jonathan Brossard
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*******************************************************************************
*
* Multi-Architecture Disassembly for wsh
*
*/

#ifndef WSH_DISASM_H
#define WSH_DISASM_H

#include <lua.h>
#include <capstone/capstone.h>
#include <elf.h>

// Architecture information structure
typedef struct arch_info {
	const char *name;
	const char *aliases[3];
	cs_arch arch;
	cs_mode mode;
	uint16_t elf_machine;
	const char *description;
	const char *category;
} arch_info_t;

// Per-binary architecture tracking
typedef struct binary_arch {
	char *filename;
	arch_info_t *arch;
	unsigned long base_addr;
	struct binary_arch *next;
} binary_arch_t;

// Function declarations for wsh integration
void init_multiarch_support(void);
void wsh_binary_loaded_hook(const char *filename, unsigned long base_addr);
void register_disasm_functions(lua_State * L);

// Lua callable functions
int disasm(lua_State * L);
int disasm_sym(lua_State * L);
int arch_set(lua_State * L);
int arch_info(lua_State * L);
int arch_list(lua_State * L);

// Internal helper functions
arch_info_t *get_arch_by_name(const char *name);
arch_info_t *get_arch_by_elf_machine(uint16_t machine, uint8_t elf_class, uint8_t elf_endian);
arch_info_t *detect_arch_from_elf(const char *filename);
void register_binary_arch(const char *filename, unsigned long base_addr);
arch_info_t *find_arch_for_address(unsigned long addr);

#endif				/* WSH_DISASM_H */
