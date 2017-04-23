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

#if defined(_WIN32) || defined(_WINNT)
# include "mmap-windows.c"
#else
# include <sys/mman.h>
#endif

#ifdef HAVE_LINK_H
#include <link.h>
#endif

#include "bswap.h"

#if BYTE_ORDER == LITTLE_ENDIAN
# define ELFMAG_U32 ((uint32_t)(ELFMAG0 + 0x100 * (ELFMAG1 + (0x100 * (ELFMAG2 + 0x100 * ELFMAG3)))))
#elif BYTE_ORDER == BIG_ENDIAN
# define ELFMAG_U32 ((uint32_t)((((ELFMAG0 * 0x100) + ELFMAG1) * 0x100 + ELFMAG2) * 0x100 + ELFMAG3))
#else
#error "PDP-endian not supported."
#endif

struct library {
	char *name;
	int resolved;
	char *path;
	struct library *next;
};
static struct library *lib_list = NULL;
static int byteswap;

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

static Elf32_Shdr *elf32_find_section_type(uint32_t key, Elf32_Ehdr *ehdr)
{
	int j;
	Elf32_Shdr *shdr;
	shdr = (Elf32_Shdr *) (ehdr->e_shoff + (char *)ehdr);
	for (j = ehdr->e_shnum; --j >= 0; ++shdr) {
		if (key == byteswap32_to_host(shdr->sh_type)) {
			return shdr;
		}
	}
	return NULL;
}

static Elf32_Phdr *elf32_find_phdr_type(uint32_t type, Elf32_Ehdr *ehdr)
{
	int j;
	Elf32_Phdr *phdr = (Elf32_Phdr *) (ehdr->e_phoff + (char *)ehdr);
	for (j = ehdr->e_phnum; --j >= 0; ++phdr) {
		if (type == byteswap32_to_host(phdr->p_type)) {
			return phdr;
		}
	}
	return NULL;
}

/* Returns value if return_val==1, ptr otherwise */
static void *elf32_find_dynamic(int64_t const key, Elf32_Dyn *dynp,
		       Elf32_Ehdr *ehdr, int return_val)
{
	Elf32_Phdr *pt_text = elf32_find_phdr_type(PT_LOAD, ehdr);
	unsigned tx_reloc = byteswap32_to_host(pt_text->p_vaddr) - byteswap32_to_host(pt_text->p_offset);
	for (; DT_NULL != byteswap32_to_host(dynp->d_tag); ++dynp) {
		if (key == byteswap32_to_host(dynp->d_tag)) {
			if (return_val == 1)
				return (void *)byteswap32_to_host(dynp->d_un.d_val);
			else
				return (void *)(byteswap32_to_host(dynp->d_un.d_val) - tx_reloc + (char *)ehdr);
		}
	}
	return NULL;
}

static int check_elf32_header(Elf32_Ehdr *const ehdr)
{
	if (!ehdr || *(uint32_t*)ehdr != ELFMAG_U32
	 || ehdr->e_ident[EI_CLASS] != ELFCLASS32
	 || ehdr->e_ident[EI_VERSION] != EV_CURRENT
	) {
		return 1;
	}

	/* Check if the target endianness matches the host's endianness */
	byteswap = 0;
	if (BYTE_ORDER == LITTLE_ENDIAN) {
		if (ehdr->e_ident[5] == ELFDATA2MSB)
			byteswap = 1;
	} else if (BYTE_ORDER == BIG_ENDIAN) {
		if (ehdr->e_ident[5] == ELFDATA2LSB)
			byteswap = 1;
	}

	/* Be very lazy, and only byteswap the stuff we use */
	if (byteswap) {
		ehdr->e_type = bswap_16(ehdr->e_type);
		ehdr->e_phoff = byteswap32_to_host(ehdr->e_phoff);
		ehdr->e_shoff = byteswap32_to_host(ehdr->e_shoff);
		ehdr->e_phnum = bswap_16(ehdr->e_phnum);
		ehdr->e_shnum = bswap_16(ehdr->e_shnum);
	}

	return 0;
}


static int add_library(Elf32_Ehdr *ehdr, Elf32_Dyn *dynamic, int is_setuid, char *s)
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

	/*printf("add_library is adding '%s' to '%s'\n", newlib->name, newlib->path); */
	if (!lib_list) {
		lib_list = newlib;
	} else {
		for (cur = lib_list; cur->next; cur = cur->next) ;	/* nothing */
		cur->next = newlib;
	}
	return 0;
}

static void find_needed_libraries32(Elf32_Ehdr *ehdr, Elf32_Dyn *dynamic, int is_setuid)
{
	Elf32_Dyn *dyns;

	for (dyns = dynamic; byteswap32_to_host(dyns->d_tag) != DT_NULL; ++dyns) {
		if (DT_NEEDED == byteswap32_to_host(dyns->d_tag)) {
			char *strtab;
			strtab = (char *)elf32_find_dynamic(DT_STRTAB, dynamic, ehdr, 0);
			add_library(ehdr, dynamic, is_setuid, (char *)strtab + byteswap32_to_host(dyns->d_un.d_val));
		}
	}
}

