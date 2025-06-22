/**
*
* Witchcraft Compiler Collection - wldd
*
* Authors: 
* Markus Gothe <nietzsche@lysator.liu.se>
* Jonathan Brossard <endrazine@gmail.com>
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

#include <libelf.h>
#include <gelf.h>


int opt_verbose = 0;

#include <fcntl.h>
#include <fcntl.h>            /* Definition of AT_* constants */
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <link.h>

// Function to search for library in standard paths
char* find_library_path(const char* libname) {
    // Check if the name already contains a path
    if (strchr(libname, '/') != NULL) {
        if (access(libname, F_OK) == 0) {
            return realpath(libname, NULL);
        }
        return NULL;
    }

    // Get LD_LIBRARY_PATH environment variable
    char* ld_path = getenv("LD_LIBRARY_PATH");
    if (ld_path != NULL) {
        char* ld_copy = strdup(ld_path);
        char* path = strtok(ld_copy, ":");

        while (path != NULL) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, libname);
            
            if (access(full_path, F_OK) == 0) {
                free(ld_copy);
                return realpath(full_path, NULL);
            }
            
            path = strtok(NULL, ":");
        }
        free(ld_copy);
    }

    // Check standard library paths
    const char* standard_paths[] = {
        "/lib",
        "/usr/lib",
        "/usr/local/lib",
        "/lib/x86_64-linux-gnu",  // Common on Debian/Ubuntu
        "/usr/lib/x86_64-linux-gnu",
        NULL
    };

    for (int i = 0; standard_paths[i] != NULL; i++) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", standard_paths[i], libname);
        
        if (access(full_path, F_OK) == 0) {
            return realpath(full_path, NULL);
        }
    }

    // Check paths from /etc/ld.so.conf
    FILE* conf = fopen("/etc/ld.so.conf", "r");
    if (conf != NULL) {
        char line[1024];
        while (fgets(line, sizeof(line), conf)) {
            // Remove newline
            line[strcspn(line, "\n")] = 0;
            
            // Skip comments and empty lines
            if (line[0] == '#' || line[0] == '\0') continue;
            
            // Handle include directives
            if (strncmp(line, "include ", 8) == 0) {
                // For simplicity, we'll skip nested includes
                continue;
            }
            
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", line, libname);
            
            if (access(full_path, F_OK) == 0) {
                fclose(conf);
                return realpath(full_path, NULL);
            }
        }
        fclose(conf);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int fd = 0;
    Elf *e = 0;
    char *lib = 0, *tmplib = 0;
    GElf_Ehdr ehdr;
    Elf_Scn *scn = 0;
    Elf_Data *data;
    GElf_Shdr shdr;
    GElf_Dyn dyn;
    size_t shstrndx = 0;
    int found_dynamic_section = 0, found_lib = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s filename [-v]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if(argc > 2) {
    	opt_verbose = 1;
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

    if(ehdr.e_type != ET_EXEC && ehdr.e_type != ET_DYN) {
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

		if (!opt_verbose) {                
		        /* Parse out the library's name */
		        tmplib = calloc(strlen(lib),  sizeof(char));
		        sscanf(lib, "lib%[^.|^-]", tmplib);
		        printf("-l%s ", tmplib);
		        free(tmplib);
		        found_lib = 1;
                } else {
                	printf("%s ", find_library_path(lib));
		        found_lib = 1;
                }
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
