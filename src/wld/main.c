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
* Imported function prototype
*/
int mk_lib(char *name, unsigned int noinit, unsigned int strip_vernum, unsigned int no_now_flag, unsigned int use_segments);


const struct option long_options[] = {
	{ "libify", no_argument, 0, 'l' },
	{ "no-init", no_argument, 0, 'n' },
	{ "no-bind-now", no_argument, 0, 'N' },
	{ "strip-symbol-versions", no_argument, 0, 's' },
	{ "use-segments", no_argument, 0, 'S' },
	{ 0, 0, 0, 0 }
};



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

		c = getopt_long(argc, argv, "lnNsS", long_options, &option_index);
		if ((c == 0xff)||(c == -1)) {
			break;
		}

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
			fprintf(stderr, "!! ERROR: unknown option : '%c'\n", c);
			return NULL;

		}
	}

	if (optind == argc) {
		printf("\n!! ERROR: Not enough parameters\n\n");
		usage(argv[0]);
	}

	if (optind != argc - 1) {
		printf("\n!! ERROR: Too many parameters\n\n");
		usage(argv[0]);
	}

	if (!libify_flag) {
		printf("\n!! ERROR: --libify option not set : not processing\n\n");
		usage(argv[0]);
	}
	// assume given argument is an input file, find absolute path
	target = realpath(argv[optind], 0);

	mk_lib(target, noinit_flag, strip_vernum_flag, no_now_flag, use_segments_flag);

	return 0;
}