/* map the .so, and locate interesting pieces */
/*
#warning "There may be two warnings here about vfork() clobbering, ignore them"
*/
static int find_dependencies32(char *filename)
{
	int is_suid = 0;
	FILE *thefile;
	struct stat statbuf;
	Elf32_Ehdr *ehdr32 = NULL;
	Elf32_Shdr *dynsec32 = NULL;
	Elf32_Dyn *dynamic32 = NULL;
	
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

	if ((size_t) statbuf.st_size < sizeof(Elf32_Ehdr))
		goto foo;

	if (!S_ISREG(statbuf.st_mode))
		goto foo;

	/* mmap the file to make reading stuff from it effortless */
	ehdr32 = mmap(0, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(thefile), 0);
	if (ehdr32 == MAP_FAILED) {
		fclose(thefile);
		fprintf(stderr, "mmap(%s) failed: %s\n", filename, strerror(errno));
		return -1;
	}

foo:
	fclose(thefile);

	/* Check if this looks like a legit ELF file */
	if (check_elf32_header(ehdr32)) {
		/*fprintf(stderr, "%s: not an ELF file.\n", filename);*/
		return -1;
	}
	/* Check if this is the right kind of ELF file */
	if (ehdr32->e_type != ET_EXEC && ehdr32->e_type != ET_DYN) {
		fprintf(stderr, "%s: not a dynamic executable\n", filename);
		return -1;
	}
	if (ehdr32->e_type == ET_EXEC || ehdr32->e_type == ET_DYN) {
		if (statbuf.st_mode & S_ISUID)
			is_suid = 1;
		if ((statbuf.st_mode & (S_ISGID | S_IXGRP)) == (S_ISGID | S_IXGRP))
			is_suid = 1;
		/* FIXME */
		if (is_suid)
			fprintf(stderr, "%s: is setuid\n", filename);
	}

	dynsec32 = elf32_find_section_type(SHT_DYNAMIC, ehdr32);
	if (dynsec32) {
		dynamic32 = (Elf32_Dyn *) (byteswap32_to_host(dynsec32->sh_offset) + (char *)ehdr32);
		find_needed_libraries32(ehdr32, dynamic32, is_suid);
	}

	return 0;
}


static Elf64_Shdr *elf64_find_section_type(uint32_t key, Elf64_Ehdr *ehdr)
{
	int j;
	Elf64_Shdr *shdr;
	shdr = (Elf64_Shdr *) (ehdr->e_shoff + (char *)ehdr);
	for (j = ehdr->e_shnum; --j >= 0; ++shdr) {
		if (key == byteswap32_to_host(shdr->sh_type)) {
			return shdr;
		}
	}
	return NULL;
}

static Elf64_Phdr *elf64_find_phdr_type(uint32_t type, Elf64_Ehdr *ehdr)
{
	int j;
	Elf64_Phdr *phdr = (Elf64_Phdr *) (ehdr->e_phoff + (char *)ehdr);
	for (j = ehdr->e_phnum; --j >= 0; ++phdr) {
		if (type == byteswap32_to_host(phdr->p_type)) {
			return phdr;
		}
	}
	return NULL;
}

/* Returns value if return_val==1, ptr otherwise */
static void *elf64_find_dynamic(int64_t const key, Elf64_Dyn *dynp,
		       Elf64_Ehdr *ehdr, int return_val)
{
	Elf64_Phdr *pt_text = elf64_find_phdr_type(PT_LOAD, ehdr);
	unsigned tx_reloc = byteswap64_to_host(pt_text->p_vaddr) - byteswap64_to_host(pt_text->p_offset);
	for (; DT_NULL != byteswap64_to_host(dynp->d_tag); ++dynp) {
		if (key == byteswap64_to_host(dynp->d_tag)) {
			if (return_val == 1)
				return (void *)byteswap64_to_host(dynp->d_un.d_val);
			else
				return (void *)(byteswap64_to_host(dynp->d_un.d_val) - tx_reloc + (char *)ehdr);
		}
	}
	return NULL;
}

static int check_elf64_header(Elf64_Ehdr *const ehdr)
{
	if (!ehdr || *(uint32_t*)ehdr != ELFMAG_U32
	 || ehdr->e_ident[EI_CLASS] != ELFCLASS64
	 || ehdr->e_ident[EI_VERSION] != EV_CURRENT
	) {
		return 1;
	}

	/* Check if the target endianness matches the host's endianness */
	byteswap = 0;
	if (BYTE_ORDER == LITTLE_ENDIAN) {
		if (ehdr->e_ident[5] == ELFDATA2MSB)
			byteswap = 1;
	} else if (BYTE_ORDER == BIG_ENDIAN) {
		if (ehdr->e_ident[5] == ELFDATA2LSB)
			byteswap = 1;
	}

	/* Be very lazy, and only byteswap the stuff we use */
	if (byteswap) {
		ehdr->e_type = bswap_16(ehdr->e_type);
		ehdr->e_phoff = byteswap64_to_host(ehdr->e_phoff);
		ehdr->e_shoff = byteswap64_to_host(ehdr->e_shoff);
		ehdr->e_phnum = bswap_16(ehdr->e_phnum);
		ehdr->e_shnum = bswap_16(ehdr->e_shnum);
	}

	return 0;
}


