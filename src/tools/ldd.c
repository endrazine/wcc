/* vi: set sw=4 ts=4: */
/*
 * A small little ldd implementation for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Several functions in this file (specifically, elf_find_section_type(),
 * elf_find_phdr_type(), and elf_find_dynamic(), were stolen from elflib.c from
 * elfvector (http://www.BitWagon.com/elfvector.html) by John F. Reiser
 * <jreiser@BitWagon.com>, which is copyright 2000 BitWagon Software LLC
 * (GPL2).
 *
 * Licensed under GPLv2 or later
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

#if defined(_WIN32) || defined(_WINNT)
# include "mmap-windows.c"
#else
# include <sys/mman.h>
#endif

#ifdef HAVE_LINK_H
#include <link.h>
#endif
/* makefile will include elf.h for us */

#include "bswap.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/* For SunOS */
#ifndef PATH_MAX
#define PATH_MAX _POSIX_PATH_MAX
#endif

#undef UCLIBC_ENDIAN_HOST
#define UCLIBC_ENDIAN_LITTLE 1234
#define UCLIBC_ENDIAN_BIG    4321
#if defined(BYTE_ORDER)
# if BYTE_ORDER == LITTLE_ENDIAN
#  define UCLIBC_ENDIAN_HOST UCLIBC_ENDIAN_LITTLE
# elif BYTE_ORDER == BIG_ENDIAN
#  define UCLIBC_ENDIAN_HOST UCLIBC_ENDIAN_BIG
# endif
#elif defined(__BYTE_ORDER)
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define UCLIBC_ENDIAN_HOST UCLIBC_ENDIAN_LITTLE
# elif __BYTE_ORDER == __BIG_ENDIAN
#  define UCLIBC_ENDIAN_HOST UCLIBC_ENDIAN_BIG
# endif
#endif
#if !defined(UCLIBC_ENDIAN_HOST)
# error "Unknown host byte order!"
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define ELFMAG_U32 ((uint32_t)(ELFMAG0 + 0x100 * (ELFMAG1 + (0x100 * (ELFMAG2 + 0x100 * ELFMAG3)))))
#elif __BYTE_ORDER == __BIG_ENDIAN
# define ELFMAG_U32 ((uint32_t)((((ELFMAG0 * 0x100) + ELFMAG1) * 0x100 + ELFMAG2) * 0x100 + ELFMAG3))
#endif

#if defined(__alpha__)
#define MATCH_MACHINE(x) (x == EM_ALPHA)
#define ELFCLASSM	ELFCLASS64
#endif

#if defined(__arm__) || defined(__thumb__)
#define MATCH_MACHINE(x) (x == EM_ARM)
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__avr32__)
#define MATCH_MACHINE(x) (x == EM_AVR32)
#define ELFCLASSM      ELFCLASS32
#endif

#if defined(__s390__)
#define MATCH_MACHINE(x) (x == EM_S390)
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__hppa__)
#define MATCH_MACHINE(x) (x == EM_PARISC)
#if defined(__LP64__)
#define ELFCLASSM		ELFCLASS64
#else
#define ELFCLASSM		ELFCLASS32
#endif
#endif

#if defined(__i386__)
#ifndef EM_486
#define MATCH_MACHINE(x) (x == EM_386)
#else
#define MATCH_MACHINE(x) (x == EM_386 || x == EM_486)
#endif
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__ia64__)
#define MATCH_MACHINE(x) (x == EM_IA_64)
#define ELFCLASSM	ELFCLASS64
#endif

#if defined(__mc68000__)
#define MATCH_MACHINE(x) (x == EM_68K)
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__mips__)
#define MATCH_MACHINE(x) (x == EM_MIPS || x == EM_MIPS_RS3_LE)
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__powerpc64__)
#define MATCH_MACHINE(x) (x == EM_PPC64)
#define ELFCLASSM	ELFCLASS64
#elif defined(__powerpc__)
#define MATCH_MACHINE(x) (x == EM_PPC)
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__sh__)
#define MATCH_MACHINE(x) (x == EM_SH)
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__v850e__)
#define MATCH_MACHINE(x) ((x) == EM_V850 || (x) == EM_CYGNUS_V850)
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__sparc__)
#define MATCH_MACHINE(x) ((x) == EM_SPARC || (x) == EM_SPARC32PLUS)
#define ELFCLASSM    ELFCLASS32
#endif

