/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2025 Jonathan Brossard
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <libelf.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <uthash.h>
#include <utlist.h>

#include <libwitch/wsh.h>

#include "loadblacklist.h"

#define ELF_ST_BIND ELF64_ST_BIND
#define ELF_ST_TYPE ELF64_ST_TYPE

#define MIN_MAP_SIZE 0xa00000

#ifdef __amd64__


unsigned long int load_address = NULL;

void *start_addr = 0;

/**
* Forward prototypes declaration
*/
int userland_load_binary(char *fname);

/**
* Imported functions prototypes
*/
char *symbol_tobind(int n);
char *symbol_totype(int n);
int custom_add_symbol(char *symbol, char *libname, char *htype, char *hbind, unsigned long value, unsigned int size, unsigned long int addr);

/**
* Main wsh context
*/
extern wsh_t *wsh;

/**
* Add a symbol to linked list
*/
int custom_add_symbol(char *symbol, char *libname, char *htype, char *hbind, unsigned long value, unsigned int size, unsigned long int addr)
{
	symbols_t *s = 0;
	symbols_t *si = 0, *stmp = 0, *res = 0;

	s = calloc(1, sizeof(symbols_t));
	if(!s){ fprintf(stderr, " !! Error: calloc() = %s\n", strerror(errno)); return -1; }
	s->addr = addr;
	s->symbol = strdup(symbol);
	s->size = size;
	s->value = value;
	s->libname = strdup(libname);
	s->htype = strdup(htype);
	s->hbind = strdup(hbind);

	// search this element in linked list
	DL_FOREACH_SAFE(wsh->symbols, si, stmp) {
		// same symbol name
		if((!strncmp(si->symbol,s->symbol,strlen(si->symbol)))&&(strlen(si->symbol) == strlen(s->symbol))){
			res = si;
		}
	}
	if(res){ return 1; } // already in linked list

	DL_APPEND(wsh->symbols, s);
	return 0;
}

/**
* Validate ELF image
*/
int validate_elf(Elf64_Ehdr *hdr)
{
        // Verify ELF signature
        if (strncmp(hdr->e_ident, "\x7f\x45\x4c\x46", 4)) {
                return 1;
        }

	return 0;
}

/**
* Resolve address of a symbol
*/
void *resolve(const char *sym)
{
	static void *handle = NULL;
	if (handle == NULL) {
		handle = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
	}
	return dlsym(handle, sym);
}

/**
* Preform rel relocations
*/
void do_rel(Elf64_Shdr *shdr, const Elf64_Sym *syms, const char *strtab, const char *src, char *dst)
{
	Elf64_Rel *rel = 0;
	unsigned int j = 0;

	rel = (Elf64_Rel *) (src + shdr->sh_offset);

	for (j = 0; j < shdr->sh_size / sizeof(Elf64_Rel); j += 1) {
		const char *sym = strtab + syms[ELF64_R_SYM(rel[j].r_info)].st_name;
		if (wsh->opt_verbose) {
			printf("rel[%d]\n", j);
		}
		switch (ELF64_R_TYPE(rel[j].r_info)) {
		case R_X86_64_JUMP_SLOT:
			if (wsh->opt_verbose) {
				printf("JUMP slot to: %p\n", (void *) resolve(sym));
			}
			*(Elf64_Word *) (dst + rel[j].r_offset) = (Elf64_Word) resolve(sym);
			break;
		case R_X86_64_GLOB_DAT:
			if (wsh->opt_verbose) {
				printf("GLOB DAT to: %p\n", (void *) resolve(sym));
			}
			*(Elf64_Word *) (dst + rel[j].r_offset) = (Elf64_Word) resolve(sym);
			break;
		}

		if (wsh->opt_verbose) {
			printf("\n");
		}
	}
}

