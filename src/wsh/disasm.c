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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <libwitch/wsh.h>
#include <libwitch/disasm.h>



// Architecture support table - only includes architectures available in most Capstone builds
static arch_info_t supported_archs[] = {
	// Traditional processors
	{ "x86", { "i386", "ia32", ""}, CS_ARCH_X86, CS_MODE_32, EM_386, "Intel x86 32-bit", "Traditional" },
	{ "x86_64", { "x64", "amd64", ""}, CS_ARCH_X86, CS_MODE_64, EM_X86_64, "Intel x86 64-bit", "Traditional" },
	{ "x86_16", { "x16", "", ""}, CS_ARCH_X86, CS_MODE_16, EM_386, "Intel x86 16-bit", "Traditional" },

	// ARM family
	{ "arm", { "arm32", "", ""}, CS_ARCH_ARM, CS_MODE_ARM, EM_ARM, "ARM 32-bit", "ARM" },
	{ "armthumb", { "thumb", "", ""}, CS_ARCH_ARM, CS_MODE_THUMB, EM_ARM, "ARM Thumb mode", "ARM" },
	{ "arm64", { "aarch64", "armv8", ""}, CS_ARCH_ARM64, CS_MODE_ARM, EM_AARCH64, "ARM 64-bit (AArch64)", "ARM" },

	// MIPS family  
	{ "mips", { "mips32", "", ""}, CS_ARCH_MIPS, CS_MODE_MIPS32, EM_MIPS, "MIPS 32-bit little-endian", "MIPS" },
	{ "mipsbe", { "mips32be", "", ""}, CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_BIG_ENDIAN, EM_MIPS, "MIPS 32-bit big-endian", "MIPS" },
	{ "mips64", { "mips64le", "", ""}, CS_ARCH_MIPS, CS_MODE_MIPS64, EM_MIPS, "MIPS 64-bit little-endian", "MIPS" },
	{ "mips64be", { "", "", ""}, CS_ARCH_MIPS, CS_MODE_MIPS64 | CS_MODE_BIG_ENDIAN, EM_MIPS, "MIPS 64-bit big-endian", "MIPS" },

	// PowerPC family
	{ "ppc", { "powerpc", "ppc32", ""}, CS_ARCH_PPC, CS_MODE_32, EM_PPC, "PowerPC 32-bit", "PowerPC" },
	{ "ppc64", { "powerpc64", "ppc64le", ""}, CS_ARCH_PPC, CS_MODE_64, EM_PPC64, "PowerPC 64-bit little-endian", "PowerPC" },
	{ "ppc64be", { "", "", ""}, CS_ARCH_PPC, CS_MODE_64 | CS_MODE_BIG_ENDIAN, EM_PPC64, "PowerPC 64-bit big-endian", "PowerPC" },

	// SPARC family
	{ "sparc", { "sparc32", "", ""}, CS_ARCH_SPARC, CS_MODE_32 | CS_MODE_BIG_ENDIAN, EM_SPARC, "SPARC 32-bit", "SPARC" },
	{ "sparc64", { "sparcv9", "", ""}, CS_ARCH_SPARC, CS_MODE_64 | CS_MODE_BIG_ENDIAN, EM_SPARCV9, "SPARC 64-bit (v9)", "SPARC" },

	// Legacy/Embedded processors
	{ "m68k", { "68k", "motorola68k", ""}, CS_ARCH_M68K, CS_MODE_BIG_ENDIAN, EM_68K, "Motorola 68000 family", "Legacy" },
	{ "systemz", { "s390x", "sysz", "zarch"}, CS_ARCH_SYSZ, CS_MODE_BIG_ENDIAN, EM_S390, "IBM System Z (s390x)", "Mainframe" },
	{ "xcore", { "", "", ""}, CS_ARCH_XCORE, CS_MODE_BIG_ENDIAN, 0, "XMOS XCore", "Embedded" },

#ifdef CS_ARCH_M680X
	// Only include if M680X is available
	{ "m680x", { "6800", "6809", ""}, CS_ARCH_M680X, CS_MODE_BIG_ENDIAN, 0, "Motorola 680x family", "Legacy" },
#endif

#ifdef CS_ARCH_TMS320C64X
	// Only include if TMS320C64X is available
	{ "tms320c64x", { "c64x", "ti_c64x", ""}, CS_ARCH_TMS320C64X, 0, 0, "Texas Instruments TMS320C64x", "DSP" },
#endif

#ifdef CS_ARCH_RISCV
	// Only include if RISC-V is available (newer Capstone versions)
	{ "riscv32", { "rv32", "riscv", ""}, CS_ARCH_RISCV, CS_MODE_RISCV32, EM_RISCV, "RISC-V 32-bit", "RISC-V" },
	{ "riscv64", { "rv64", "", ""}, CS_ARCH_RISCV, CS_MODE_RISCV64, EM_RISCV, "RISC-V 64-bit", "RISC-V" },
#endif

#ifdef CS_ARCH_MOS65XX
	// Only include if MOS65XX is available
	{ "mos65xx", { "6502", "mos6502", ""}, CS_ARCH_MOS65XX, 0, 0, "MOS Technology 65xx (6502)", "Legacy" },
#endif

#ifdef CS_ARCH_BPF
	// Only include if BPF is available
	{ "bpf", { "ebpf", "", ""}, CS_ARCH_BPF, 0, 0, "Berkeley Packet Filter (eBPF)", "VM" },
#endif

#ifdef CS_ARCH_EVM
	// Only include if EVM is available
	{ "evm", { "ethereum", "", ""}, CS_ARCH_EVM, 0, 0, "Ethereum Virtual Machine", "VM" },
#endif

#ifdef CS_ARCH_WASM
	// Only include if WebAssembly is available
	{ "wasm", { "webassembly", "", ""}, CS_ARCH_WASM, 0, 0, "WebAssembly", "VM" },
#endif

	{ NULL, { ""}, 0, 0, 0, NULL, NULL }
};

