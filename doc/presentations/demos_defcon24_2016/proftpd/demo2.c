/**
* Calling pr_version_get_str() from Proftpd.so
* and display memory map exported via /proc/
*
* endrazine for Defcon 24 // August 2016
*/
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* (*getversion)(void) = NULL;

int get_symbol(char *filename, char *symbolname){
	void *handle;
	char *error = 0;

	handle = dlopen(filename, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	getversion = dlsym(handle, symbolname);

	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "%s\n", error);
		exit(EXIT_FAILURE);
	}

	return 0;
}

int print_map(void){
	char cmd[256];

	memset(cmd, 0x00, 256);
	snprintf(cmd, 255, "cat /proc/%u/maps", getpid());
	system(cmd);
	return 0;
}

int main(void){
	get_symbol("/tmp/proftpd.so", "pr_version_get_str");
	printf("Using proftpd.so version: %s\n", getversion());
	print_map();
	return 0;
}