/**
* Preform rela relocations
*/
void do_rela(Elf64_Shdr *shdr, const Elf64_Sym *dynsymtab, const char *dynstrtab, const char *src, char *dst, Elf64_Shdr *shstrtab, Elf64_Shdr *shdrs, char *binary)
{
	Elf64_Rela *rela = 0;
	unsigned int j = 0;
	unsigned int link = 0, info = 0;
	Elf64_Sym symbol;
	char *resolved_sym_addr = 0;

	rela = (Elf64_Rela *) (src + shdr->sh_offset);

	if (wsh->opt_verbose) {
		printf(" -- parsing section: %s\n", binary + shstrtab->sh_offset + shdr->sh_name);
	}

	for (j = 0; j < shdr->sh_size / sizeof(Elf64_Rela); j += 1) {

		const char *sym = dynstrtab + dynsymtab[ELF64_R_SYM(rela[j].r_info)].st_name;
		if (wsh->opt_verbose) {
			printf("%s[%d]\n", binary + shstrtab->sh_offset + shdr->sh_name, j);
			printf("symbol: %s\n", sym ? sym : "");

			printf("rela.r_offset: %p\n", (void *) rela[j].r_offset);
			printf("rela.r_info: %p\n", (void *) rela[j].r_info);
			printf("rela.r_addend: %p\n", (void *) rela[j].r_addend);
		}
		link = shdr->sh_link;
		info = shdr->sh_info;
		if (wsh->opt_verbose) {
			printf("link: %d ", link);
			printf("(%s)\n", binary + shstrtab->sh_offset + shdrs[link].sh_name);

			printf("info: %d ", link);
			printf("(%s)\n", binary + shstrtab->sh_offset + shdrs[info].sh_name);

			printf("writing at offset: %p\n", (void *) rela[j].r_offset);
		}
		resolved_sym_addr = resolve(sym);

		switch (ELF64_R_TYPE(rela[j].r_info)) {
		case R_X86_64_JUMP_SLOT:
			if (wsh->opt_verbose) {
				printf("R_X86_64_JUMP_SLOT to: %p (%s)\n", resolved_sym_addr, sym);
			}
			*(unsigned long int *) (dst + rela[j].r_offset) = resolved_sym_addr;
			break;
		case R_X86_64_GLOB_DAT:
			if (wsh->opt_verbose) {
				printf("R_X86_64_GLOB_DAT to: %p (%s)\n", resolved_sym_addr, sym);
			}
			*(unsigned long int *) (dst + rela[j].r_offset) = resolved_sym_addr;
			break;
		case R_X86_64_RELATIVE:
			if (wsh->opt_verbose) {
				printf("R_X86_64_RELATIVE\n");
			}
			*(unsigned long int *) (dst + rela[j].r_offset) = rela[j].r_addend + load_address;
			break;
		case R_X86_64_COPY:
			symbol = dynsymtab[ELF64_R_SYM(rela[j].r_info)];
			if (wsh->opt_verbose) {
				printf("R_X86_64_COPY: %p (%s) size:%ld\n", resolved_sym_addr, sym, symbol.st_size);
			}
			memcpy((dst + rela[j].r_offset), resolved_sym_addr, symbol.st_size);
			break;
		case R_X86_64_64:
			symbol = dynsymtab[ELF64_R_SYM(rela[j].r_info)];
			if (wsh->opt_verbose) {
				printf("R_X86_64_64: %p (%s)\n", resolved_sym_addr, sym);
			}
			*(uint64_t *) (dst + rela[j].r_offset) = (uint64_t) (resolved_sym_addr + rela[j].r_addend);
			break;

		default:
			break;
		}

		if (wsh->opt_verbose) {
			printf("\n");
		}
	}
}

/**
* Search application entry point
*/
void *find_entrypoint(char *dst)
{
	Elf64_Ehdr *hdr = 0;

	hdr = (Elf64_Ehdr *) dst;
	if (wsh->opt_verbose) {
		printf("    * Entry point address: %p\n", (void *) hdr->e_entry);
	}
	return dst + hdr->e_entry;
}

/**
* Find a symbol from .strtab/.symtab
*/
void *find_sym(const char *name, unsigned int maxsyms, Elf64_Sym *symtab, const char *strtab, const char *src, char *dst)
{
	unsigned int i = 0;

	for (i = 0; i < maxsyms; i++) {
		if (wsh->opt_verbose) {
			printf("[%03d] %s %p\n", i, symtab[i].st_name + strtab, (void *) symtab[i].st_value);
		}
		if ((!strncmp(name, symtab[i].st_name + strtab, strlen(name))) && (strlen(symtab[i].st_name + strtab) == strlen(name))) {
			return /*dst +*/ symtab[i].st_value;
		}
	}

	return (void *) -1;
}

