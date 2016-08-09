/**
* Demo application for WCC
* as demoed at Defcon 24.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

static char foobar[10];
char *unused1 = "This string is made to fill the top of the .rodata section\n";


extern char *program_invocation_name;

static int wantedlen(char *str){	// Internal function, no entry in dynsym
	return strlen(str)+10;		// relocation to strlen@plt
}

int do_something(char *msg){

	static char *buff = 4;		// Global initialized : @data

	buff = calloc(1, wantedlen(msg));	// relocations to calloc@plt and wantedlen@text	// write to relocation buff@data
					// heap addresses do not require relocations
	if(!buff){
		printf("error in calloc : %s",	// relocation to perror@plt and string@rodata
			strerror(errno));	// relocations to strerror@plt and errno@bss (imported global variable)
		exit(-1);		// relocation to exit@plt
	}

	sprintf(buff,"Hello %s from %s\n",msg, program_invocation_name);		// relocations to sprintf@plt, %s@rodata // relocation to buff@data

	fprintf(stderr, buff);			// relocation to fprintf@plt and stderr@bss (imported global variable)

	return 42;
}

int main (int argc, char **argv){	// arguments are passed to main@plt via __libc_start_main@plt called from _start@text

	int n = 0;
        char *msg = "Defcon !";	// Relocation to string@rodata

	// arguments passing and return values
	// require no relocations
	n = do_something(msg);		// relocation to do_something@text  : This is actually a rip relative call.
					// if we do not relocate it, it will work fine. It will too if we relocate it !

	exit(n);			// relocation to exit@plt

	return 0;
}

