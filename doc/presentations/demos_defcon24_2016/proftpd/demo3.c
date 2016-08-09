/**
* Calling the main() ... within proftpd.so
*
* Note: yes, we have a "shared library" with a main() !!
*
* Note 2: you're not supposed to call main directly,
* you may want to pass its address as an argument
* to __libc_start_main() instead.
*
* endrazine for Defcon 24 // August 2016
*/
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

static int (*__main)(int argc, char **argv) = NULL;

int get_symbol(char *filename, char *symbolname){
	void *handle;
	char *error = 0;

	handle = dlopen(filename, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	__main = dlsym(handle, symbolname);

	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "%s\n", error);
		exit(EXIT_FAILURE);
	}

	return 0;
}

int main(void){
	char *argz[] = {"/bin/foo", 0x00};

	get_symbol("/tmp/proftpd.so", "main");

	__main(1, argz);	// call main() from proftpd.so

	return 0;
}


