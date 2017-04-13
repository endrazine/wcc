/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016 Jonathan Brossard
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


#include <config.h>
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
#include <libelf.h>
#include <gelf.h>

#define DEFAULT_NAME "wld"

/**
* Patch ELF ehdr->e_type to ET_DYN
*/
int mk_lib(char *name)
{
  int fd;
  struct stat sb;
  char *map = 0;
  Elf32_Ehdr *ehdr32;
  Elf64_Ehdr *ehdr64;

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

  munmap(map, sb.st_size);
  close(fd);
  return 0;
}

int print_version(void)
{
	printf("%s (%s %s)\n", PACKAGE_STRING, __DATE__, __TIME__);
  return 0;
}

int main(int argc, char **argv)
{

  if ((argc < 2)||(strncmp(argv[1],"-libify",7))) {
    print_version();
    printf("\nUsage: %s [options] file\n", argc > 0 ? argv[0] : DEFAULT_NAME);
    printf("\noptions:\n\n    -libify          Set Class to ET_DYN in input ELF file.\n\n");
    exit(EXIT_FAILURE);
  }

  mk_lib(argv[2]);

  return 0;
}
