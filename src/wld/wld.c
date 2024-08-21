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
	{"libify", no_argument, 0, 'l'},
	{"noinit", no_argument, 0, 'n'},
	{"nonow", no_argument, 0, 'N'},
	{"strip-symbol-versions", no_argument, 0, 's'},
	{0, 0, 0, 0}
};

/**
* Patch ELF ehdr->e_type to ET_DYN
*/
int mk_lib(char *name, unsigned int noinit, unsigned int strip_vernum, unsigned int no_now_flag)
{
	int fd;
	struct stat sb;
	char *map = 0;
	Elf32_Ehdr *ehdr32;
	Elf64_Ehdr *ehdr64;

	Elf_Ehdr *elf = 0;
	Elf_Shdr *shdr = 0;
	Elf_Dyn *dyn;
	unsigned int shnum = 0;
	unsigned int i = 0, j = 0;

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
		break;
	case ELFCLASS64:
		ehdr64 = (Elf64_Ehdr *) map;
		ehdr64->e_type = ET_DYN;
		break;
	default:
		printf(" !! unknown ELF class\n");
		exit(EXIT_FAILURE);
	}

	/**
	* Work around : https://patchwork.ozlabs.org/project/glibc/patch/20190312130235.8E82C89CE49C@oldenburg2.str.redhat.com/
	* We need to find and nullify entries DT_FLAGS, DT_FLAGS_1 and DT_BIND_NOW in dynamic section
	*/
	elf = (Elf_Ehdr *) map;
	shdr = (Elf_Shdr *) (map + elf->e_shoff);
	shnum = elf->e_shnum;

	for (i = 0; i < shnum; i++) {
		// Find dynamic section
		if (shdr[i].sh_type == SHT_DYNAMIC) {
			dyn = map + shdr[i].sh_offset;
			// Patch dynamic section
			for (j = 0; j < (shdr[i].sh_size / sizeof(Elf_Dyn)); j++) {
				switch (dyn->d_tag) {
				case DT_BIND_NOW:	// Remove BIND_NOW flag if present
					dyn->d_tag = DT_NULL;
					dyn->d_un.d_val = -1;
					break;
				case DT_FLAGS_1:	// Remove DF_1_NOOPEN and DF_1_PIE flags if present
					dyn->d_un.d_val = dyn->d_un.d_val & ~DF_1_NOOPEN;
					dyn->d_un.d_val = dyn->d_un.d_val & ~DF_1_PIE;
					if (no_now_flag) { // Remove DF_1_NOW flag			
						dyn->d_un.d_val = dyn->d_un.d_val & ~DF_1_NOW;
					}
					break;

					// Optionally ignore constructors and destructors.
				case DT_INIT_ARRAYSZ:
					if (noinit) {
						dyn->d_un.d_val = 0;
					}
					break;
				case DT_INIT_ARRAY:
					if (noinit) {
						dyn->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAYSZ:
					if (noinit) {
						dyn->d_un.d_val = 0;
					}
					break;
				case DT_FINI_ARRAY:
					if (noinit) {
						dyn->d_un.d_val = 0;
					}
					break;

				case DT_VERNEED:
					if (strip_vernum) {
						dyn->d_tag = DT_NULL;
						dyn->d_un.d_val = -1;
					}
					break;

				case DT_VERNEEDNUM:
					if (strip_vernum) {
						dyn->d_tag = DT_NULL;
						dyn->d_un.d_val = -1;
					}
					break;

				default:
					break;
				}
				dyn += 1;
			}
			break;
		}
		// Find .gnu.version section and wipe it clear
		if (shdr[i].sh_type == SHT_GNU_versym) {
			memset(map + shdr[i].sh_offset, 0x00, shdr[i].sh_size);
		}

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
      	printf("\nUsage: %s <-l|--libify> [-n|--noinit] [-s|--strip-symbol-versions] [-N|--nonow] file\n", name);
      	printf("\nOptions:\n    --libify (-l)                         Transform executable into shared library.\n");
      	printf("    --noinit (-n)                         Ignore constructors and desctructors in output library.\n");
      	printf("    --nonow (-N)                          Remove BIND_NOW flag in output library.\n");
      	printf("    --strip-symbol-versions (-s)          Strip symbol versions in output library.\n");

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

	// assume given argument is an input file, find absolute path
	target = realpath(argv[optind], 0);

	mk_lib(target, noinit_flag, strip_vernum_flag, no_now_flag);

	return 0;
}