#if defined(__cris__)
#define MATCH_MACHINE(x) (x == EM_CRIS)
#define ELFCLASSM	ELFCLASS32
#endif

#if defined(__x86_64__)
#define MATCH_MACHINE(x) (x == EM_X86_64)
#define ELFCLASSM	ELFCLASS64
#endif

#if defined(__microblaze__)
#define MATCH_MACHINE(x) (x == EM_MICROBLAZE_OLD)
#define ELFCLASSM	ELFCLASS32
#endif

#ifndef MATCH_MACHINE
# ifdef __linux__
#  include <asm/elf.h>
# endif
# ifdef ELF_ARCH
#  define MATCH_MACHINE(x) (x == ELF_ARCH)
# endif
# ifdef ELF_CLASS
#  define ELFCLASSM ELF_CLASS
# endif
#endif
#ifndef MATCH_MACHINE
# warning "You really should add a MATCH_MACHINE() macro for your architecture"
#endif

#if UCLIBC_ENDIAN_HOST == UCLIBC_ENDIAN_LITTLE
#define ELFDATAM	ELFDATA2LSB
#elif UCLIBC_ENDIAN_HOST == UCLIBC_ENDIAN_BIG
#define ELFDATAM	ELFDATA2MSB
#endif

#define ARRAY_SIZE(v)	(sizeof(v) / sizeof(*v))
#define TRUSTED_LDSO	"/lib/" UCLIBC_LDSO

struct library {
	char *name;
	int resolved;
	char *path;
	struct library *next;
};
static struct library *lib_list = NULL;
static char not_found[] = "not found";
static char *interp_name = NULL;
static char *interp_dir = NULL;
static int byteswap;
static int interpreter_already_found = 0;

static __inline__ uint32_t byteswap32_to_host(uint32_t value)
{
	if (byteswap == 1) {
		return (bswap_32(value));
	} else {
		return (value);
	}
}
static __inline__ uint64_t byteswap64_to_host(uint64_t value)
{
	if (byteswap == 1) {
		return (bswap_64(value));
	} else {
		return (value);
	}
}

#if ELFCLASSM == ELFCLASS32
# define byteswap_to_host(x) byteswap32_to_host(x)
#else
# define byteswap_to_host(x) byteswap64_to_host(x)
#endif

static ElfW(Shdr) *elf_find_section_type(uint32_t key, ElfW(Ehdr) *ehdr)
{
	int j;
	ElfW(Shdr) *shdr;
	shdr = (ElfW(Shdr) *) (ehdr->e_shoff + (char *)ehdr);
	for (j = ehdr->e_shnum; --j >= 0; ++shdr) {
		if (key == byteswap32_to_host(shdr->sh_type)) {
			return shdr;
		}
	}
	return NULL;
}

static ElfW(Phdr) *elf_find_phdr_type(uint32_t type, ElfW(Ehdr) *ehdr)
{
	int j;
	ElfW(Phdr) *phdr = (ElfW(Phdr) *) (ehdr->e_phoff + (char *)ehdr);
	for (j = ehdr->e_phnum; --j >= 0; ++phdr) {
		if (type == byteswap32_to_host(phdr->p_type)) {
			return phdr;
		}
	}
	return NULL;
}

