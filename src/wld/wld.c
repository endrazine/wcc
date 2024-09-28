/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2024 Jonathan Brossard
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
* Work around : https://patchwork.ozlabs.org/project/glibc/patch/20190312130235.8E82C89CE49C@oldenburg2.str.redhat.com/
* We need to find and nullify entries DT_FLAGS, DT_FLAGS_1 and DT_BIND_NOW in dynamic section
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

const struct option long_options[] = {
	{ "libify", no_argument, 0, 'l' },
	{ "no-init", no_argument, 0, 'n' },
	{ "no-bind-now", no_argument, 0, 'N' },
	{ "strip-symbol-versions", no_argument, 0, 's' },
	{ "use-segments", no_argument, 0, 'S' },
	{ 0, 0, 0, 0 }
};

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
		printf(" !! ERROR: Binary has no section headers\n");
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
		printf(" !! ERROR: Binary has no section headers\n");
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
		printf(" !! couldn't open %s : %s\n", name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fstat(fd, &sb) == -1) {
		printf(" !! couldn't stat %s : %s\n", name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ((unsigned int) sb.st_size < sizeof(Elf32_Ehdr)) {
		printf(" !! file %s is too small (%u bytes) to be a valid ELF.\n", name, (unsigned int) sb.st_size);
		exit(EXIT_FAILURE);
	}

	map = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		printf(" !! couldn't mmap %s : %s\n", name, strerror(errno));
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
		}
		break;
	default:
		printf(" !! unknown ELF class\n");
		exit(EXIT_FAILURE);
	}

	munmap(map, sb.st_size);
	close(fd);
	return 0;
}

int print_version(void)
{
	printf("%s version:%s    (%s %s)\n", WNAME, WVERSION, WTIME, WDATE);
	return 0;
}

int usage(char *name)
{
	print_version();
	printf("\nUsage: %s <-l|--libify> [-n|--no-init] [-s|--strip-symbol-versions] [-N|--no-bind-now] [-S|--use-segments] file\n", name);
	printf("\nOptions:\n");
	printf("    --libify (-l)                         Transform executable into shared library.\n");
	printf("    --no-init (-n)                        Remove constructors and desctructors from output library.\n");
	printf("    --no-bind-now (-N)                    Remove BIND_NOW flag from output library.\n");
	printf("    --strip-symbol-versions (-s)          Strip symbol versions from output library.\n");
	printf("    --use-segments (-S)                   Process binary using segments rather than sections\n");

	return 0;
}

int main(int argc, char **argv)
{
	char c = 0;
	int option_index = 0;
	char *target = 0;
	int libify_flag = 0;
	int noinit_flag = 0;
	int strip_vernum_flag = 0;
	int no_now_flag = 0;
	int use_segments_flag = 0;

	if ((argc < 2)) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	while (1) {

		c = getopt_long(argc, argv, "lnNs", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {

		case 'l':
			libify_flag = 1;
			break;

		case 'n':
			noinit_flag = 1;
			break;

		case 'N':
			no_now_flag = 1;
			break;

		case 's':
			strip_vernum_flag = 1;
			break;

		case 'S':
			use_segments_flag = 1;
			break;

		default:
			fprintf(stderr, " [!!] unknown option : '%c'\n", c);
			return NULL;

		}
	}

	if (optind == argc) {
		printf("\nNot enough parameters\n\n");
		usage(argv[0]);
	}

	if (optind != argc - 1) {
		printf("\nToo many parameters\n\n");
		usage(argv[0]);
	}

	if (!libify_flag) {
		printf("\n--libify option not set : not processing\n\n");
		usage(argv[0]);
	}
	// assume given argument is an input file, find absolute path
	target = realpath(argv[optind], 0);

	mk_lib(target, noinit_flag, strip_vernum_flag, no_now_flag, use_segments_flag);

	return 0;
}
