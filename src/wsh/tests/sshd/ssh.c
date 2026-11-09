/**
* Test code for the Witchcraft Compiler Collection
*
* Copyright 2016-2026 Jonathan Brossard.
*
* This file is licensed under the MIT License.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "ssh.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char **argv, char **envp){
	int ret = 0;

	printf("calling setlogin()\n");
	ret = setlogin(0x41414141);
	printf("returned successfully: %d\n", ret);
	return 0;
}