/* Returns value if return_val==1, ptr otherwise */
static void *elf_find_dynamic(int64_t const key, ElfW(Dyn) *dynp,
		       ElfW(Ehdr) *ehdr, int return_val)
{
	ElfW(Phdr) *pt_text = elf_find_phdr_type(PT_LOAD, ehdr);
	unsigned tx_reloc = byteswap_to_host(pt_text->p_vaddr) - byteswap_to_host(pt_text->p_offset);
	for (; DT_NULL != byteswap_to_host(dynp->d_tag); ++dynp) {
		if (key == byteswap_to_host(dynp->d_tag)) {
			if (return_val == 1)
				return (void *)byteswap_to_host(dynp->d_un.d_val);
			else
				return (void *)(byteswap_to_host(dynp->d_un.d_val) - tx_reloc + (char *)ehdr);
		}
	}
	return NULL;
}

static char *elf_find_rpath(ElfW(Ehdr) *ehdr, ElfW(Dyn) *dynamic)
{
	ElfW(Dyn) *dyns;

	for (dyns = dynamic; byteswap_to_host(dyns->d_tag) != DT_NULL; ++dyns) {
		if (DT_RPATH == byteswap_to_host(dyns->d_tag)) {
			char *strtab;
			strtab = (char *)elf_find_dynamic(DT_STRTAB, dynamic, ehdr, 0);
			return ((char *)strtab + byteswap_to_host(dyns->d_un.d_val));
		}
	}
	return NULL;
}

static int check_elf_header(ElfW(Ehdr) *const ehdr)
{
	if (!ehdr || *(uint32_t*)ehdr != ELFMAG_U32
	 || ehdr->e_ident[EI_CLASS] != ELFCLASSM
	 || ehdr->e_ident[EI_VERSION] != EV_CURRENT
	) {
		return 1;
	}

	/* Check if the target endianness matches the host's endianness */
	byteswap = 0;
	if (UCLIBC_ENDIAN_HOST == UCLIBC_ENDIAN_LITTLE) {
		if (ehdr->e_ident[5] == ELFDATA2MSB)
			byteswap = 1;
	} else if (UCLIBC_ENDIAN_HOST == UCLIBC_ENDIAN_BIG) {
		if (ehdr->e_ident[5] == ELFDATA2LSB)
			byteswap = 1;
	}

	/* Be very lazy, and only byteswap the stuff we use */
	if (byteswap) {
		ehdr->e_type = bswap_16(ehdr->e_type);
		ehdr->e_phoff = byteswap_to_host(ehdr->e_phoff);
		ehdr->e_shoff = byteswap_to_host(ehdr->e_shoff);
		ehdr->e_phnum = bswap_16(ehdr->e_phnum);
		ehdr->e_shnum = bswap_16(ehdr->e_shnum);
	}

	return 0;
}

static __inline__ void map_cache(void)
{
}
static __inline__ void unmap_cache(void)
{
}

/* This function's behavior must exactly match that
 * in uClibc/ldso/ldso/dl-elf.c */
static void search_for_named_library(char *name, char *result,
				     const char *path_list)
{
	int i, count = 1;
	char *path, *path_n;
	struct stat filestat;

	/* We need a writable copy of this string */
	path = strdup(path_list);
	if (!path) {
		fprintf(stderr, "%s: Out of memory!\n", __func__);
		exit(EXIT_FAILURE);
	}
	/* Eliminate all double //s */
	path_n = path;
	while ((path_n = strstr(path_n, "//"))) {
		i = strlen(path_n);
		memmove(path_n, path_n + 1, i - 1);
		*(path_n + i - 1) = '\0';
	}

	/* Replace colons with zeros in path_list and count them */
	for (i = strlen(path); i > 0; i--) {
		if (path[i] == ':') {
			path[i] = 0;
			count++;
		}
	}
	path_n = path;
	for (i = 0; i < count; i++) {
		strcpy(result, path_n);
		strcat(result, "/");
		strcat(result, name);
		if (stat(result, &filestat) == 0 && filestat.st_mode & S_IRUSR) {
			free(path);
			return;
		}
		path_n += (strlen(path_n) + 1);
	}
	free(path);
	*result = '\0';
}

