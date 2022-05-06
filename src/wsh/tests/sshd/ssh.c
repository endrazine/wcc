/**
* Test code for the Witchcraft Compiler Collection
*
* Copyright 2016-2022 Jonathan Brossard.
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

int main (int argc, char **argv, char **envp){
	int ret;

	printf("calling get_hostkey_index()\n");
	ret = get_hostkey_index(0x41414141, 0x42424242, 0x43434343, 0x444444444, 0x45454545, 0x46464646);
	printf("returned: %d\n", ret);
	return 0;
}