// Global state
static arch_info_t *default_arch = NULL;
static binary_arch_t *binary_archs = NULL;

/**
 * Get architecture info by name (including aliases)
 */
arch_info_t *get_arch_by_name(const char *name)
{
	for (int i = 0; supported_archs[i].name; i++) {
		if (strcasecmp(supported_archs[i].name, name) == 0) {
			return &supported_archs[i];
		}
		for (int j = 0; j < 3 && supported_archs[i].aliases[j][0]; j++) {
			if (strcasecmp(supported_archs[i].aliases[j], name) == 0) {
				return &supported_archs[i];
			}
		}
	}
	return NULL;
}

/**
 * Get architecture info by ELF machine type
 */
arch_info_t *get_arch_by_elf_machine(uint16_t machine, uint8_t elf_class, uint8_t elf_endian)
{
	for (int i = 0; supported_archs[i].name; i++) {
		if (supported_archs[i].elf_machine == machine) {
			arch_info_t *arch = &supported_archs[i];

			// Handle variants that need special detection
			if (machine == EM_MIPS) {
				if (elf_class == ELFCLASS64) {
					return (elf_endian == ELFDATA2MSB) ? get_arch_by_name("mips64be") : get_arch_by_name("mips64");
				} else {
					return (elf_endian == ELFDATA2MSB) ? get_arch_by_name("mipsbe") : get_arch_by_name("mips");
				}
			}

			if (machine == EM_RISCV) {
				return (elf_class == ELFCLASS64) ? get_arch_by_name("riscv64") : get_arch_by_name("riscv32");
			}

			if (machine == EM_PPC || machine == EM_PPC64) {
				if (elf_class == ELFCLASS64) {
					return (elf_endian == ELFDATA2MSB) ? get_arch_by_name("ppc64be") : get_arch_by_name("ppc64");
				} else {
					return get_arch_by_name("ppc");
				}
			}

			return arch;
		}
	}
	return NULL;
}

/**
 * Detect architecture from ELF header
 */
arch_info_t *detect_arch_from_elf(const char *filename)
{
	int fd;
	Elf64_Ehdr ehdr64;
	Elf32_Ehdr *ehdr32;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;

	if (read(fd, &ehdr64, sizeof(ehdr64)) != sizeof(ehdr64)) {
		close(fd);
		return NULL;
	}
	close(fd);

	// Check ELF magic
	if (memcmp(ehdr64.e_ident, ELFMAG, SELFMAG) != 0) {
		return NULL;
	}

	uint16_t machine;
	uint8_t elf_class = ehdr64.e_ident[EI_CLASS];
	uint8_t elf_endian = ehdr64.e_ident[EI_DATA];

	if (elf_class == ELFCLASS64) {
		machine = ehdr64.e_machine;
	} else if (elf_class == ELFCLASS32) {
		ehdr32 = (Elf32_Ehdr *) & ehdr64;
		machine = ehdr32->e_machine;
	} else {
		return NULL;
	}

	return get_arch_by_elf_machine(machine, elf_class, elf_endian);
}