static void locate_library_file(ElfW(Ehdr) *ehdr, ElfW(Dyn) *dynamic,
                                int is_suid, struct library *lib)
{
	char *buf;
	char *path;
	struct stat filestat;

	/* If this is a fully resolved name, our job is easy */
	if (stat(lib->name, &filestat) == 0) {
		lib->path = strdup(lib->name);
		return;
	}

	/* We need some elbow room here.  Make some room... */
	buf = malloc(1024);
	if (!buf) {
		fprintf(stderr, "%s: Out of memory!\n", __func__);
		exit(EXIT_FAILURE);
	}

	/* This function must match the behavior of _dl_load_shared_library
	 * in readelflib1.c or things won't work out as expected... */

	/* The ABI specifies that RPATH is searched first, so do that now.  */
	path = elf_find_rpath(ehdr, dynamic);
	if (path) {
		search_for_named_library(lib->name, buf, path);
		if (*buf != '\0') {
			lib->path = buf;
			return;
		}
	}

	/* Next check LD_{ELF_}LIBRARY_PATH if specified and allowed.
	 * Since this app doesn't actually run an executable I will skip
	 * the suid check, and just use LD_{ELF_}LIBRARY_PATH if set */
	if (is_suid == 1)
		path = NULL;
	else
		path = getenv("LD_LIBRARY_PATH");
	if (path) {
		search_for_named_library(lib->name, buf, path);
		if (*buf != '\0') {
			lib->path = buf;
			return;
		}
	}

	/* Next look for libraries wherever the shared library
	 * loader was installed -- this is usually where we
	 * should find things... */
	if (interp_dir) {
		search_for_named_library(lib->name, buf, interp_dir);
		if (*buf != '\0') {
			lib->path = buf;
			return;
		}
	}

	/* Lastly, search the standard list of paths for the library.
	   This list must exactly match the list in uClibc/ldso/ldso/dl-elf.c */
	path = "/lib:/usr/lib:/usr/X11R6/lib"
	    ;
	search_for_named_library(lib->name, buf, path);
	if (*buf != '\0') {
		lib->path = buf;
	} else {
		free(buf);
		lib->path = not_found;
	}
}

static int add_library(ElfW(Ehdr) *ehdr, ElfW(Dyn) *dynamic, int is_setuid, char *s)
{
	char *tmp, *tmp1, *tmp2;
	struct library *cur, *newlib = lib_list;

	if (!s || !strlen(s))
		return 1;

	tmp = s;
	while (*tmp) {
		if (*tmp == '/')
			s = tmp + 1;
		tmp++;
	}

	/* We add ldso elsewhere */
	if (interpreter_already_found && (tmp = strrchr(interp_name, '/')) != NULL) {
		int len = strlen(interp_dir);
		if (strcmp(s, interp_name + 1 + len) == 0)
			return 1;
	}

	for (cur = lib_list; cur; cur = cur->next) {
		/* Check if this library is already in the list */
		tmp1 = tmp2 = cur->name;
		while (*tmp1) {
			if (*tmp1 == '/')
				tmp2 = tmp1 + 1;
			tmp1++;
		}
		if (strcmp(tmp2, s) == 0) {
			/*printf("find_elf_interpreter is skipping '%s' (already in list)\n", cur->name); */
			return 0;
		}
	}

	/* Ok, this lib needs to be added to the list */
	newlib = malloc(sizeof(struct library));
	if (!newlib)
		return 1;
	newlib->name = malloc(strlen(s) + 1);
	strcpy(newlib->name, s);
	newlib->resolved = 0;
	newlib->path = NULL;
	newlib->next = NULL;

	/* Now try and locate where this library might be living... */
	locate_library_file(ehdr, dynamic, is_setuid, newlib);

	/*printf("add_library is adding '%s' to '%s'\n", newlib->name, newlib->path); */
	if (!lib_list) {
		lib_list = newlib;
	} else {
		for (cur = lib_list; cur->next; cur = cur->next) ;	/* nothing */
		cur->next = newlib;
	}
	return 0;
}

