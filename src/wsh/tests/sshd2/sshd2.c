/**
* Test code for the Witchcraft Compiler Collection
*
* Copyright 2016 Jonathan Brossard.
*
* This file is licensed under the MIT License.
*
* Note:
* The whole trick to call main() within sshd after
* linking against /usr/sbin/sshd is to have our
* main() function really called __main(Ã)
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "ssh.h"

// Forward declarations
int main (int argc, char **argv, char **envp);
extern int (__libc_start_main) (int (*main) (int, char **, char **),
		int argc,
		char **ubp_av,
		void (*init) (void),
		void (*fini) (void),
		void (*rtld_fini) (void),
		void *stack_end)
__attribute__ ((noreturn));



// Optional constructor
__attribute__ ((__constructor__))  void init_me(void) {
	printf(" [*] Calling main() from sshd...\n");
}



unsigned long long getrsp()
{
  __asm__ ("movq %rsp, %rax");
}


// Entry point
int __main (){
	printf("let's call main() within sshd\n");

//	main(0,"foobar", 0);	// short version

	// or pass arguments...
	unsigned int myargc;
	char *myargv[]={"/usr/sbin/sshd", "-h", 0x00};
	myargc = 2;
	__libc_start_main(main, myargc, myargv, _init, _fini, 0, 0x7ffffffff000 /*(getrsp() & ~0xfff) + 0x1000*/);
	return 0;
}