/**
 * Register binary architecture
 */
void register_binary_arch(const char *filename, unsigned long base_addr)
{
	arch_info_t *arch = detect_arch_from_elf(filename);
	if (!arch)
		return;

	// Check if already registered
	binary_arch_t *ba = binary_archs;
	while (ba) {
		if (strcmp(ba->filename, filename) == 0) {
			ba->arch = arch;
			ba->base_addr = base_addr;
			return;
		}
		ba = ba->next;
	}

	// Add new entry
	ba = malloc(sizeof(binary_arch_t));
	ba->filename = strdup(filename);
	ba->arch = arch;
	ba->base_addr = base_addr;
	ba->next = binary_archs;
	binary_archs = ba;

	// Set default if first binary
	if (!default_arch) {
		default_arch = arch;
	}
}

/**
 * Find architecture for address
 */
arch_info_t *find_arch_for_address(unsigned long addr)
{
	// Try to find which binary this address belongs to
	binary_arch_t *ba = binary_archs;
	while (ba) {
		if (addr >= ba->base_addr) {
			return ba->arch;
		}
		ba = ba->next;
	}

	// Try to find from loaded sections
	extern wsh_t *wsh;	// Reference to global wsh
	sections_t *s = wsh->shdrs;
	while (s) {
		if (addr >= s->addr && addr < s->addr + s->size) {
			// Find binary that contains this section
			binary_arch_t *ba = binary_archs;
			while (ba) {
				if (addr >= ba->base_addr && addr < ba->base_addr + 0x100000) {
					return ba->arch;
				}
				ba = ba->next;
			}
		}
		s = s->next;
	}

	return default_arch;
}

/**
 * Multi-architecture disassembly function
 */
int disasm(lua_State *L)
{
	unsigned long int addr = 0;
	unsigned int length = 32;
	arch_info_t *target_arch = NULL;
	int argc = lua_gettop(L);

	// Parse arguments
	if (argc >= 1) {
		if (lua_isnumber(L, 1)) {
			addr = (unsigned long int) lua_tonumber(L, 1);
		} else {
			printf("Error: Address must be a number\n");
			return 0;
		}
	} else {
		printf("Usage: disasm(address, [length], [arch])\n");
		printf("  address: Memory address to disassemble\n");
		printf("  length:  Number of bytes (default: 32)\n");
		printf("  arch:    Architecture override (optional)\n");
		printf("\nSupported architectures by category:\n");

		const char *current_category = "";
		for (int i = 0; supported_archs[i].name; i++) {
			if (strcmp(current_category, supported_archs[i].category) != 0) {
				current_category = supported_archs[i].category;
				printf("\n  %s:\n", current_category);
			}
			printf("    %-12s - %s\n", supported_archs[i].name, supported_archs[i].description);
		}
		return 0;
	}

	if (argc >= 2 && lua_isnumber(L, 2)) {
		length = (unsigned int) lua_tonumber(L, 2);
		if (length > 1024) {
			printf("Warning: Length limited to 1024 bytes\n");
			length = 1024;
		}
	}
	// Architecture selection logic
	if (argc >= 3 && lua_isstring(L, 3)) {
		const char *arch_str = lua_tostring(L, 3);
		target_arch = get_arch_by_name(arch_str);
		if (!target_arch) {
			printf("Error: Unknown architecture '%s'\n", arch_str);
			printf("Use 'arch_list()' to see supported architectures\n");
			return 0;
		}
	} else {
		// Auto-detect architecture for this address
		target_arch = find_arch_for_address(addr);
		if (!target_arch) {
			target_arch = default_arch ? default_arch : &supported_archs[1];	// x86_64 fallback
			extern wsh_t *wsh;
			if (wsh->opt_verbose) {
				printf("Using default architecture: %s\n", target_arch->description);
			}
		}
	}

	// Check memory accessibility (skip for VM architectures)
	if (strcmp(target_arch->category, "VM") != 0) {
		if (msync((void *) (addr & ~0xfff), 0x1000, MS_ASYNC)) {
			printf("Error: Memory at 0x%lx is not accessible\n", addr);
			return 0;
		}
	}
	// Check if this architecture is supported by current Capstone build
	if (!cs_support(target_arch->arch)) {
		printf("Error: Architecture %s (%s) not supported by this Capstone build\n", target_arch->name, target_arch->description);
		printf("Recompile Capstone with support for this architecture\n");
		return 0;
	}
	// Initialize capstone
	csh handle;
	cs_insn *insn;
	size_t count;

	if (cs_open(target_arch->arch, target_arch->mode, &handle)) {
		printf("Error: Failed to initialize capstone for %s\n", target_arch->description);
		return 0;
	}

	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

	// Disassemble
	count = cs_disasm(handle, (uint8_t *) addr, length, addr, 0, &insn);

	if (count > 0) {
		printf(BLUE "\n   Disassembly of 0x%lx (%u bytes) [%s]:\n\n" NORMAL, addr, length, target_arch->description);

		lua_newtable(L);

		for (size_t i = 0; i < count; i++) {
			printf("  " GREEN "0x%lx:" NORMAL " %-12s %s\n", insn[i].address, insn[i].mnemonic, insn[i].op_str);

			// Build Lua table entry
			lua_pushinteger(L, i + 1);
			lua_newtable(L);

			lua_pushstring(L, "address");
			lua_pushinteger(L, insn[i].address);
			lua_settable(L, -3);

			lua_pushstring(L, "mnemonic");
			lua_pushstring(L, insn[i].mnemonic);
			lua_settable(L, -3);

			lua_pushstring(L, "operands");
			lua_pushstring(L, insn[i].op_str);
			lua_settable(L, -3);

			lua_pushstring(L, "arch");
			lua_pushstring(L, target_arch->name);
			lua_settable(L, -3);

			lua_pushstring(L, "category");
			lua_pushstring(L, target_arch->category);
			lua_settable(L, -3);

			lua_settable(L, -3);
		}

		printf(NORMAL "\n");
		cs_free(insn, count);
	} else {
		printf("Error: Failed to disassemble %s code at 0x%lx\n", target_arch->description, addr);
		printf("This could be due to:\n");
		printf("  - Invalid code for this architecture\n");
		printf("  - Wrong architecture selected\n");
		printf("  - Data instead of code at this address\n");
		lua_newtable(L);
	}

	cs_close(&handle);
	return 1;
}