static void find_needed_libraries(ElfW(Ehdr) *ehdr, ElfW(Dyn) *dynamic, int is_setuid)
{
	ElfW(Dyn) *dyns;

	for (dyns = dynamic; byteswap_to_host(dyns->d_tag) != DT_NULL; ++dyns) {
		if (DT_NEEDED == byteswap_to_host(dyns->d_tag)) {
			char *strtab;
			strtab = (char *)elf_find_dynamic(DT_STRTAB, dynamic, ehdr, 0);
			add_library(ehdr, dynamic, is_setuid, (char *)strtab + byteswap_to_host(dyns->d_un.d_val));
		}
	}
}

static struct library *find_elf_interpreter(ElfW(Ehdr) *ehdr)
{
	ElfW(Phdr) *phdr;

	if (interpreter_already_found == 1)
		return NULL;
	phdr = elf_find_phdr_type(PT_INTERP, ehdr);
	if (phdr) {
		struct library *cur, *newlib = NULL;
		char *s = (char *)ehdr + byteswap_to_host(phdr->p_offset);

		char *tmp, *tmp1;
		interp_name = strdup(s);
		interp_dir = strdup(s);
		tmp = strrchr(interp_dir, '/');
		if (tmp)
			*tmp = '\0';
		else {
			free(interp_dir);
			interp_dir = interp_name;
		}
		tmp1 = tmp = s;
		while (*tmp) {
			if (*tmp == '/')
				tmp1 = tmp + 1;
			tmp++;
		}
		for (cur = lib_list; cur; cur = cur->next) {
			/* Check if this library is already in the list */
			if (strcmp(cur->name, tmp1) == 0) {
				/*printf("find_elf_interpreter is replacing '%s' (already in list)\n", cur->name); */
				newlib = cur;
				free(newlib->name);
				if (newlib->path != not_found) {
					free(newlib->path);
				}
				newlib->name = NULL;
				newlib->path = NULL;
				break;
			}
		}
		if (newlib == NULL)
			newlib = malloc(sizeof(struct library));
		if (!newlib)
			return NULL;
		newlib->name = malloc(strlen(s) + 1);
		strcpy(newlib->name, s);
		newlib->path = strdup(newlib->name);
		newlib->resolved = 1;
		newlib->next = NULL;
		interpreter_already_found = 1;
		return newlib;
	}
	return NULL;
}

/* map the .so, and locate interesting pieces */
/*
#warning "There may be two warnings here about vfork() clobbering, ignore them"
*/
static int find_dependencies(char *filename)
{
	int is_suid = 0;
	FILE *thefile;
	struct stat statbuf;
	ElfW(Ehdr) *ehdr = NULL;
	ElfW(Shdr) *dynsec = NULL;
	ElfW(Dyn) *dynamic = NULL;
	struct library *interp;

	if (filename == not_found)
		return 0;

	if (!filename) {
		fprintf(stderr, "No filename specified.\n");
		return -1;
	}
	if (!(thefile = fopen(filename, "r"))) {
		perror(filename);
		return -1;
	}
	if (fstat(fileno(thefile), &statbuf) < 0) {
		perror(filename);
		fclose(thefile);
		return -1;
	}

	if ((size_t) statbuf.st_size < sizeof(ElfW(Ehdr)))
		goto foo;

	if (!S_ISREG(statbuf.st_mode))
		goto foo;

	/* mmap the file to make reading stuff from it effortless */
	ehdr = mmap(0, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(thefile), 0);
	if (ehdr == MAP_FAILED) {
		fclose(thefile);
		fprintf(stderr, "mmap(%s) failed: %s\n", filename, strerror(errno));
		return -1;
	}

foo:
	fclose(thefile);

	/* Check if this looks like a legit ELF file */
	if (check_elf_header(ehdr)) {
		fprintf(stderr, "%s: not an ELF file.\n", filename);
		return -1;
	}
	/* Check if this is the right kind of ELF file */
	if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
		fprintf(stderr, "%s: not a dynamic executable\n", filename);
		return -1;
	}
	if (ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN) {
		if (statbuf.st_mode & S_ISUID)
			is_suid = 1;
		if ((statbuf.st_mode & (S_ISGID | S_IXGRP)) == (S_ISGID | S_IXGRP))
			is_suid = 1;
		/* FIXME */
		if (is_suid)
			fprintf(stderr, "%s: is setuid\n", filename);
	}

	interpreter_already_found = 0;
	interp = find_elf_interpreter(ehdr);

	if (interp
	    && (ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN)
	    && ehdr->e_ident[EI_CLASS] == ELFCLASSM
	    && ehdr->e_ident[EI_DATA] == ELFDATAM
	    && ehdr->e_ident[EI_VERSION] == EV_CURRENT
	    && MATCH_MACHINE(ehdr->e_machine))
	{
		struct stat st;
		if (stat(interp->path, &st) == 0 && S_ISREG(st.st_mode)) {
			pid_t pid;
			int status;
			static const char *const environment[] = {
				"PATH=/usr/bin:/bin:/usr/sbin:/sbin",
				"SHELL=/bin/sh",
				"LD_TRACE_LOADED_OBJECTS=1",
				NULL
			};

			if ((pid = vfork()) == 0) {
				/* Cool, it looks like we should be able to actually
				 * run this puppy.  Do so now... */
				execle(filename, filename, NULL, environment);
				_exit(0xdead);
			}

			/* Wait till it returns */
			waitpid(pid, &status, 0);

			if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
				return 1;
			}

			/* If the exec failed, we fall through to trying to find
			 * all the needed libraries ourselves by rummaging about
			 * in the ELF headers... */
		}
	}

	dynsec = elf_find_section_type(SHT_DYNAMIC, ehdr);
	if (dynsec) {
		dynamic = (ElfW(Dyn) *) (byteswap_to_host(dynsec->sh_offset) + (char *)ehdr);
		find_needed_libraries(ehdr, dynamic, is_suid);
	}

	return 0;
}