static int add_library64(Elf64_Ehdr *ehdr, Elf64_Dyn *dynamic, int is_setuid, char *s)
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

	/*printf("add_library is adding '%s' to '%s'\n", newlib->name, newlib->path); */
	if (!lib_list) {
		lib_list = newlib;
	} else {
		for (cur = lib_list; cur->next; cur = cur->next) ;	/* nothing */
		cur->next = newlib;
	}
	return 0;
}

static void find_needed_libraries64(Elf64_Ehdr *ehdr, Elf64_Dyn *dynamic, int is_setuid)
{
	Elf64_Dyn *dyns;

	for (dyns = dynamic; byteswap64_to_host(dyns->d_tag) != DT_NULL; ++dyns) {
		if (DT_NEEDED == byteswap64_to_host(dyns->d_tag)) {
			char *strtab;
			strtab = (char *)elf64_find_dynamic(DT_STRTAB, dynamic, ehdr, 0);
			add_library64(ehdr, dynamic, is_setuid, (char *)strtab + byteswap64_to_host(dyns->d_un.d_val));
		}
	}
}

/* map the .so, and locate interesting pieces */
/*
#warning "There may be two warnings here about vfork() clobbering, ignore them"
*/
static int find_dependencies64(char *filename)
{
	int is_suid = 0;
	FILE *thefile;
	struct stat statbuf;
	Elf64_Ehdr *ehdr64 = NULL;
	Elf64_Shdr *dynsec64 = NULL;
	Elf64_Dyn *dynamic64 = NULL;

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

	if ((size_t) statbuf.st_size < sizeof(Elf64_Ehdr))
		goto foo;

	if (!S_ISREG(statbuf.st_mode))
		goto foo;

	/* mmap the file to make reading stuff from it effortless */
	ehdr64 = mmap(0, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(thefile), 0);
	if (ehdr64 == MAP_FAILED) {
		fclose(thefile);
		fprintf(stderr, "mmap(%s) failed: %s\n", filename, strerror(errno));
		return -1;
	}

foo:
	fclose(thefile);

	/* Check if this looks like a legit ELF file */
	if (check_elf64_header(ehdr64)) {
		/*fprintf(stderr, "%s: not an ELF file.\n", filename);*/
		return -1;
	}
	/* Check if this is the right kind of ELF file */
	if (ehdr64->e_type != ET_EXEC && ehdr64->e_type != ET_DYN) {
		fprintf(stderr, "%s: not a dynamic executable\n", filename);
		return -1;
	}
	if (ehdr64->e_type == ET_EXEC || ehdr64->e_type == ET_DYN) {
		if (statbuf.st_mode & S_ISUID)
			is_suid = 1;
		if ((statbuf.st_mode & (S_ISGID | S_IXGRP)) == (S_ISGID | S_IXGRP))
			is_suid = 1;
		/* FIXME */
		if (is_suid)
			fprintf(stderr, "%s: is setuid\n", filename);
	}

	dynsec64 = elf64_find_section_type(SHT_DYNAMIC, ehdr64);
	if (dynsec64) {
		dynamic64 = (Elf64_Dyn *) (byteswap64_to_host(dynsec64->sh_offset) + (char *) ehdr64);
		find_needed_libraries64(ehdr64, dynamic64, is_suid);
	}

	return 0;
}

int main(int argc, char **argv)
{
	int multi = 0;
	int got_em_all = 1;
	char *filename = NULL;
	char *tmp = NULL;
	struct library *cur;

	if (argc < 2) {
		fprintf(stderr, "%s: missing file arguments\nTry `%s --help' for more information.\n", argv[0], argv[0]);
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
			fprintf(stderr, "Usage: %s [OPTION]... FILE...\n\t--help\t\tprint this help and exit\n", argv[0]);
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

		if ((find_dependencies32(filename) != 0) && find_dependencies64(filename))
			continue;
			
		while (got_em_all) {
			got_em_all = 0;
			/* Keep walking the list till everybody is resolved */
			for (cur = lib_list; cur; cur = cur->next) {
				if (cur->resolved == 0 && cur->path) {
					got_em_all = 1;
					/*printf("checking sub-depends for '%s'\n", cur->path);
					find_dependencies32(cur->path);
					find_dependencies64(cur->path);
					*/
					cur->resolved = 1;
				}
			}
		}

		/* Print the list */
		got_em_all = 0;
		for (cur = lib_list; cur; cur = cur->next) {
			got_em_all = 1;
			tmp = calloc(strlen(cur->name),  sizeof(char));
			sscanf(cur->name, "lib%[^.]", tmp);
			printf("-l%s ", tmp);
			free(tmp);
		}
		if(got_em_all){
			printf("\n");
		}
		for (cur = lib_list; cur; cur = cur->next) {
			free(cur->name);
			cur->name = NULL;
			if (cur->path) {
				free(cur->path);
				cur->path = NULL;
			}
		}
		lib_list = NULL;
	}

	return 0;
}
