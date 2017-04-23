/**
*
* Witchcraft Compiler Collection - wldd
*
* Author: Markus Gothe <nietzsche@lysator.liu.se>
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2017 Markus Gothe
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
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>


#ifdef HAVE_LIBELF_LIBELF_H
#include <libelf/libelf.h>
#else
#include <libelf.h>
#endif

#ifdef HAVE_LIBELF_GELF_H
#include <libelf/gelf.h>
#else
#include <gelf.h>
#endif

int main(int argc, char **argv)
{
    int fd;
    Elf *e;
    char *lib, *tmplib;
    GElf_Ehdr ehdr; /* Stack allocate this */
    Elf_Scn *scn;
    Elf_Data *data;
    GElf_Shdr  shdr; /* Stack allocate this */
    GElf_Dyn  dyn; /* Stack allocate this */
    size_t n, shstrndx , sz;
    int found_dynamic_section = 0, found_lib = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    if ((fd = open(argv[1], O_RDONLY, 0)) < 0) {
        fprintf(stderr, "open() failed: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }


    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
        fprintf(stderr, "elf_begin() failed: %s\n", elf_errmsg(-1));
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (elf_kind(e) != ELF_K_ELF) {
        fprintf(stderr, "%s is not an ELF object.\n", elf_errmsg ( -1));
        goto exit_fail;
    }

    if (elf_getident(e, NULL) == NULL) {
        fprintf(stderr, "elf_getident() failed: %s\n", elf_errmsg ( -1));
        goto exit_fail;
    }

    if (gelf_getehdr(e, &ehdr) == NULL) {
        fprintf(stderr, "gelf_getehdr() failed: %s\n", elf_errmsg ( -1));
        goto exit_fail;
    }

    if(ehdr.e_type != ET_EXEC && ehdr.e_type != ET_EXEC) {
        fprintf(stderr, "%s is not a dynamic executable.\n", argv[1]);
        goto exit_fail;
    }

    if (gelf_getclass(e) == ELFCLASSNONE) {
        fprintf(stderr, "gelf_getclass() failed: %s\n", elf_errmsg ( -1));
        goto exit_fail;
    }

    if (elf_getshdrstrndx(e, &shstrndx) != 0) {
        fprintf(stderr, "elf_getshdrstrndx() failed: %s\n", elf_errmsg ( -1));
        goto exit_fail;
    }

    scn = NULL;
    while  ((scn = elf_nextscn(e, scn)) != NULL) {
        if (gelf_getshdr(scn , &shdr) != &shdr) {
            fprintf(stderr, "getshdr() failed: %s\n", elf_errmsg ( -1));
            goto exit_fail;
        }
        if(shdr.sh_type == SHT_DYNAMIC) {
            found_dynamic_section = 1;
            break;
        }
    }

    if(found_dynamic_section != 0) {
        /* Parse dynamic section and look for DT_NEEDED entries */
        if ((data = elf_getdata(scn, NULL)) == NULL) {
            fprintf(stderr, "elf_getdata() failed: %s\n", elf_errmsg ( -1));
            goto exit_fail;
        }

        for (size_t j = 0; j < data->d_size / shdr.sh_entsize; j++) {
            if (gelf_getdyn(data, (int)j, &dyn) == NULL) {
                fprintf(stderr, "gelf_getdyn() failed: %s\n", elf_errmsg ( -1));
                goto exit_fail;
            }
            if (dyn.d_tag == DT_NEEDED) {
                /* Wow, we found an entry! */
                if ((lib = elf_strptr(e, shdr.sh_link, dyn.d_un.d_ptr)) == NULL)
                {
                    fprintf(stderr, "elf_strpt() failed: %s\n", elf_errmsg ( -1));
                    goto exit_fail;
                }
                
                /* Parse out the library's name */
                tmplib = calloc(strlen(lib),  sizeof(char));
                sscanf(lib, "lib%[^.]", tmplib);
                printf("-l%s ", tmplib);
                free(tmplib);
                found_lib = 1;
            }
        }
        if(found_lib != 0) {
            printf("\n");
        }
    }

    (void) elf_end(e);
    (void) close(fd);
    exit(EXIT_SUCCESS);

exit_fail:
    (void) elf_end(e);
    (void) close(fd);
    exit(EXIT_FAILURE);
}
