/**
* Calling pr_version_get_str() from Proftpd.so
*
* endrazine for Defcon 24 // August 2016
*/
#include <stdio.h>
#include <dlfcn.h>

int main(void){
	char* (*getversion)() = NULL;
	void *handle;
	handle = dlopen("/tmp/proftpd.so", RTLD_LAZY);
	getversion = dlsym(handle, "pr_version_get_str");
	printf("Using proftpd.so version: \e[31m%s\e[0m\n", getversion());
	return 0;
}