/**
 * Symbol-based disassembly
 */
int disasm_sym(lua_State *L)
{
	const char *symbol_name;
	unsigned int length = 32;
	const char *arch_str = NULL;
	int argc = lua_gettop(L);

	if (argc < 1 || !lua_isstring(L, 1)) {
		printf("Usage: disasm_sym(symbol_name, [length], [arch])\n");
		return 0;
	}

	symbol_name = lua_tostring(L, 1);

	if (argc >= 2 && lua_isnumber(L, 2)) {
		length = (unsigned int) lua_tonumber(L, 2);
	}

	if (argc >= 3 && lua_isstring(L, 3)) {
		arch_str = lua_tostring(L, 3);
	}
	// Find symbol using wsh's symbol lookup
	extern symbols_t *symbol_from_name(char *fname);
	symbols_t *sym = symbol_from_name((char *) symbol_name);
	if (!sym) {
		printf("Error: Symbol '%s' not found\n", symbol_name);
		return 0;
	}
	// Use symbol size if available and no length specified
	if (sym->size > 0 && length == 32) {
		length = sym->size > 512 ? 512 : sym->size;
	}

	printf("Disassembling symbol '%s' at 0x%lx (%u bytes)\n", symbol_name, sym->addr, length);

	// Clear the stack and set up proper arguments for disasm
	lua_settop(L, 0);

	// Push arguments in correct order for disasm function
	lua_pushinteger(L, sym->addr);	// arg 1: address
	lua_pushinteger(L, length);	// arg 2: length
	if (arch_str) {
		lua_pushstring(L, arch_str);	// arg 3: architecture (optional)
	}

	return disasm(L);
}

/**
 * Set default architecture
 */
int arch_set(lua_State *L)
{
	if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
		printf("Usage: arch_set(\"architecture\")\n");
		printf("Available architectures:\n");
		for (int i = 0; supported_archs[i].name; i++) {
			printf("  %-12s - %s\n", supported_archs[i].name, supported_archs[i].description);
		}
		return 0;
	}

	const char *arch_name = lua_tostring(L, 1);
	arch_info_t *arch = get_arch_by_name(arch_name);

	if (!arch) {
		printf("Error: Unknown architecture '%s'\n", arch_name);
		return 0;
	}

	default_arch = arch;
	printf("Default architecture set to: %s\n", arch->description);
	return 0;
}

