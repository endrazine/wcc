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
*/


#define _XOPEN_SOURCE 500
#define _FILE_OFFSET_BITS 64
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <string.h>

#include <libwitch/helper.h>

extern unsigned int lastsignal;

#ifndef HAS_ZFIRST
#define HAS_ZFIRST 1
struct section *zfirst = 0;
int nsections=0;
#else
extern struct section *zfirst;
extern int nsections;
#endif

/*
* Is a given address mapped ?
*/
int is_mapped(unsigned long int addr){

	struct section *tmpsection = zfirst;
	while (tmpsection != 0x00) {
		if ((tmpsection->init <= addr) && (tmpsection->end > addr)) {
			// return size to end of section
			return tmpsection->end - addr;
		}
		tmpsection = tmpsection->next;
	}
	return 0;
}

/*
* read /proc/pid/map
*/
int read_maps(int pid)
{
	char mpath[255];
	FILE *fmaps;
	char line[1000];
#ifdef __x86_64__
	unsigned long long int initz, endz, size;
#else
	unsigned long int initz, endz, size;
#endif
	char *name;
	unsigned int counter=1;
	struct section *zptr;
	unsigned int perms, t;
	int delta;

	// path to maps file
	sprintf(mpath, "/proc/%d/maps", pid);
	fmaps = fopen(mpath, "r");

	if (fmaps == NULL) {
		perror("[!!] Error reading maps file");
		exit(1);	   
	}

	while ( fgets ( line, sizeof line, fmaps ) != NULL ) {
#ifdef __x86_64__

		// we first need to check if the possible address is a 32 or 64 one		
		initz = strtoul(line, NULL, 16);
		endz = strtoul(strchr(line, '-')+1, NULL, 16);		
		size = endz - initz;

		delta=strchr(line, ' ')-line;
#else
		delta=18;
		endz = strtoull(line + 9, NULL, 16);
		initz = strtoull(line + 0, NULL, 16);
		size = endz - initz;
#endif

		// find permissions
		perms = 0;
		char hperms[5];
		memset(hperms,0x00,5);
		memcpy(hperms,line+delta,4);
		for (t = 0; t < 4; t++) {
			switch (line[t + delta]) {
			case 'r':
				perms += 2;	/*printf(" read "); */
				break;
			case 'w':
				perms += 4;	/*printf(" write "); */
				break;
			case 'x':
				perms += 1;	/*printf(" execute "); */
				break;
			case 'p':		/*printf(" private "); */
				break;
			case 's':
				perms += 8;	/*printf(" shared "); */
				break;
			}
		}

		// find name
		strtok(line, " ");
		for (t=0;t<5;t++) {
			name = strtok(NULL, " ");			
		}
		// Remove leading spaces
		while(*name != '\0' && isspace(*name))
		{
			++name;
		}
		// Remove trailing newline
		name[strlen(name) - 1] = '\0';
		
		// Omit vsyscall as pread fails for the last address
		if (!strncmp("[vsyscall]", name,10)) 
			continue;

		// add to linked list
		zptr = (struct section *)malloc(sizeof(struct section));		
		memset(zptr, 0x00, sizeof(struct section));
		zptr->init = initz;
		zptr->end = endz;
		zptr->size = size;
		zptr->perms = perms;
		strcpy(zptr->hperms, hperms);
		zptr->num=counter++;
		strcpy(zptr->name, name);

		if (zfirst == 0x00) {	// we are first
			zfirst = zptr;

		} else {	// append
			struct section *tmpsection = zfirst;
			while (tmpsection->next != 0x00) {
				tmpsection = tmpsection->next;
			}
			tmpsection->next = zptr;

		}
	}

	fclose(fmaps);
	nsections=counter-1;
	return 0;
}