/**
* Find a dynamic symbol from .dynsym/.dynstr
*/
void *find_dynsym(const char *name, Elf64_Shdr *shdr, const char *dynstrtab, const char *src, char *dst)
{
	Elf64_Sym *syms = (Elf64_Sym *) (src + shdr->sh_offset);
	unsigned int i = 0;
	for (i = 0; i < shdr->sh_size / sizeof(Elf64_Sym); i += 1) {
		if (strcmp(name, dynstrtab + syms[i].st_name) == 0) {
			return dst + syms[i].st_value;
		}
	}
	return (void *) -1;
}

/**
* Walk symbols from .strtab/.symtab
*/
void *parse_sym(unsigned int maxsyms, Elf64_Sym *symtab, const char *strtab, const char *src, char *dst, char *libname, void *base_addr)
{
	unsigned int i = 0, j = 0;

	char *htype = 0, *hbind = 0;
	unsigned int stype = 0, sbind = 0;
	unsigned long int ret = 0;
	char newname[1024];

	for (i = 0; i < maxsyms; i++) {
	
		stype = ELF_ST_TYPE(symtab[i].st_info);
		htype = symbol_totype(stype);
		sbind = ELF_ST_BIND(symtab[i].st_info);
		hbind = symbol_tobind(sbind);

		if (wsh->opt_verbose) {
			printf("[%03d] %08p %s %s %s\n", i, (void *) symtab[i].st_value, symtab[i].st_name + strtab, htype, hbind);
		}
			custom_add_symbol((char*)(symtab[i].st_name + strtab), libname, htype, hbind, (unsigned long int)symtab[i].st_value, (unsigned int)symtab[i].st_size, (unsigned long int)ret);

		if ((symtab[i].st_value)&&(!strncmp(htype, "Function", 8))/*&&(!strncmp(hbind, "Global", 6))*/) {

			/**
			* If function name is blackslisted, skip...
			*/

			// Lua blacklist
			for(j=0; j < sizeof(myblacklist)/sizeof(char*);j++){
				if((strlen((char*)(symtab[i].st_name + strtab)) == strlen(myblacklist[j]))&&(!strncmp(myblacklist[j] ,(char*)(symtab[i].st_name + strtab), strlen(myblacklist[j])))){
					goto skip_symbol;
				}
			}
			
			/*
			* Make C function available from lua
			*/
			memset(newname, 0x00, 1024);
			snprintf(newname, 1023, "reflect_%s", symtab[i].st_name + strtab);
			lua_pushcfunction(wsh->L, (void *) (symtab[i].st_value + (unsigned long int) base_addr));
			lua_setglobal(wsh->L, newname);

			/**
			* Create a wrapper function with the original name
			*/
			char *luacmd = calloc(1, 2048);
			snprintf(luacmd, 2047, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", symtab[i].st_name + strtab, newname);
			luabuff_append(luacmd);
			exec_luabuff();		
			free(luacmd);
		}
skip_symbol:
		;
	}

	return 0;
}

/**
* Walk dynamic symbols from .dynsym/.dynstr
*/
void *parse_dynsym(Elf64_Shdr *shdr, const char *dynstrtab, const char *src, char *dst, char *libname, void *base_addr)
{
	Elf64_Sym *syms = (Elf64_Sym *) (src + shdr->sh_offset);
	
	char *htype = 0, *hbind = 0;
	unsigned int stype = 0, sbind = 0;
	unsigned long int ret = 0;
	char newname[1024];

	unsigned int i = 0;
	for (i = 0; i < shdr->sh_size / sizeof(Elf64_Sym); i += 1) {
	
		stype = ELF_ST_TYPE(syms[i].st_info);
		htype = symbol_totype(stype);
		sbind = ELF_ST_BIND(syms[i].st_info);
		hbind = symbol_tobind(sbind);

		if (wsh->opt_verbose) {	
			printf("[%04d] %08lx %s %s %s\n", i, syms[i].st_value, dynstrtab + syms[i].st_name, htype, hbind);
		}
		custom_add_symbol((char*)(dynstrtab + syms[i].st_name), libname, htype, hbind, (unsigned long int)syms[i].st_value, (unsigned int)syms[i].st_size, (unsigned long int)ret);
		
		if ((syms[i].st_value)&&(!strncmp(htype, "Function", 8))/*&&(!strncmp(hbind, "Global", 6))*/) {

			/*
			* Make C function available from lua
			*/
			memset(newname, 0x00, 1024);
			snprintf(newname, 1023, "reflect_%s", dynstrtab + syms[i].st_name);
			lua_pushcfunction(wsh->L, (void *) (syms[i].st_value + (unsigned long int) base_addr));
			lua_setglobal(wsh->L, newname);

			/**
			* Create a wrapper function with the original name
			*/
			char *luacmd = calloc(1, 2048);
			snprintf(luacmd, 2047, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", dynstrtab + syms[i].st_name, newname);
			luabuff_append(luacmd);
			exec_luabuff();		
			free(luacmd);
		}
	
	}
	return 0;
}

/**
* Load ELF segments memory
*/
void *elf_load(char *elf_start, unsigned int size, char *libname)
{
	Elf64_Ehdr *hdr = 0;
	Elf64_Phdr *phdr = 0;
	Elf64_Shdr *shdr = 0;
	Elf64_Sym *dynsymtab = 0;
	Elf64_Sym *symtab = 0;
	char *dynstrtab = 0;
	char *strtab = 0;
	char *start = 0;
	char *taddr = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	char *raw = 0;
	unsigned int nsyms = 0;
	int scount = 0, scount2 = 0;
//	char *text_base = 0;
	symbols_t *s = 0;

	Elf64_Shdr *shstrtab = 0;
	Elf64_Dyn *dyn = 0;

	hdr = (Elf64_Ehdr *) elf_start;

	DL_COUNT(wsh->symbols, s, scount);

	if (validate_elf(hdr)) {
                printf(" !! ERROR: parsing failed : invalid ELF in %s() at %s:%d\n", __func__, __FILE__, __LINE__);
		return -1;
	}

        // Find base load address
	phdr = (Elf64_Phdr *) (elf_start + hdr->e_phoff);
        if (!hdr->e_phnum) {
                printf(" !! ERROR: parsing failed : no Segments in ELF in %s() at %s:%d\n", __func__, __FILE__, __LINE__);
        }

        printf(" -- load address:   0x%lx\n", phdr[0].p_vaddr & 0xffffffff0000);
        load_address = phdr[0].p_vaddr & 0xffffffff0000 ? phdr[0].p_vaddr & 0xffffffff0000 : 0x400000;

	raw = mmap(load_address, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 0, 0);

	if (!raw) {
                printf(" !! ERROR: mmap() failed : %s in %s() at %s:%d\n", strerror(errno), __func__, __FILE__, __LINE__);
		return -1;
	}

	printf(" -- mapping ELF at: %p\n", raw);

	memset(raw, 0x0, size);

	phdr = (Elf64_Phdr *) (elf_start + hdr->e_phoff);

	// Load PT_LOAD segments (.text and .data)
	for (i = 0; i < hdr->e_phnum; ++i) {
		if (phdr[i].p_type != PT_LOAD) {
			continue;
		}
		if (phdr[i].p_filesz > phdr[i].p_memsz) {
                        printf(" !! ERROR: parsing failed : invalid ELF (p_filesz > p_memsz) in %s() at %s:%d\n", __func__, __FILE__, __LINE__);
			munmap(raw, size);
			return -1;
		}
		if (!phdr[i].p_filesz) {
			continue;
		}

		start = elf_start + phdr[i].p_offset;
		taddr = phdr[i].p_vaddr + raw;
		memmove(taddr, start, phdr[i].p_filesz);
//		printf("*** SEGMENT: %p\n", taddr);
		if (!(phdr[i].p_flags & PF_W)) {
			// READ ONLY SEGMENT
			mprotect((unsigned char *) taddr, phdr[i].p_memsz, PROT_READ);
		}

		if (phdr[i].p_flags & PF_X) {
			// EXECUTABLE SEGMENT
			mprotect((unsigned char *) taddr, phdr[i].p_memsz, PROT_EXEC | PROT_READ);
//			if(!text_base) {
//				text_base = taddr;
//				printf(" **** TEXT BASE: %p\n", text_base);
//			}
		}
	}


	// Load dynamic symbols
	shdr = (Elf64_Shdr *) (elf_start + hdr->e_shoff);
	for (i = 0; i < hdr->e_shnum; ++i) {
		if (shdr[i].sh_type == SHT_DYNSYM) {
			dynsymtab = (Elf64_Sym *) (elf_start + shdr[i].sh_offset);
			dynstrtab = elf_start + shdr[shdr[i].sh_link].sh_offset;
			parse_dynsym(shdr + i, dynstrtab, elf_start, raw, libname, raw);
		}
		if (shdr[i].sh_type == SHT_SYMTAB) {
			if (!symtab) {
				symtab = (Elf64_Sym *) (elf_start + shdr[i].sh_offset);
				strtab = elf_start + shdr[shdr[i].sh_link].sh_offset;
				nsyms = shdr[i].sh_size / sizeof(Elf64_Sym);
			}
		}
	}

	// Load regular symbols
	parse_sym(nsyms, symtab, strtab, elf_start, raw, libname, raw);

	// Parse dynamic segment and load needed libraries in address space
	shdr = (Elf64_Shdr *) (elf_start + hdr->e_shoff);
	for (i = 0; i < hdr->e_shnum; i++) {
		if (shdr[i].sh_type == SHT_DYNAMIC) {
			printf(" -- dynamic section found : section[%u]\n", i);
			dyn = elf_start + shdr[i].sh_offset;
			for (j = 0; j < (shdr[i].sh_size / sizeof(Elf64_Dyn)); j++) {
				switch (dyn->d_tag) {
				case DT_NEEDED:
					printf("    * needed library: %s\n", dynstrtab + dyn->d_un.d_val);
					void *aret = dlopen(dynstrtab + dyn->d_un.d_val, RTLD_NOW | RTLD_GLOBAL);
					if(!aret){
						fprintf(stderr, "ERROR: dlopen() %s \n", dlerror());
					}
					break;
				default:
					break;
				}
				dyn += 1;

			}
		}
	}

	// Find section string table's section (.shstrtab)
	if ((hdr->e_shstrndx) && (hdr->e_shstrndx <= hdr->e_shnum)) {
		if (shdr[hdr->e_shstrndx].sh_type == SHT_STRTAB) {
			shstrtab = &shdr[hdr->e_shstrndx];
			printf(" -- found shstrtab\n");
		}
	}
	// Perform relocations
	if ((dynsymtab) && (dynstrtab)) {
		for (i = 0; i < hdr->e_shnum; ++i) {
			if (shdr[i].sh_type == SHT_REL) {
				do_rel(shdr + i, dynsymtab, dynstrtab, elf_start, raw);
			}
			if (shdr[i].sh_type == SHT_RELA) {
				do_rela(shdr + i, dynsymtab, dynstrtab, elf_start, raw, shstrtab, shdr, elf_start);
			}
		}
	}

	DL_COUNT(wsh->symbols, s, scount2);
	printf(" ** binary loaded (%d new symbols)\n", scount2 - scount);
	return 0;

}

int userland_load_binary(char *fname)
{
	int fd = 0;
	struct stat sb;
	char *map = 0;
	Elf32_Ehdr *ehdr32;
	Elf64_Ehdr *ehdr64;

	printf(" ** attempting to load %s using userland loader\n", fname);

	fd = open(fname, O_RDONLY);
	if (fd <= 0) {
		printf("!! ERROR: couldn't open %s : %s\n", fname, strerror(errno));
		return -1;
	}

	if (fstat(fd, &sb) == -1) {
		printf("!! ERROR: couldn't stat %s : %s\n", fname, strerror(errno));
		return -1;
	}

	if ((unsigned int) sb.st_size < sizeof(Elf32_Ehdr)) {
		printf("!! ERROR: file %s is too small (%u bytes) to be a valid ELF.\n", fname, (unsigned int) sb.st_size);
		return -1;
	}

	map = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		printf("!! ERROR: couldn't mmap %s : %s\n", fname, strerror(errno));
		return -1;
	}

	switch (map[EI_CLASS]) {
	case ELFCLASS64:
		char *raw = calloc(1, sb.st_size+MIN_MAP_SIZE);
		if(!raw){
			printf("!! ERROR: couldn't calloc() %s : %s\n", fname, strerror(errno));
			return -1;		
		}
		memcpy(raw, map, sb.st_size);
	
		return elf_load(raw, sb.st_size+MIN_MAP_SIZE, fname);
	case ELFCLASS32:
	default:
		printf("!! ERROR: unknown ELF class\n");
		return -1;
	}

	munmap(map, sb.st_size);
	close(fd);
	return 0;
}

#else

int userland_load_binary(char *fname)
{
	return -1;
}
#endif