int main(int argc, char **argv)
{
	int multi = 0;
	int got_em_all = 1;
	char *filename = NULL;
	struct library *cur;

	if (argc < 2) {
		fprintf(stderr, "ldd: missing file arguments\n"
				"Try `ldd --help' for more information.\n");
		exit(EXIT_FAILURE);
	}
	if (argc > 2)
		multi++;

	while (--argc > 0) {
		++argv;

		if (strcmp(*argv, "--") == 0) {
			/* Ignore "--" */
			continue;
		}

		if (strcmp(*argv, "--help") == 0 || strcmp(*argv, "-h") == 0) {
			fprintf(stderr, "Usage: ldd [OPTION]... FILE...\n"
					"\t--help\t\tprint this help and exit\n");
			exit(EXIT_SUCCESS);
		}

		filename = *argv;
		if (!filename) {
			fprintf(stderr, "No filename specified.\n");
			exit(EXIT_FAILURE);
		}

		if (multi) {
			printf("%s:\n", *argv);
		}

		map_cache();

		if (find_dependencies(filename) != 0)
			continue;

		while (got_em_all) {
			got_em_all = 0;
			/* Keep walking the list till everybody is resolved */
			for (cur = lib_list; cur; cur = cur->next) {
				if (cur->resolved == 0 && cur->path) {
					got_em_all = 1;
					printf("checking sub-depends for '%s'\n", cur->path);
					find_dependencies(cur->path);
					cur->resolved = 1;
				}
			}
		}

		unmap_cache();

		/* Print the list */
		got_em_all = 0;
		for (cur = lib_list; cur; cur = cur->next) {
			got_em_all = 1;
			printf("\t%s => %s (0x00000000)\n", cur->name, cur->path);
		}
		if (interp_name && interpreter_already_found == 1)
			printf("\t%s => %s (0x00000000)\n", interp_name, interp_name);
		else
			printf("\tnot a dynamic executable\n");

		for (cur = lib_list; cur; cur = cur->next) {
			free(cur->name);
			cur->name = NULL;
			if (cur->path && cur->path != not_found) {
				free(cur->path);
				cur->path = NULL;
			}
		}
		lib_list = NULL;
	}

	return 0;
}
