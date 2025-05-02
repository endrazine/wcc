/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2025 Jonathan Brossard
*
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
* Note:
*   Option -S performs libification using segments instead of sections.  
*
*/


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <elf.h>
#include <getopt.h>

#include <config.h>

#include "wld.h"

#define DEFAULT_NAME "wld"

#define DF_1_NOW        0x00000001
#define DF_1_PIE        0x08000000


/**
* Process 64bits ELF binary using Segments
*/
int process_segments64(char *map, unsigned int noinit, unsigned int strip_vernum, unsigned int no_now_flag)
{
	Elf_Ehdr *elf64 = 0;
	Elf64_Phdr *phdr64 = 0;
	Elf64_Dyn *dyn64 = 0;
	unsigned int phnum = 0;
	unsigned int i = 0, j = 0;

	elf64 = (Elf64_Ehdr *) map;
	phdr64 = (Elf64_Phdr *) (map + elf64->e_phoff);
	phnum = elf64->e_phnum;

	for (i = 0; i < phnum; i++) {
		if (phdr64[i].p_type == PT_DYNAMIC) {
			dyn64 = map + phdr64[i].p_offset;
			// Patch dynamic segment
			for (j = 0; j < (phdr64[i].p_filesz / sizeof(Elf64_Dyn)); j++) {
				switch (dyn64->d_tag) {
				case DT_BIND_NOW:	// Remove BIND_NOW flag if present
					if (no_now_flag) {
						dyn64->d_tag = DT_NULL;
						dyn64->d_un.d_val = -1;
					}
					break;
				case DT_FLAGS_1:	// Remove DF_1_NOOPEN and DF_1_PIE flags if present
					dyn64->d_un.d_val = dyn64->d_un.d_val & ~DF_1_NOOPEN;
					dyn64->d_un.d_val = dyn64->d_un.d_val & ~DF_1_PIE;
					if (no_now_flag) {	// Remove DF_1_NOW flag                      
						dyn64->d_un.d_val = dyn64->d_un.d_val & ~DF_1_NOW;
					}
					break;

					// Optionally ignore constructors and destructors.
				case DT_INIT_ARRAYSZ:
					if (noinit) {
						dyn64->d_un.d_val = 0;
					}
					break;
				case DT_INIT_ARRAY:
					if (noinit) {
						dyn64->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAYSZ:
					if (noinit) {
						dyn64->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAY:
					if (noinit) {
						dyn64->d_un.d_val = 0;
					}
					break;

				case DT_VERNEED:
					if (strip_vernum) {
						dyn64->d_tag = DT_NULL;
						dyn64->d_un.d_val = -1;
					}
					break;

				case DT_VERNEEDNUM:
					if (strip_vernum) {
						dyn64->d_tag = DT_NULL;
						dyn64->d_un.d_val = -1;
					}
					break;

				default:
					break;
				}
				dyn64 += 1;
			}
			break;
		}
	}

	return 0;
}

char *display_relocation_type(int val)
{
	switch (val) {
	case 0:
		return "R_X86_64_NONE";
	case 1:
		return "R_X86_64_64";
	case 2:
		return "R_X86_64_PC32";
	case 3:
		return "R_X86_64_GOT32";
	case 4:
		return "R_X86_64_PLT32";
	case 5:
		return "R_X86_64_COPY";
	case 6:
		return "R_X86_64_GLOB_DAT";
	case 7:
		return "R_X86_64_JUMP_SLOT";
	case 8:
		return "R_X86_64_RELATIVE";
	case 9:
		return "R_X86_64_GOTPCREL";
	case 10:
		return "R_X86_64_32";
	case 11:
		return "R_X86_64_32S";
	case 12:
		return "R_X86_64_16";
	case 13:
		return "R_X86_64_PC16";
	case 14:
		return "R_X86_64_8";
	case 15:
		return "R_X86_64_PC8";
	case 16:
		return "R_X86_64_DTPMOD64";
	case 17:
		return "R_X86_64_DTPOFF64";
	case 18:
		return "R_X86_64_TPOFF64";
	case 19:
		return "R_X86_64_TLSGD";
	case 20:
		return "R_X86_64_TLSLD";
	case 21:
		return "R_X86_64_DTPOFF32";
	case 22:
		return "R_X86_64_GOTTPOFF";
	case 23:
		return "R_X86_64_TPOFF32";
	case 24:
		return "R_X86_64_PC64";
	case 25:
		return "R_X86_64_GOTOFF64";
	case 26:
		return "R_X86_64_GOTPC32";
	case 27:
		return "R_X86_64_GOT64";
	case 28:
		return "R_X86_64_GOTPCREL64";
	case 29:
		return "R_X86_64_GOTPC64";
	case 30:
		return "R_X86_64_GOTPLT64";
	case 31:
		return "R_X86_64_PLTOFF64";
	case 32:
		return "R_X86_64_SIZE32";
	case 33:
		return "R_X86_64_SIZE64";
	case 34:
		return "R_X86_64_GOTPC32_TLSDESC";
	case 35:
		return "R_X86_64_TLSDESC_CALL";
	case 36:
		return "R_X86_64_TLSDESC";
	case 37:
		return "R_X86_64_IRELATIVE";
	case 38:
		return "R_X86_64_RELATIVE64";
	case 41:
		return "R_X86_64_GOTPCRELX";
	case 42:
		return "R_X86_64_REX_GOTPCRELX";
	case 43:
		return "R_X86_64_NUM";
	default:
		break;
	}

	return "";
}

char *sym_binding(int val)
{
	switch (val) {
	case 0:
		return " LOCAL";
	case 1:
		return "GLOBAL";
	case 2:
		return "  WEAK";
	case 10:
		return "UNIQUE";
	default:
		break;
	}
	return "";
}

char *sym_type(int val)
{
	switch (val) {
	case 0:
		return " NOTYPE";
	case 1:
		return " OBJECT";
	case 2:
		return "   FUNC";
	case 3:
		return "SECTION";
	case 4:
		return "   FILE";
	case 5:
		return " COMMON";
	case 6:
		return "    TLS";		
	default:
		break;
	}
	return "";
}

char *sym_visibility(int val)
{
	switch (val) {
	case 0:
		return "  DEFAULT";
	case 1:
		return " INTERNAL";
	case 2:
		return "   HIDDEN";
	case 3:
		return "PROTECTED";
	default:
		break;
	}
	return "";
}

/**
* Process 64bits ELF relocations using Sections
*/
int fix_relocations_sections64(char *map)
{
	Elf64_Ehdr *elf64 = 0;
	Elf64_Shdr *shdr64 = 0;
	Elf64_Rela *rela64 = 0;
	Elf64_Sym *sym64 = 0;
	unsigned int shnum = 0;
	unsigned int i = 0, j = 0, k = 0;
	long int sym_to_patch[10240];
	unsigned int sym_to_patch_index = 0;

	memset(sym_to_patch, 0x00, sizeof(sym_to_patch));

	elf64 = (Elf64_Ehdr *) map;
	shdr64 = (Elf64_Shdr *) (map + elf64->e_shoff);
	shnum = elf64->e_shnum;

	if (!shnum) {
		printf("!! ERROR: Binary has no section headers, try using option -S\n");
		exit(EXIT_FAILURE);
	}

	// Fix relocations
	for (i = 0; i < shnum; i++) {
		// Find relocation sections
		if (shdr64[i].sh_type == SHT_RELA) {
			printf("found SHT_RELA section with index %u\n", i);
			rela64 = map + shdr64[i].sh_offset;
			for (j = 0; j < (shdr64[i].sh_size / sizeof(Elf64_Rela)); j++) {

				if (ELF64_R_TYPE(rela64->r_info) == 5) {
					printf("Relocation at index %u Found R_X86_64_COPY : turning into R_X86_64_GLOB_DAT for symbol %lu\n", j, ELF64_R_SYM(rela64->r_info));
					rela64->r_info++; // 0x008000000006;

					if(sym_to_patch_index >= 10240-10) {
						printf("!! ERROR: Too many relocations to patch\n");
						exit(EXIT_FAILURE);
					}
					
					sym_to_patch[sym_to_patch_index++] = ELF64_R_SYM(rela64->r_info);

				}
				printf("[%03u] 0x%ld %s 0x%ld 0x%lx\n", j, rela64->r_offset, display_relocation_type(ELF64_R_TYPE(rela64->r_info)),
					ELF64_R_SYM(rela64->r_info), rela64->r_addend);
				rela64 += 1;
			}	
		}
	}

	// Fix Symbols
	for (i = 0; i < shnum; i++) {
		// Find Dynamic Symbol Table
		if (shdr64[i].sh_type == SHT_DYNSYM) {
			sym64 = map + shdr64[i].sh_offset;
			char *dynstr = map + shdr64[i+1].sh_offset;			
			for (j = 0; j < (shdr64[i].sh_size / sizeof(Elf64_Sym)); j++) {

				if (j == sym_to_patch[k]) {
					printf("Patching symbol at index: %u\n", j);
//					sym64->st_value = 0;
					sym64->st_size  += 100;
//					sym64->st_shndx = 0;
					
					k++;
				}

				printf("[%03u] %016lx  %02lu %s %s %s % 4u  %s\n", j, sym64->st_value, sym64->st_size, sym_type(ELF64_ST_TYPE(sym64->st_info)), sym_binding(ELF64_ST_BIND(sym64->st_info)), sym_visibility(sym64->st_other), sym64->st_shndx ,sym64->st_name + dynstr);
				sym64 += 1;
			}
		}
	}

	return 0;
}

/**
* Process 64bits ELF binary using Sections
*/
int process_sections64(char *map, unsigned int noinit, unsigned int strip_vernum, unsigned int no_now_flag)
{
	Elf64_Ehdr *elf64 = 0;
	Elf64_Shdr *shdr64 = 0;
	Elf64_Dyn *dyn64 = 0;
	unsigned int shnum = 0;
	unsigned int i = 0, j = 0;

	elf64 = (Elf64_Ehdr *) map;
	shdr64 = (Elf64_Shdr *) (map + elf64->e_shoff);
	shnum = elf64->e_shnum;

	if (!shnum) {
		printf("!! ERROR: Binary has no section headers, try using option -S\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < shnum; i++) {
		// Find dynamic section
		if (shdr64[i].sh_type == SHT_DYNAMIC) {
			dyn64 = map + shdr64[i].sh_offset;
			// Patch dynamic section
			for (j = 0; j < (shdr64[i].sh_size / sizeof(Elf64_Dyn)); j++) {
				switch (dyn64->d_tag) {
				case DT_BIND_NOW:	// Remove BIND_NOW flag if present
					if (no_now_flag) {
						dyn64->d_tag = DT_NULL;
						dyn64->d_un.d_val = -1;
					}
					break;
				case DT_FLAGS_1:	// Remove DF_1_NOOPEN and DF_1_PIE flags if present
					dyn64->d_un.d_val = dyn64->d_un.d_val & ~DF_1_NOOPEN;
					dyn64->d_un.d_val = dyn64->d_un.d_val & ~DF_1_PIE;
					if (no_now_flag) {	// Remove DF_1_NOW flag                      
						dyn64->d_un.d_val = dyn64->d_un.d_val & ~DF_1_NOW;
					}
					
					// Turn into a DT_FLAG with value DF_SYMBOLIC
//					dyn64->d_tag = DT_FLAGS;
//					dyn64->d_un.d_val = DF_1_GLOBAL|DF_1_NOW|DF_1_NOOPEN|DF_1_INITFIRST;
					
					break;

					// Optionally ignore constructors and destructors.
				case DT_INIT_ARRAYSZ:
					if (noinit) {
						dyn64->d_un.d_val = 0;
					}
					break;
				case DT_INIT_ARRAY:
					if (noinit) {
						dyn64->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAYSZ:
					if (noinit) {
						dyn64->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAY:
					if (noinit) {
						dyn64->d_un.d_val = 0;
					}
					break;

				case DT_VERNEED:
					if (strip_vernum) {
						dyn64->d_tag = DT_NULL;
						dyn64->d_un.d_val = -1;
					}
					break;

				case DT_VERNEEDNUM:
					if (strip_vernum) {
						dyn64->d_tag = DT_NULL;
						dyn64->d_un.d_val = -1;
					}
					break;

				default:
					break;
				}
				dyn64 += 1;
			}
			break;
		}
		// Find .gnu.version section and wipe it clear
		if (shdr64[i].sh_type == SHT_GNU_versym) {
			memset(map + shdr64[i].sh_offset, 0x00, shdr64[i].sh_size);
		}
	}

	return 0;
}

/**
* Process 32bits ELF binary using Segments
*/
int process_segments32(char *map, unsigned int noinit, unsigned int strip_vernum, unsigned int no_now_flag)
{
	Elf_Ehdr *elf32 = 0;
	Elf32_Phdr *phdr32 = 0;
	Elf32_Dyn *dyn32 = 0;
	unsigned int phnum = 0;
	unsigned int i = 0, j = 0;

	elf32 = (Elf32_Ehdr *) map;
	phdr32 = (Elf32_Phdr *) (map + elf32->e_phoff);
	phnum = elf32->e_phnum;

	for (i = 0; i < phnum; i++) {
		if (phdr32[i].p_type == PT_DYNAMIC) {
			dyn32 = map + phdr32[i].p_offset;
			// Patch dynamic segment
			for (j = 0; j < (phdr32[i].p_filesz / sizeof(Elf32_Dyn)); j++) {
				switch (dyn32->d_tag) {
				case DT_BIND_NOW:	// Remove BIND_NOW flag if present
					if (no_now_flag) {
						dyn32->d_tag = DT_NULL;
						dyn32->d_un.d_val = -1;
					}
					break;
				case DT_FLAGS_1:	// Remove DF_1_NOOPEN and DF_1_PIE flags if present
					dyn32->d_un.d_val = dyn32->d_un.d_val & ~DF_1_NOOPEN;
					dyn32->d_un.d_val = dyn32->d_un.d_val & ~DF_1_PIE;
					if (no_now_flag) {	// Remove DF_1_NOW flag                      
						dyn32->d_un.d_val = dyn32->d_un.d_val & ~DF_1_NOW;
					}
					break;

					// Optionally ignore constructors and destructors.
				case DT_INIT_ARRAYSZ:
					if (noinit) {
						dyn32->d_un.d_val = 0;
					}
					break;
				case DT_INIT_ARRAY:
					if (noinit) {
						dyn32->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAYSZ:
					if (noinit) {
						dyn32->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAY:
					if (noinit) {
						dyn32->d_un.d_val = 0;
					}
					break;

				case DT_VERNEED:
					if (strip_vernum) {
						dyn32->d_tag = DT_NULL;
						dyn32->d_un.d_val = -1;
					}
					break;

				case DT_VERNEEDNUM:
					if (strip_vernum) {
						dyn32->d_tag = DT_NULL;
						dyn32->d_un.d_val = -1;
					}
					break;

				default:
					break;
				}
				dyn32 += 1;
			}
			break;
		}
	}

	return 0;
}

/**
* Process 32bits ELF binary using Sections
*/
int process_sections32(char *map, unsigned int noinit, unsigned int strip_vernum, unsigned int no_now_flag)
{
	Elf32_Ehdr *elf32 = 0;
	Elf32_Shdr *shdr32 = 0;
	Elf32_Dyn *dyn32 = 0;
	unsigned int shnum = 0;
	unsigned int i = 0, j = 0;

	elf32 = (Elf32_Ehdr *) map;
	shdr32 = (Elf32_Shdr *) (map + elf32->e_shoff);
	shnum = elf32->e_shnum;

	if (!shnum) {
		printf("!! ERROR: Binary has no section headers, try using option -S\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < shnum; i++) {
		// Find dynamic section
		if (shdr32[i].sh_type == SHT_DYNAMIC) {
			dyn32 = map + shdr32[i].sh_offset;
			// Patch dynamic section
			for (j = 0; j < (shdr32[i].sh_size / sizeof(Elf32_Dyn)); j++) {
				switch (dyn32->d_tag) {
				case DT_BIND_NOW:	// Remove BIND_NOW flag if present
					if (no_now_flag) {
						dyn32->d_tag = DT_NULL;
						dyn32->d_un.d_val = -1;
					}
					break;
				case DT_FLAGS_1:	// Remove DF_1_NOOPEN and DF_1_PIE flags if present
					dyn32->d_un.d_val = dyn32->d_un.d_val & ~DF_1_NOOPEN;
					dyn32->d_un.d_val = dyn32->d_un.d_val & ~DF_1_PIE;
					if (no_now_flag) {	// Remove DF_1_NOW flag                      
						dyn32->d_un.d_val = dyn32->d_un.d_val & ~DF_1_NOW;
					}
					break;

					// Optionally ignore constructors and destructors.
				case DT_INIT_ARRAYSZ:
					if (noinit) {
						dyn32->d_un.d_val = 0;
					}
					break;
				case DT_INIT_ARRAY:
					if (noinit) {
						dyn32->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAYSZ:
					if (noinit) {
						dyn32->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAY:
					if (noinit) {
						dyn32->d_un.d_val = 0;
					}
					break;

				case DT_VERNEED:
					if (strip_vernum) {
						dyn32->d_tag = DT_NULL;
						dyn32->d_un.d_val = -1;
					}
					break;

				case DT_VERNEEDNUM:
					if (strip_vernum) {
						dyn32->d_tag = DT_NULL;
						dyn32->d_un.d_val = -1;
					}
					break;

				default:
					break;
				}
				dyn32 += 1;
			}
			break;
		}
		// Find .gnu.version section and wipe it clear
		if (shdr32[i].sh_type == SHT_GNU_versym) {
			memset(map + shdr32[i].sh_offset, 0x00, shdr32[i].sh_size);
		}
	}

	return 0;
}

/**
* Patch ELF ehdr->e_type to ET_DYN
*/
int mk_lib(char *name, unsigned int noinit, unsigned int strip_vernum, unsigned int no_now_flag, unsigned int use_segments)
{
	int fd = 0;
	struct stat sb;
	char *map = 0;
	Elf32_Ehdr *ehdr32;
	Elf64_Ehdr *ehdr64;

	fd = open(name, O_RDWR);
	if (fd <= 0) {
		printf("!! ERROR: couldn't open %s : %s\n", name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fstat(fd, &sb) == -1) {
		printf("!! ERROR: couldn't stat %s : %s\n", name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ((unsigned int) sb.st_size < sizeof(Elf32_Ehdr)) {
		printf("!! ERROR: file %s is too small (%u bytes) to be a valid ELF.\n", name, (unsigned int) sb.st_size);
		exit(EXIT_FAILURE);
	}

	map = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		printf("!! ERROR: couldn't mmap %s : %s\n", name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	switch (map[EI_CLASS]) {
	case ELFCLASS32:
		ehdr32 = (Elf32_Ehdr *) map;
		ehdr32->e_type = ET_DYN;
		if (use_segments) {
			process_segments32(map, noinit, strip_vernum, no_now_flag);
		} else {
			process_sections32(map, noinit, strip_vernum, no_now_flag);
		}
		break;
	case ELFCLASS64:
		ehdr64 = (Elf64_Ehdr *) map;
		ehdr64->e_type = ET_DYN;
		if (use_segments) {
			process_segments64(map, noinit, strip_vernum, no_now_flag);
		} else {
			process_sections64(map, noinit, strip_vernum, no_now_flag);
			fix_relocations_sections64(map);
		}
		break;
	default:
		printf("!! ERROR: unknown ELF class\n");
		exit(EXIT_FAILURE);
	}

	munmap(map, sb.st_size);
	close(fd);
	return 0;
}