/**
 * Show current architecture settings
 */
int arch_info(lua_State *L)
{
	printf(BLUE "\n   Architecture Information:\n\n" NORMAL);

	printf("Default architecture: %s\n", default_arch ? default_arch->description : "None set");

	printf("\nLoaded binaries:\n");
	binary_arch_t *ba = binary_archs;
	int count = 0;
	while (ba) {
		printf("  %-30s -> %s\n", ba->filename, ba->arch->description);
		ba = ba->next;
		count++;
	}

	if (count == 0) {
		printf("  No binaries loaded\n");
	}

	printf("\nSupported architectures by category:\n");
	const char *current_category = "";
	for (int i = 0; supported_archs[i].name; i++) {
		if (strcmp(current_category, supported_archs[i].category) != 0) {
			current_category = supported_archs[i].category;
			printf("  %s: ", current_category);
		}
		printf("%s ", supported_archs[i].name);
		// Check if this is the last in category or different next category
		if (!supported_archs[i + 1].name || strcmp(current_category, supported_archs[i + 1].category) != 0) {
			printf("\n");
		}
	}

	return 0;
}

/**
 * List supported architectures
 */
int arch_list(lua_State *L)
{
	printf(BLUE "\n   Supported Architectures:\n\n" NORMAL);

	lua_newtable(L);

	const char *current_category = "";
	for (int i = 0; supported_archs[i].name; i++) {
		// Print category header
		if (strcmp(current_category, supported_archs[i].category) != 0) {
			current_category = supported_archs[i].category;
			printf("  " BLUE "%s:\n" NORMAL, current_category);
		}
		// Check if supported by current build
		bool is_supported = cs_support(supported_archs[i].arch);
		const char *status = is_supported ? GREEN "✓" NORMAL : RED "✗" NORMAL;

		printf("    %s %-12s - %s\n", status, supported_archs[i].name, supported_archs[i].description);

		// Add to Lua table
		lua_pushstring(L, supported_archs[i].name);
		lua_newtable(L);

		lua_pushstring(L, "description");
		lua_pushstring(L, supported_archs[i].description);
		lua_settable(L, -3);

		lua_pushstring(L, "category");
		lua_pushstring(L, supported_archs[i].category);
		lua_settable(L, -3);

		lua_pushstring(L, "supported");
		lua_pushboolean(L, is_supported);
		lua_settable(L, -3);

		lua_settable(L, -3);
	}

	printf("\n");
	return 1;
}

/**
 * Initialize multi-architecture support
 */
void init_multiarch_support(void)
{
	// Set reasonable default based on compile-time detection
#if defined(__x86_64__) || defined(_M_X64)
	default_arch = get_arch_by_name("x86_64");
#elif defined(__i386__) || defined(_M_IX86)
	default_arch = get_arch_by_name("x86");
#elif defined(__aarch64__)
	default_arch = get_arch_by_name("arm64");
#elif defined(__arm__)
	default_arch = get_arch_by_name("arm");
#elif defined(__mips64)
	default_arch = get_arch_by_name("mips64");
#elif defined(__mips__)
	default_arch = get_arch_by_name("mips");
#elif defined(__powerpc64__)
	default_arch = get_arch_by_name("ppc64");
#elif defined(__powerpc__)
	default_arch = get_arch_by_name("ppc");
#elif defined(__riscv) && (__riscv_xlen == 64)
	default_arch = get_arch_by_name("riscv64");
#elif defined(__riscv)
	default_arch = get_arch_by_name("riscv32");
#else
	default_arch = get_arch_by_name("x86_64");	// Safe fallback
#endif

	binary_archs = NULL;

	extern wsh_t *wsh;
	if (wsh && wsh->opt_verbose && default_arch) {
		printf("Multi-architecture disassembly initialized (default: %s)\n", default_arch->description);
	}
}

/**
 * Hook for binary loading
 */
void wsh_binary_loaded_hook(const char *filename, unsigned long base_addr)
{
	register_binary_arch(filename, base_addr);

	extern wsh_t *wsh;
	if (wsh && wsh->opt_verbose) {
		arch_info_t *arch = detect_arch_from_elf(filename);
		if (arch) {
			printf("Detected architecture: %s for %s\n", arch->description, filename);
		}
	}
}
