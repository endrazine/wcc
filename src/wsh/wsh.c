/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2024 Jonathan Brossard
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

#include <libwitch/wsh.h>
#include <libwitch/wsh_functions.h>
#include <libwitch/wsh_help.h>
#include <libwitch/sigs.h>
#include <uthash.h>
#include <utlist.h>
#include <libgen.h>	// For basename()
#include <lauxlib.h>
#include <sys/sendfile.h> // For sendfile()

// Forward declarations
int wsh_usage(char *name);
int wsh_print_version(void);

// address sanitizer macro : disable a function by prepending ATTRIBUTE_NO_SANITIZE_ADDRESS to its definition
#if defined(__clang__) || defined (__GNUC__)
# define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address)) __attribute__((disable_sanitizer_instrumentation))
#else
# define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

#if defined(__has_feature)
#   if __has_feature(address_sanitizer) // clang
#       define __SANITIZE_ADDRESS__ 	// GCC
#   endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#define HAS_ASAN 1
#endif


#ifndef __amd64__
#define REG_RIP    16
#endif

#ifdef __arm__
#define REG_EFL 0
#define REG_ERR 0
#endif

#ifdef __LP64__ // Generic 64b
#define Elf_Ehdr    Elf64_Ehdr
#define Elf_Shdr    Elf64_Shdr
#define Elf_Sym     Elf64_Sym
#define Elf_Addr    Elf64_Addr
#define Elf_Sword   Elf64_Sxword
#define Elf_Section Elf64_Half
#define ELF_ST_BIND ELF64_ST_BIND
#define ELF_ST_TYPE ELF64_ST_TYPE
#define Elf_Rel     Elf64_Rel
#define Elf_Rela    Elf64_Rela
#define ELF_R_SYM   ELF64_R_SYM
#define ELF_R_TYPE  ELF64_R_TYPE
#define ELF_R_INFO  ELF64_R_INFO
#define Elf_Phdr    Elf64_Phdr
#define Elf_Xword   Elf64_Xword
#define Elf_Word    Elf64_Word
#define Elf_Off     Elf64_Off
#define ELFCLASS    ELFCLASS64
#define ELFMACHINE  EM_X86_64
#define CS_MODE     CS_MODE_64
#define RELOC_MODE  RELOC_X86_64
#define EXTRA_VDSO  "linux-vdso.so.1"
#else           // Generic 32b
#define Elf_Ehdr    Elf32_Ehdr
#define Elf_Shdr    Elf32_Shdr
#define Elf_Sym     Elf32_Sym
#define Elf_Addr    Elf32_Addr
#define Elf_Sword   Elf64_Sword
#define Elf_Section Elf32_Half
#define ELF_ST_BIND ELF32_ST_BIND
#define ELF_ST_TYPE ELF32_ST_TYPE
#define Elf_Rel     Elf32_Rel
#define Elf_Rela    Elf32_Rela
#define ELF_R_SYM   ELF32_R_SYM
#define ELF_R_TYPE  ELF32_R_TYPE
#define ELF_R_INFO  ELF32_R_INFO
#define Elf_Phdr    Elf32_Phdr
#define Elf_Xword   Elf32_Xword
#define Elf_Word    Elf32_Word
#define Elf_Off     Elf32_Off
#define ELFCLASS    ELFCLASS32
#define ELFMACHINE  EM_386
#define CS_MODE     CS_MODE_32
#define RELOC_MODE  RELOC_X86_32
#define EXTRA_VDSO  "linux-gate.so.1"
#endif

learn_t *protorecords = NULL;

/**
* Main wsh context
*/
extern wsh_t *wsh;

/**
* Bruteforce valid memory mapping ranges
*/
int bfmap(lua_State * L)
{
	unsigned long int pcounter = 0;
	unsigned int page_size = 4096;

	unsigned long long int i = 0, j = 0, r = 0;

	printf(GREEN "\n   Memory segments\n\n");

	for (r = 0; r < sizeof(ranges) / sizeof(range_t); r++) {
		for (i = ranges[r].min; i < ranges[r].max; i += page_size) {

			if (pcounter++ == 10000) {
				pcounter = 0;
				printf("  %016llx\r", i);
			}

			if (msync((void *) i, page_size, MS_ASYNC)) {
				continue;
			}	// Invalid page

			// We found a valid page, find length of mapping
			for (j = 0; j < 0x100000000; j += page_size) {
				if (msync((void *) i + j, page_size, 0)) {
					break;
				}
			}
			printf(NORMAL "  %016llx-%016llx\n" GREEN, i, i + j);
			i += j;
		}
	}
	printf(NORMAL "                       \r\n");
	return 0;
}

/**
* Get permissions in human readable format
*/
int ptoh(int perms, char hperms[])
{
	snprintf(hperms, 5, "%s%s%s", (perms & 0x04) ? "r" : "-", (perms & 0x02) ? "w" : "-", (perms & 0x01) ? "x" : "-");
	return 0;
}

/**
* Print information on a given function
*/
void info_function(void *addr)
{
	Dl_info dli;
	dladdr(addr, &dli);
	printf(" -- %s() = %p	from %s:%p\n", dli.dli_sname, dli.dli_saddr, dli.dli_fname, dli.dli_fbase);
}

/**
* Fatal error : print an error message and exit with error
*/
void fatal_error(lua_State * L, char *msg)
{
	fprintf(stderr, "\nFATAL ERROR:\n  %s: %s\n\n", msg, lua_tostring(L, -1));
	_Exit(EXIT_FAILURE);
}

/**
* Simple hexdump routine
*/
void hexdump(uint8_t * data, size_t size, size_t colorstart, size_t color_len)
{
	size_t i = 0, j = 0;

	for (j = 0; j < size; j += 16) {

		// Highlight offset in greed
		if (wsh->opt_hollywood) {
			printf(GREEN);
		}
		printf("%p    ", data + j);

		if (wsh->opt_hollywood) {
			printf(NORMAL);
		}

		for (i = j; i < j + 16; i++) {

			// Highlight match in red
			if ((wsh->opt_hollywood) && (color_len) && (colorstart == i)) {
				printf(RED);
			}
			if ((wsh->opt_hollywood) && (color_len) && (colorstart + color_len == i)) {
				printf(NORMAL);
			}

			if (i < size) {
				printf("%02x ", data[i] & 255);
			} else {
				printf("   ");
			}
		}

		printf("   ");

		for (i = j; i < j + 16; i++) {

			// Highlight match in red
			if ((wsh->opt_hollywood) && (color_len) && (colorstart == i)) {
				printf(RED);
			}
			if ((wsh->opt_hollywood) && (color_len) && (colorstart + color_len == i)) {
				printf(NORMAL);
			}

			if (i < size)
				putchar(32 <= (data[i] & 127) && (data[i] & 127) < 127 ? data[i] & 127 : '.');
			else
				putchar(' ');
		}
		putchar('\n');
	}
}

/**
* Resolve the address of a symbol within a given library
*/
static unsigned long int resolve_addr(char *symbol, char *libname)
{
	unsigned long int ret = 0;
	struct link_map *handle = 0;
	Dl_info dli;

	if ((!symbol) || (!*symbol)) {
		return -1;
	}

	handle = dlopen(libname, wsh->opt_global ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW);
	if (!handle) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		_Exit(EXIT_FAILURE);
	}

	dlerror();		/* Clear any existing error */

	ret = (unsigned long int) dlsym(handle, symbol);

	if (!ret) {
#ifdef PEDANTIC_WARNINGS
		char *err = 0;
		err = dlerror();

		if (err) {
			fprintf(stderr, "ERROR: %s\n", err);
			//_Exit(EXIT_FAILURE);
		}
#endif
		/* Ignore the symbol */
		dlclose(handle);

		return -1;
	}

	dladdr((void *) ret, &dli);

	// Is it the correct lib ?
	if ((dli.dli_fname) && (libname) && (strlen(libname)) && (strncmp(libname, dli.dli_fname, strlen(libname)))) {
		ret = -1;
	}

	dlclose(handle);

	return ret;
}

/**
* Return symbol binding type in human readable format
*/
char *symbol_tobind(int n)
{
	switch (n) {
	case 0:
		return "Local";
	case 1:
		return "Global";
	case 2:
		return "Weak";
	case 10:
		return "Unique";
	case 11:
		return "Secondary";
	default:
		break;
	}

	return "Default";
}

/**
* Return symbol type in human readable format
*/
char *symbol_totype(int n)
{
	switch (n) {
	case 0:
		return "Notype";
	case 1:
		return "Object";
	case 2:
	case STT_GNU_IFUNC:
		return "Function";
	case 3:
		return "SECTION";
	case 4:
		return "File";
	case 5:
		return "Common";
	case 6:
		return "TLS";
	default:
		break;
	}

	return "Default";
}

unsigned int ltrace(void)
{
	return 0;
}


/**
* Scan a symbol, save it to linked list
*/
int scan_symbol(char *symbol, char *libname)
{
	struct link_map *handle;
	Dl_info dli;
	Elf_Sym *s = 0;
	char *htype = 0, *hbind = 0;
	unsigned long int ret = 0;
	unsigned int stype = 0, sbind = 0;
	int retv = 0;

	handle = dlopen(libname, wsh->opt_global ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW);
	if (!handle) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		_Exit(EXIT_FAILURE);
	}

	dlerror();		/* Clear any existing error */

	ret = (unsigned long int) dlsym(handle, symbol);

	if ((dladdr1((void*)ret, &dli, (void **) &s, RTLD_DL_SYMENT))&&(s)) {

		stype = ELF_ST_TYPE(s->st_info);
		htype = symbol_totype(stype);

		sbind = ELF_ST_BIND(s->st_info);
		hbind = symbol_tobind(sbind);

		retv = add_symbol(symbol, libname, htype, hbind, s->st_value, s->st_size, ret);
		if(retv){return retv;}
	}

	dlclose(handle);
	return 0;
}

/**
* Shell autocompletion routine
*/
void completion(const char *buf, linenoiseCompletions * lc)
{
	/**
	* We want to add the next word uppon 'tab' completion,
	* exposing all the internally available keywords dynamically
	*/
	char *opt, *word = 0;
	unsigned int n = 0, i = 0;
	unsigned int p = 0, w = 0;

	n = strlen(buf);
	switch (n) {
	case 0:
		// No letter given, add default commands to completion options
		for (i = 0; i < sizeof(default_options) / sizeof(char *); i++) {
			linenoiseAddCompletion(lc, default_options[i]);
		}

		// Add reflected symbols
		symbols_t *s, *stmp;
		DL_FOREACH_SAFE(wsh->symbols, s, stmp) {
			linenoiseAddCompletion(lc, s->symbol);
		}

		return;
		break;
	default:		// Input buffer is non empty:
		// the n first characters need to stay,
		// the last word needs to be completed with possible options
		opt = strdup(buf);
		// find position of last word
		for (p = strlen(opt); p > 0; p--) {
			if ((opt[p] == 0x20)||(opt[p] == 0x28)) {
				w = p + 1;
				break;
			}
		}
		// last word now starts at opt[w]
		word = opt + w;

		if (strlen(word) == n) {	// There is no space in input tokens : it is a single command, add all those that match

			// Add core functions
			for (i = 0; i < sizeof(default_options) / sizeof(char *); i++) {
				if (!strncmp(buf, default_options[i], strlen(buf))) {
					linenoiseAddCompletion(lc, default_options[i]);
				}
			}

			// Add all lua default functions
			for (i = 0; i < sizeof(lua_default_functions) / sizeof(char *); i++) {
				if (!strncmp(buf, lua_default_functions[i], strlen(buf))) {
					linenoiseAddCompletion(lc, lua_default_functions[i]);
				}
			}

			// Add reflected symbols
			symbols_t *s, *stmp;
			DL_FOREACH_SAFE(wsh->symbols, s, stmp) {
				if (!strncmp(buf, s->symbol, strlen(buf))) {
					linenoiseAddCompletion(lc, s->symbol);
				}
			}


		} else {	// There is more than one word in this command
//TODO

		}
//              linenoiseAddCompletion(lc, buf);
		break;
	}

}

/**
* Disable ASLR
*/
int disable_aslr(void)
{
	int fd = 0;
	char c = 0x30;

	fd = open(PROC_ASLR_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "!! ERROR : open(%s, O_RDWR) %s\n", PROC_ASLR_PATH, strerror(errno));
		return -1;
	}
	write(fd, &c, 1);
	close(fd);
	return 0;
}

/**
* Enable ASLR
*/
int enable_aslr(void)
{
	int fd = 0;
//      char c = 0x31;
	char c = 0x32;

	fd = open(PROC_ASLR_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "!! ERROR : open(%s,O_RDWR) %s\n", PROC_ASLR_PATH, strerror(errno));
		return -1;
	}
	write(fd, &c, 1);
	close(fd);
	return 0;
}

/**
* Display detailed help
*/
int detailed_help(char *name)
{
	unsigned int i = 0;

	/**
	* Search command
	*/
	for(i=0 ; i < sizeof(cmdhelp)/sizeof(help_t) ; i++){
		if(!strncmp(cmdhelp[i].name, name, strlen(cmdhelp[i].name))){
			printf("\n\tWSH HELP FOR COMMAND %s\n\n\n", name);
			printf("NAME\n\n\t%s\n\nSYNOPSIS\n\n\t%s %s\n\nDESCRIPTION\n\n\t%s\n\nRETURN VALUES\n\n\t%s\n\n\n", cmdhelp[i].name, cmdhelp[i].name, cmdhelp[i].proto, cmdhelp[i].descr, cmdhelp[i].retval);
			return 0;
		}
	}

	/**
	* Search function
	*/
	for(i=0 ; i < sizeof(fcnhelp)/sizeof(help_t) ; i++){
		if(!strncmp(fcnhelp[i].name, name, strlen(fcnhelp[i].name))){
			printf("\n\tWSH HELP FOR FUNCTION %s\n\n\n", name);
			printf("NAME\n\n\t%s\n\nSYNOPSIS\n\n\t%s%s(%s)\n\nDESCRIPTION\n\n\t%s\n\nRETURN VALUES\n\n\t%s\n\n\n", fcnhelp[i].name, fcnhelp[i].protoprefix, fcnhelp[i].name, fcnhelp[i].proto, fcnhelp[i].descr, fcnhelp[i].retval);
			return 0;
		}
	}

	printf("ERROR:\tNo help available for function %s()\n", name);
	return 0;
}

/**
* Display help
*/
int help(lua_State * L)
{
	const char *arg = 0;

	if (lua_isstring(L, 1)) {
		arg = luaL_checkstring(L, 1);
		detailed_help((char *) arg);
	} else {
		printf("  [Shell commands]\n\n\thelp, quit, exit, shell, exec, clear\n\n");
		printf("  [Functions]\n\n");
		printf(" + basic:\n\thelp(), man()\n\n");
		printf(" + memory display:\n\t hexdump(), hex_dump(), hex()\n\n");
		printf(" + memory maps:\n\tshdrs(), phdrs(), map(), procmap(), bfmap()\n\n");
		printf(" + symbols:\n\tsymbols(), functions(), objects(), info(), search(), headers()\n\n");
		printf(" + memory search:\n\tgrep(), grepptr()\n\n");
		printf(" + load libaries:\n\tloadbin(), libs(), entrypoints(), rescan()\n\n");
		printf(" + code execution:\n\tlibcall()\n\n");
		printf(" + buffer manipulation:\n\txalloc(), ralloc(), xfree(), balloc(), bset(), bget(), rdstr(), rdnum()\n\n");
		printf(" + control flow:\n\t breakpoint(), bp()\n\n");
		printf(" + system settings:\n\tenableaslr(), disableaslr()\n\n");
		printf(" + settings:\n\t verbose(), hollywood()\n\n");
		printf(" + advanced:\n\tltrace()\n\nTry help(\"cmdname\") for detailed usage on command cmdname.\n\n");
	}
	return 0;
}

/**
* Decode Segment flags
*/
char *decode_flags(unsigned int flags)
{
	char message[20];
	unsigned int pf_x = (flags & 0x1);
	unsigned int pf_w = (flags & 0x2);
	unsigned int pf_r = (flags & 0x4);

	memset(message, 0x00, 20);
	if (pf_r){
		strcat(message, "r");
	}else{
		strcat(message, "-");
	}
	if (pf_w){
		strcat(message, "w");
	}else{
		strcat(message, "-");
	}
	if (pf_x){
		strcat(message, "x");
	}else{
		strcat(message, "-");
	}
	return strdup(message);
}

/**
* Decode Segment type
*/
char *decode_type(unsigned int type)
{
	char *ret = 0;

	switch (type) {
	case 0:
		return "PT_NULL";
		break;
	case 1:
		return "PT_LOAD";
		break;
	case 2:
		return "PT_DYNAMIC";
		break;
	case 3:
		return "PT_INTERP";
		break;
	case 4:
		return "PT_NOTE";
		break;
	case 5:
		return "PT_SHLIB";
		break;
	case 6:
		return "PT_PHDR";
		break;
	case 7:
		return "PT_TLS";
		break;
	case 8:
		return "PT_NUM";
		break;
	case 0x6474e550:
		return "PT_GNU_EH_FRAME";
		break;
	case 0x6474e551:
		return "PT_GNU_STACK";
		break;
	case 0x6474e552:
		return "PT_GNU_RELRO";
		break;

	case 0x6474e553:
		return "PT_GNU_PROPERTY";
		break;

	default:
		ret = calloc(1, 200);
		snprintf(ret, 199, "Unknown: 0x%x\n", type);
		return ret;	// leak
		break;
	}
}

/**
* Callback function to parse Program headers (ELF Segments)
*/
int phdr_callback(struct dl_phdr_info *info, size_t size, void *data)
{
	char *pflags = 0, *ptype = 0;
	const char *fname = 0;
	Elf_Phdr *p = 0;
	int j = 0;

	for (j = 0; j < info->dlpi_phnum; j++) {
		p = (Elf_Phdr *) &info->dlpi_phdr[j];

		pflags = p ? decode_flags(p->p_flags) : 0;
		ptype = decode_type(p->p_type);
		fname = info->dlpi_name;
		if((!fname)||(strlen(fname) < 2)){
#ifdef DEBUG
			if(info->dlpi_addr + p->p_vaddr >= 0x7fd000000000){
				fname = "[vdso]";
			}else{
				fname = realpath(__progname_full,0);	// leak
			}
#else
			return 0;
#endif
		}

		// Save segment
		segment_add(info->dlpi_addr + p->p_vaddr, p->p_memsz, pflags, fname, ptype, p->p_flags);
	}

	return 0;
}

/**
* Add a symbol to linked list
*/
int add_symbol(char *symbol, char *libname, char *htype, char *hbind, unsigned long value, unsigned int size, unsigned long int addr)
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
* Add a section to linked list
*/
void section_add(unsigned long int addr, unsigned long int size, char *libname, char *name, char *perms, int flags)
{
	sections_t *s = 0;

	s = calloc(1, sizeof(sections_t));
	if(!s){ fprintf(stderr, " !! Error: calloc() = %s\n", strerror(errno)); return; }
	s->addr = addr;
	s->size = size;
	s->flags = flags;
	s->libname = strdup(libname);
	s->name = strdup(name);
	s->perms = strdup(perms);

	DL_APPEND(wsh->shdrs, s);
}

/**
* Add a segment to linked list
*/
void segment_add(unsigned long int addr, unsigned long int size, char *perms, char *fname, char *ptype, int flags)
{
	segments_t *s = 0;

	s = calloc(1, sizeof(segments_t));
	if(!s){ fprintf(stderr, " !! Error: calloc() = %s\n", strerror(errno)); return; }
	s->addr = addr;
	s->size = size;
	s->flags = flags;
	s->libname = strdup(fname);
	s->perms = strdup(perms);
	s->type = strdup(ptype);

	DL_APPEND(wsh->phdrs, s);
}

/**
* Add an entry point to linked list
*/
void entry_point_add(unsigned long long int addr, char *fname)
{
	eps_t *s = 0;

	s = calloc(1, sizeof(eps_t));
	s->name = strdup(fname);
	s->addr = addr;

	DL_APPEND(wsh->eps, s);
}

/**
* Parse a section from an ELF
*/
void scan_section(Elf_Shdr * shdr, char *strTab, int shnum, char *fname, unsigned long int baseaddr)
{
	int i = 0;
	char hperms[5];

	for (i = 0; i < shnum; i++) {
		memset(hperms, 0x00, 5);
		snprintf(hperms, 5, "%s%s%s", (shdr[i].sh_flags & 0x02) ? "r" : "-", (shdr[i].sh_flags & 0x01) ? "w" : "-", (shdr[i].sh_flags & 0x04) ? "x" : "-");

		if(shdr[i].sh_addr){
			section_add(shdr[i].sh_addr + baseaddr, shdr[i].sh_size, fname, &strTab[shdr[i].sh_name], hperms, shdr[i].sh_flags);
		}
	}
}

/**
* Parse all sections from an ELF
*/
int scan_sections(char *fname, unsigned long int baseaddr)
{
	void *data = 0;
	Elf_Ehdr *elf = 0;
	Elf_Shdr *shdr = 0;
	int fd = 0;
	char *strtab = 0;

	fd = open(fname, O_RDONLY);
	data = mmap(NULL, lseek(fd, 0, SEEK_END), PROT_READ, MAP_SHARED, fd, 0);
	elf = (Elf_Ehdr *) data;
	if(elf == (void*)-1){ return -1; }

	entry_point_add(elf->e_entry + baseaddr, fname);

	shdr = (Elf_Shdr *) (data + elf->e_shoff);
	strtab = (char *) (data + shdr[elf->e_shstrndx].sh_offset);
	scan_section(shdr, strtab, elf->e_shnum, fname, baseaddr);
	close(fd);
	return 0;
}

/**
* Callback function to parse Section headers (ELF Sections)
*/
int shdr_callback(struct dl_phdr_info *info, size_t size, void *data)
{
	if(strlen(info->dlpi_name) < 2){
		return 0;
	}

	scan_sections(info->dlpi_name, info->dlpi_addr);
	return 0;
}

/**
* Display Program headers (ELF Segments)
*/
int phdrs(lua_State * L)
{
	print_phdrs();
	return 0;
}

/**
* Find section from address
*/
sections_t *section_from_addr(unsigned long int addr)
{
	sections_t *s = 0, *stmp = 0, *res = 0;

	DL_FOREACH_SAFE(wsh->shdrs, s, stmp) {
		if((s->addr <= addr)&&(s->addr + s->size >= addr)){
			res = s;
		}
	}
	return res;
}

/**
* Find segment from address
*/
segments_t *segment_from_addr(unsigned long int addr)
{
	segments_t *s = 0, *stmp = 0, *res = 0;

	DL_FOREACH_SAFE(wsh->phdrs, s, stmp) {
		if((s->addr <= addr)&&(s->addr + s->size >= addr)){
			res = s;
		}
	}
	return res;
}

/**
* Return a symbol from an address
*/
symbols_t *symbol_from_addr(unsigned long int addr)
{
	symbols_t *s = 0, *stmp = 0, *res = 0;

	DL_FOREACH_SAFE(wsh->symbols, s, stmp) {
		if((s->addr <= addr)&&(s->addr + s->size >= addr)){
			res = s;
		}
	}
	return res;
}

/**
* Return a symbol from its name
*/
symbols_t *symbol_from_name(char *fname)
{
	symbols_t *s = 0, *stmp = 0;

	DL_FOREACH_SAFE(wsh->symbols, s, stmp) {
		if(!strncmp(fname,s->symbol,strlen(fname))){
			return s;
		}
	}
	return NULL;
}


/**
* Generate headers
*/
int headers(lua_State * L)
{
	symbols_t *s = 0, *stmp = 0;
	unsigned int scount = 0;

	char *libname = 0;

	read_arg1(libname);

	printf("/**\n*\n* Automatically generated by the Witchcraft Compiler Collection %s\n*\n* %s %s\n*\n*/\n\n\n", WVERSION, WTIME, WDATE);

	/**
	* generate headers for imported objects
	*/
	printf("/**\n* Imported objects\n**/\n");
	DL_FOREACH_SAFE(wsh->symbols, s, stmp) {

		if((!libname)||(strstr(s->libname, libname))){
			if(!strncmp(s->htype,"Object",6)){
				scount++;
				printf("extern void *%s;\n", s->symbol);
			}
		}
	}

	/**
	* generate forward prototypes for imported functions
	*/
	printf("\n\n/**\n* Imported functions\n**/\n");
	DL_FOREACH_SAFE(wsh->symbols, s, stmp) {

		if((!libname)||(strstr(s->libname, libname))){
			if(strncmp(s->htype,"Object",6)){
				if(strncmp(s->symbol, "main", 5)){
					printf("void *%s();\n", s->symbol);
				}
			}
		}
	}

	return 0;
}


/**
* Empty linked list of symbols
*/
int empty_symbols(void)
{
	symbols_t *s = 0, *stmp = 0;

	DL_FOREACH_SAFE(wsh->symbols, s, stmp) {
			DL_DELETE(wsh->symbols, s);
			free(s->symbol);
			free(s->hbind);
			free(s->libname);
			free(s->htype);
			free(s);

	}

	return 0;
}

/**
* Empty linked list of segments
*/
int empty_phdrs(void)
{
	segments_t *s = 0, *stmp = 0;

	DL_FOREACH_SAFE(wsh->phdrs, s, stmp) {
			DL_DELETE(wsh->phdrs, s);

			free(s->type);
			free(s->libname);
			free(s->perms);
			free(s);

	}

	return 0;
}

/**
* Empty linked list of sections
*/
int empty_shdrs(void)
{
	sections_t *s = 0, *stmp = 0;

	DL_FOREACH_SAFE(wsh->shdrs, s, stmp) {
			DL_DELETE(wsh->shdrs, s);

			free(s->name);
			free(s->libname);
			free(s->perms);
			free(s);
	}

	return 0;
}

/**
* Empty linked list of entry points
*/
int empty_eps(void)
{
	eps_t *s = 0, *stmp = 0;

	DL_FOREACH_SAFE(wsh->eps, s, stmp) {
			DL_DELETE(wsh->eps, s);

			free(s->name);
			free(s);
	}

	return 0;
}

/**
* Display program headers (ELF Segments)
*/
int print_phdrs(void)
{
	char *lastlib = "";
	segments_t *s = 0, *stmp = 0;
	unsigned int scount = 0;
	DL_COUNT(wsh->phdrs, s, scount);

	printf(" -- Total: %u segments\n", scount);	

	DL_FOREACH_SAFE(wsh->phdrs, s, stmp) {
			if(strncmp(lastlib,s->libname,strlen(lastlib))){
				printf("\n");
			}
			lastlib = s->libname;
			char *pcolor = DARKGRAY;	// NORMAL

			switch (s->flags) {
			case 4:	// r--
				pcolor = GREEN;
				break;
			case 6:	// rw-
				pcolor = BLUE;
				break;
			case 5:	// r-x
				pcolor = RED;
				break;
			case 7:	// rwx
				pcolor = MAGENTA;
				break;
			default:
				break;
			}

			if(s->size == 0){
				pcolor = DARKGRAY;
			}


			if(wsh->opt_hollywood){
				printf(NORMAL "%012lx-%012lx%s\t%s\t%lu\t%s\t%s"NORMAL"\n", s->addr, s->addr + s->size,
					pcolor, s->perms, s->size, s->libname, s->type);
			}else{
				printf("%012lx-%012lx\t%s\t%lu\t%s\t%s\n", s->addr, s->addr + s->size,
					s->perms, s->size, s->libname, s->type);
			}
	}

	printf("\n");
	printf(" -- Total: %u segments\n", scount);	

	return 0;
}

const char* indirect_function_names[] = {
	"__gettimeofday",
	"__memcmpeq",
	"__memcpy_chk",
	"__memmove_chk",
	"__mempcpy",
	"__mempcpy_chk",
	"__memset_chk",
	"__rawmemchr",
	"__stpcpy",
	"__stpncpy",
	"__strcasecmp",
	"__strcasecmp_l",
	"__strncasecmp_l",
	"__wmemset_chk",
	"bcmp",
	"gettimeofday",
	"index",
	"memchr",
	"memcmp",
	"memcpy",
	"memmove",
	"mempcpy",
	"memrchr",
	"memset",
	"rawmemchr",
	"rindex",
	"stpcpy",
	"stpncpy",
	"strcasecmp",
	"strcasecmp_l",
	"strcat",
	"strchr",
	"strchrnul",
	"strcmp",
	"strcpy",
	"strcspn",
	"strlen",
	"strncasecmp",
	"strncasecmp_l",
	"strncat",
	"strncmp",
	"strncpy",
	"strnlen",
	"strpbrk",
	"strrchr",
	"strspn",
	"strstr",
	"time",
	"wcschr",
	"wcscmp",
	"wcscpy",
	"wcslen",
	"wcsncmp",
	"wcsnlen",
	"wcsrchr",
	"wmemchr",
	"wmemcmp",
	"wmemset",
	NULL
};

/**
* Helper function
*/
/*
uintptr_t find_libc_base() {
    uintptr_t libc_pointer;
    for(libc_pointer = (uintptr_t)printf & (~0xFFF); *(uint32_t*)libc_pointer != 0x464c457f; libc_pointer-=0x1000);
    return libc_pointer;
}
*/
#define MAX_NUM_IMPL 256

//From "include/ifunc-impl-list.h"
struct libc_ifunc_impl
{
  /* The name of function to be tested.  */
  const char *name;
  /* The address of function to be tested.  */
  void (*fn) (void);
  /* True if this implementation is usable on this machine.  */
  bool usable;
};

extern size_t __libc_ifunc_impl_list (const char *name, struct libc_ifunc_impl *array, size_t max);


/**
* Load Indirect functions
*/
int load_indirect_functions(lua_State * L)
{
    struct libc_ifunc_impl entries[MAX_NUM_IMPL];

	int retv = 0;

    for(const char** function_name = indirect_function_names; *function_name; function_name++) {
        size_t num_entries = __libc_ifunc_impl_list(*function_name, entries, MAX_NUM_IMPL);

	void *addr =  dlsym(NULL, *function_name);
//	printf("%s %p\n", *function_name, addr);

	Dl_info info;
	Elf64_Sym *mydlsym = calloc(1, sizeof(Elf64_Sym));
	if (dladdr1(addr, &info, (void **) &mydlsym, RTLD_DL_SYMENT)) {
//		printf("INFO:dli_fname:%s\n", info.dli_fname);
//		printf("INFO:dli_fbase:%p\n", info.dli_fbase);

		retv = add_symbol(*function_name, info.dli_fname, "Function", "Global", dlsym(NULL, *function_name)-info.dli_fbase, 8, dlsym(NULL, *function_name));
	}else{
		retv = add_symbol(*function_name, "indirect:libc.so", "Function", "Global", dlsym(NULL, *function_name), 8, dlsym(NULL, *function_name));
	}

	char *luacmd = calloc(1, 2048);
	snprintf(luacmd, 2047, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%p, a, b, c, d, e, f, g, h); return j, k; end\n", *function_name, dlsym(NULL, *function_name));
	luabuff_append(luacmd);
	exec_luabuff();
	free(luacmd);

        for(size_t i = 0; i < num_entries; i++) {

		if (dladdr1((void*)((uintptr_t)entries[i].fn), &info, (void **) &mydlsym, RTLD_DL_SYMENT)) {
//			printf("INFO:dli_fname:%s\n", info.dli_fname);
//			printf("INFO:dli_fbase:%p\n", info.dli_fbase);

			retv = add_symbol(entries[i].name, info.dli_fname, "Function", "Global", (void*)((uintptr_t)entries[i].fn)-info.dli_fbase, 8, (void*)((uintptr_t)entries[i].fn));
		}else{
			retv = add_symbol(entries[i].name, "indirect:libc.so", "Function", "Global", (void*)((uintptr_t)entries[i].fn), 8, (void*)((uintptr_t)entries[i].fn));
		}
		char *luacmd = calloc(1, 2048);
		snprintf(luacmd, 2047, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%p, a, b, c, d, e, f, g, h); return j, k; end\n", entries[i].name, (void*)((uintptr_t)entries[i].fn));
		luabuff_append(luacmd);
		exec_luabuff();
		free(luacmd);
        }
    }

    return 0;

	return 0;
}

/**
* Display Indirect functions
*/
int print_indirect_functions(lua_State * L)
{
    struct libc_ifunc_impl entries[MAX_NUM_IMPL];

//	int retv = 0;

    for(const char** function_name = indirect_function_names; *function_name; function_name++) {
        size_t num_entries = __libc_ifunc_impl_list(*function_name, entries, MAX_NUM_IMPL);

	void *addr =  dlsym(NULL, *function_name);

	Dl_info info;
	if (dladdr(addr, &info)) {
		printf("%p %s %s %p %lx\n", addr, *function_name,info.dli_fname, info.dli_fbase, addr - info.dli_fbase);
	}
        for(size_t i = 0; i < num_entries; i++) {
		addr =  dlsym(NULL, *function_name);
		if (dladdr(addr, &info)) {
			printf("  %p %s %s %p %lx\n", addr, entries[i].name,info.dli_fname, info.dli_fbase, addr - info.dli_fbase);
		}

        }
    }

    return 0;
}

/**
* Display symbols
*/
int print_symbols(lua_State * L)
{
	unsigned int scount = 0;
	symbols_t *s = 0, *stmp = 0;
	unsigned int i = 0;
	unsigned int pcnt = 0;
	char *symname = 0;
	char *libname = 0;
	unsigned int returnall = 0;

	read_arg1(symname);
	read_arg2(libname);
	read_arg3(returnall);

	DL_COUNT(wsh->symbols, s, scount);

	if(returnall < 2){
		printf(" -- Total: %u symbols\n", scount);	
		printf(" -- Symbols:\n\n");
	//      printf("    Type       Size                     Path                  Address              Name           (Demangled)\n");
		printf("-----------------------------------------------------------------------------------------------------------------\n");
	}

	/* create result table */
	lua_newtable(L);

	DL_FOREACH_SAFE(wsh->symbols, s, stmp) {
		if((!symname)||(strstr(s->symbol, symname))){
			if((!libname)||(strstr(s->libname, libname))){

				if(returnall < 2){
					printf("%s ", s->libname);
					for (i = strlen(s->libname); i < 40; i++)
						printf(" ");
					printf("%s ", s->symbol);
					for (i = strlen(s->symbol); i < 30; i++)
						printf(" ");
					printf("%s ", s->htype);
					for (i = strlen(s->htype); i < 10; i++)
						printf(" ");
					printf(" %s	%lx	\t\t%lu	%lx\n", s->hbind, s->value, s->size, s->addr);
				}

				/* Add symbol to Lua table */
				lua_pushstring(L, s->symbol);		/* push key */
				lua_getglobal(L, s->symbol);		/* get pointer to global with this name : keep it as value on top of stack */
			        lua_settable(L, -3);

				pcnt++;
				if((!returnall)&&(pcnt == LINES_MAX)){ pcnt = 0; int c = getchar(); switch(c){case 0x61: pcnt = LINES_MAX + 1; break; case 0x71: return 0; break; default: break;   }; }
			}
		}
	}

	if(returnall < 2){
		printf("\n");
		printf(" -- %u symbols matched\n", pcnt);	
	}

	// Return scount as second return value
	lua_pushinteger(L, scount);

	return 2;	// Return 1 table + number of match
}

/**
* Display functions
*/
int print_functions(lua_State * L)
{
	unsigned int scount = 0;
	symbols_t *s = 0, *stmp = 0;
	unsigned int i = 0;
	unsigned int pcnt = 0;
	char *libname = 0;
	char *symname = 0;
	unsigned int returnall = 0;

	read_arg1(symname);
	read_arg2(libname);
	read_arg3(returnall);

	DL_COUNT(wsh->symbols, s, scount);

	if(returnall < 2){
		printf(" -- Total: %u symbols\n", scount);	
		printf(" -- Functions:\n");
		printf("-----------------------------------------------------------------------------------------------------------------\n");
	}

	scount = 0;

	/* create result table */
	lua_newtable(L);

	DL_FOREACH_SAFE(wsh->symbols, s, stmp) {

		if(!strncmp(s->htype,"Function",8)){

			if((!symname)||(strstr(s->symbol, symname))){

				if((!libname)||(strstr(s->libname, libname))){
					scount++;

					if(returnall < 2){

						printf("%s ", strlen(s->libname) ? s->libname : wsh->selflib);
						for (i = strlen(s->libname); i < 40; i++)
							printf(" ");
						printf("%s ", s->symbol);
						for (i = strlen(s->symbol); i < 30; i++)
							printf(" ");
						printf("%s ", s->htype);
						for (i = strlen(s->htype); i < 10; i++)
							printf(" ");
						printf(" %s	%lx	\t\t%lu	%lx\n", s->hbind, s->value, s->size, s->addr);
					}

					/* Add function to Lua table */
					lua_pushstring(L, s->symbol);		/* push key */
					lua_getglobal(L, s->symbol);		/* get pointer to global with this name : keep it as value on top of stack */
					lua_settable(L, -3);

					/* handle breaks via getchar() */
					pcnt++;
					if((!returnall)&&(pcnt == LINES_MAX)){ pcnt = 0; int c = getchar(); switch(c){case 0x61: pcnt = LINES_MAX + 1; break; case 0x71: return 0; break; default: break;   }; }
				}

			}
		}
	}

	if(returnall < 2){
		printf("\n");
		printf(" -- %u functions matched\n", scount);	
	}

	// Return scount as second return value
	lua_pushinteger(L, scount);

	return 2;	// Return 1 table + number of match
}

/**
* Display objects (typically globals)
*/
int print_objects(lua_State * L)
{
	unsigned int scount = 0;
	symbols_t *s = 0, *stmp = 0;
	unsigned int i = 0;
	unsigned int pcnt = 0;

	char *libname = 0;

	read_arg1(libname);

	DL_COUNT(wsh->symbols, s, scount);
	printf(" -- Total: %u symbols\n", scount);	

	scount = 0;
	printf(" -- Objects:\n\n");
//      printf("    Type       Size                     Path                  Address              Name           (Demangled)\n");
	printf("-----------------------------------------------------------------------------------------------------------------\n");


	DL_FOREACH_SAFE(wsh->symbols, s, stmp) {

		if((!libname)||(strstr(s->libname, libname))){

			if(!strncmp(s->htype,"Object",6)){
				char *sname = 0;
				sname = strlen(s->libname) ? s->libname : wsh->selflib;
				scount++;
				printf("%s ", sname);
				for (i = strlen(sname); i < 40; i++)
					printf(" ");
				printf("%s ", s->symbol);
				for (i = strlen(s->symbol); i < 30; i++)
					printf(" ");
				printf("%s ", s->htype);
				for (i = strlen(s->htype); i < 10; i++)
					printf(" ");
				printf(" %s	%lx	\t\t%lu	%lx\n", s->hbind, s->value, s->size, s->addr);

				pcnt++;
				if(pcnt == LINES_MAX){ pcnt = 0; int c = getchar(); switch(c){case 0x61: pcnt = LINES_MAX + 1; break; case 0x71: return 0; break; default: break;   }; }
			}
		}
	}

	printf("\n");
	printf(" -- %u objects matched\n", scount);	

	return 0;
}

/**
* Display mapped librairies, return a list of library names
*/
int print_libs(lua_State * L)
{
	char *lastlib = "none";
	sections_t *s = 0, *stmp = 0;
	unsigned int scount = 0;

	/* create result table */
	lua_newtable(L);

	DL_FOREACH_SAFE(wsh->phdrs, s, stmp) {
			if(strncmp(lastlib,s->libname,strlen(lastlib))){
				scount++;
				printf("%s\n",s->libname);

				/* Add function to Lua table */
				lua_pushnumber(L, scount);		/* push key */
			        lua_pushstring(L, s->libname);		/* push value */
			        lua_settable(L, -3);
			}
			lastlib = s->libname;
	}

	/**
	* Define vdso
	*/
	printf("%s\n", EXTRA_VDSO);
	scount++;
	// Add function to Lua table //
	lua_pushnumber(L, scount);
	lua_pushstring(L, EXTRA_VDSO);
	lua_settable(L, -3);

	/* add self and vdso as shared library */
	if(wsh->opt_appear){
		// self
		scount++;
		printf("%s\n",wsh->selflib);

		// Add function to Lua table
		lua_pushnumber(L, scount);
	        lua_pushstring(L, wsh->selflib);
	        lua_settable(L, -3);

	}

	printf("\n");
	printf(" -- Total: %u libraries\n", scount);	

	// Return scount as second return value
	lua_pushinteger(L, scount);

	return 2;	// Return 1 table + number of matches
}

/**
* Display ELF sections
*/
int print_shdrs(void)
{
	char *lastlib = "";
	sections_t *s = 0, *stmp = 0;
	unsigned int scount = 0;
	char *segmenttype = "";
	char *segmentperms = "";
	segments_t *seg = 0;

	DL_COUNT(wsh->shdrs, s, scount);

	printf(" -- Total: %u sections\n", scount);	

	DL_FOREACH_SAFE(wsh->shdrs, s, stmp) {
			if(strncmp(lastlib,s->libname,strlen(lastlib))){
				printf("\n");
			}
			lastlib = s->libname;
			char *pcolor = DARKGRAY;	// NORMAL
			switch(s->flags&0x0f){
			case 2:	// r--
				pcolor = GREEN;
				break;
			case 3:	// rw-
				pcolor = BLUE;
				break;
			case 6:	// r-x
				pcolor = RED;
				break;
			case 7:	// rwx
			case 8:	// rwx
			case 9:	// rwx
				pcolor = MAGENTA;
				break;
			default:
				break;
			}

			segmenttype = "";
			segmentperms = "";
			seg = 0;

			seg = segment_from_addr(s->addr);
			if(seg){
				segmenttype = seg->type;
				segmentperms = seg->perms;
			}

			if(wsh->opt_hollywood){
				printf(NORMAL "%012lx-%012lx%s\t%s\t%lu\t%s\t%25s\t%s\t%s" NORMAL "\n", s->addr, s->addr + s->size, pcolor, s->perms, s->size, s->libname, s->name, segmenttype, segmentperms);
			}else{
				printf("%012lx-%012lx\t%s\t%lu\t%s\t%25s\t%s\t%s\n", s->addr, s->addr + s->size, s->perms, s->size, s->libname, s->name, segmenttype, segmentperms);
			}
	}

	printf("\n");
	printf(" -- Total: %u sections\n", scount);	

	return 0;
}

/**
* Display Entry points
*/
int print_eps(void)
{
	eps_t *s = 0, *stmp = 0;
	unsigned int scount = 0;

	DL_COUNT(wsh->eps, s, scount);

	printf(" -- Total: %u entry points\n\n", scount);	

	DL_FOREACH_SAFE(wsh->eps, s, stmp) {
		printf("%012llx\t%s\n", s->addr, s->name);
	}
	return 0;
}

/**
* Sort function helper for sections
*/
int shdr_cmp(sections_t *a, sections_t *b)
{
	return (a->addr - b->addr);
}

/**
* Sort function helper for segments
*/
int phdr_cmp(segments_t *a, segments_t *b)
{
	return (a->addr - b->addr);
}

/**
* Reload linked lists from ELFs binaries
*/
int reload_elfs(void)
{
	empty_eps();

	empty_phdrs();
	dl_iterate_phdr(phdr_callback, NULL);
	DL_SORT(wsh->phdrs, phdr_cmp);

	empty_shdrs();
	dl_iterate_phdr(shdr_callback, NULL);
	DL_SORT(wsh->shdrs, shdr_cmp);

	return 0;
}

/**
* Display section headers (ELF Sections)
*/
static int shdrs(lua_State * L)
{
	print_shdrs();
	return 0;
}


/**
* Display ELF Entry points
*/
int entrypoints(lua_State * L)
{
	print_eps();
	return 0;
}

/**
* Open a manual page
*/
int man(lua_State * L)
{
	void *arg = 0;
	char cmd[255];

	if (lua_isstring(L, 1)) {
		arg = luaL_checkstring(L, 1);
		memset(cmd, 0x00, 255);
		snprintf(cmd, 254, "man %s", (char*) arg);	// Obvious injection. We don't care
		system(cmd);
	}
	return 0;
}


/**
* Display information on an object/memory address
*/
int info(lua_State * L)
{
	void *symbol = 0;
	unsigned long long int ret = 0;
	Dl_info dli;
	char *error = 0;
	Elf_Sym *s = 0;
	unsigned int stype = 0, sbind = 0;
	char *htype = 0, *hbind = 0;

	unsigned long long int n = lua_tonumber(L, 1);

	if(msync(n & ~0xfff, 4096, 0) == 0){	// if read as a number, destination address is mapped

		/**
		* Address is mapped
		*/
	        printf(" * address 0x%llx is mapped\n", n);

		/**
		* Search corresponding symbols
		*/
		symbols_t *sym = symbol_from_addr(n);
		if((sym)&&(sym->addr == n)){
			printf(" * %s %s %s from %s\tsize:%lu\n", sym->hbind, sym->htype, sym->symbol, sym->libname, sym->size);
		}else if(sym){
			printf(" * %llu bytes within %s %s %s from %s\tsize:%lu\n", n - sym->addr, sym->hbind, sym->htype, sym->symbol, sym->libname, sym->size);
		}

		/**
		* Search corresponding section
		*/
		sections_t *sec = section_from_addr(n);
		if(sec){
			printf(" * %llu bytes within %s:%s  %s\n", n - sec->addr, sec->libname, sec->name, sec->perms);
		}

		/**
		* Search corresponding segment
		*/
		sections_t *seg = segment_from_addr(n);
		if(seg){
			printf(" * %llu bytes within %s:%s  %s\n", n - seg->addr, seg->libname, seg->name, seg->perms);
		}

	}else if (lua_isstring(L, 1)) {
		symbol = luaL_checkstring(L, 1);

		/**
		* Search corresponding symbols
		*/
		symbols_t *sym = symbol_from_name(symbol);
		if(!sym){
			printf(" * Symbol %s does not exist\n", (char *)symbol);
			return 0;
		}

		/**
		* Resolve symbol...
		*/
		ret = (unsigned long int) dlsym(wsh->mainhandle, symbol);
		if ((error = dlerror()) != NULL) {
			fprintf(stderr, "ERROR: %s\n", error);
			return 0;
		}

		if (dladdr1(ret, &dli, (void **) &s, RTLD_DL_SYMENT)&&(s)) {
			stype = ELF_ST_TYPE(s->st_info);
			htype = symbol_totype(stype);

			sbind = ELF_ST_BIND(s->st_info);
			hbind = symbol_tobind(sbind);

			char *secname = "";
			sections_t *sec = section_from_addr(dli.dli_saddr);
			if(sec){
				secname = sec->name;
			}

			printf(" * %s %s %s at %p	%s:%s size:%lu\n", htype, hbind, dli.dli_sname, dli.dli_saddr, dli.dli_fname, secname, s->st_size /*, s->st_value */ );

		} else {
			printf(" * symbol %s does not exist.\n", (char *)symbol);
		}
	} else {
		printf(" !! ERROR: info requires a string argument\n");
	}

	return 0;
}

/**
* Buffer management subroutines
*/

// allocate a char **
int alloccharbuf(lua_State * L)
{
	int n = 0;
	char *ptr = 0;

	n = lua_tonumber(L, 1);
	ptr = calloc(n * sizeof(char *), 1);
	lua_pushnumber(L, (unsigned long int) ptr);
	return 1;
}

// set a pointer within the char **
int setcharbuf(lua_State * L)
{
	char **buff = 0;
	unsigned int pos = 0;
	char *val = 0;

	buff = (unsigned long int) lua_tonumber(L, 1);
	pos = lua_tonumber(L, 2);
	val = lua_tostring(L, 3);

	buff[pos] = val;
	return 0;
}

/**
* Read a string (to a LUA string)
*/
int rdstr(lua_State * L)
{
	char *buff = 0;
	unsigned int n = 0;
	char *val = 0;

	buff = (unsigned long int) lua_tonumber(L, 1);
	n = lua_tonumber(L, 2);
	val = buff;
	if(n){
		lua_pushlstring(L, val, n);	// Push string with length specifier
	}else{
		lua_pushstring(L, val);	// Ascii : push string (no length)
	}
	return 1;
}

/**
* Read a number (to a LUA number)
*/
int rdnum(lua_State * L)
{
	int *buff = 0;
	int val = 0;

	buff = (int) lua_tonumber(L, 1);
	val = buff[0];

	lua_pushnumber(L, (int) val);

	return 1;
}

// read a pointer within the char **
int getcharbuf(lua_State * L)
{
	char **buff = 0;
	unsigned int pos = 0;
	char *val = 0;

	buff = (unsigned long int) lua_tonumber(L, 1);
	pos = lua_tonumber(L, 2);

	val = buff[pos];
	lua_pushstring(L, val);	// Ascii : push string
	return 1;
}
/*
int luaopen_array(lua_State * L)
{
	lua_getglobal(L, "array");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
	}
	luaL_setfuncs(L, arraylib, 0);
	lua_setglobal(L, "array");

	return 1;
}
*/

/**
* Run minimal lua shell
*/
int run_shell(lua_State * L)
{
	char *input, shell_prompt[4096];

	if (wsh->is_stdinscript) {	// Execute from stdin. don't display prompt, read line by line
		for (;;) {
			if (fgets(shell_prompt, sizeof(shell_prompt), stdin) == 0 || strcmp(shell_prompt, "cont\n") == 0){
				return 0;
			}

			if (luaL_loadbuffer(wsh->L, shell_prompt, strlen(shell_prompt), "=(shell)") || lua_pcall(L, 0, 0, 0)) {
				fprintf(stderr, "ERROR: %s\n", lua_tostring(L, -1));
				lua_pop(L, 1);	// pop error message from the stack 
			}
			lua_settop(L, 0);	// remove eventual return values
		}
	} else {
		/**
		* Set handlers for tab completion
		*/
		linenoiseSetCompletionCallback(completion);

		/**
		* Prepare history full log name
		*/
		char *SHELL_HISTORY = calloc(1024, 1);
		snprintf(SHELL_HISTORY, 1023, "%s/%s", getenv("HOME"), SHELL_HISTORY_NAME);

		/**
		* Load shell history
		*/
		linenoiseHistoryLoad(SHELL_HISTORY);

		/**
		* Main loop
		*/
		snprintf(shell_prompt, sizeof(shell_prompt), "> ");

		while ((input = linenoise(shell_prompt)) != NULL) {

			/**
			* Command analysis/execution
			*/

			if (wsh->opt_hollywood == 2) {
				printf(GREEN);
			}
			// Check for EOF.
			if (!input) {
				break;
			}

			linenoiseHistoryAdd(input);	// Add to the history. 
			linenoiseHistorySave(SHELL_HISTORY);	// Save the history on disk. 

			if ((strlen(input) == 5) && (!strncmp(input, "shell", 5))) {
				unsigned int pid = fork();
				int status;
				if (!pid) {
					execlp("/bin/sh", "sh", NULL);
				} else {
					waitpid(pid, &status, 0);
				}
				free(input);
				continue;
			}
			if (!strncmp(input, "exec ", 5)) {
				system(input + 5);
				free(input);
				continue;
			}
			if ((strlen(input) == 4) && !strncmp(input, "quit", strlen(input))) {
				free(input);
				_Exit(EXIT_SUCCESS);
			}
			if ((strlen(input) == 4) && !strncmp(input, "exit", strlen(input))) {
				free(input);
				_Exit(EXIT_SUCCESS);
			}
			if ((strlen(input) == 4) && !strncmp(input, "help", strlen(input))) {
				help(L);
				continue;
			}

			if ((strlen(input) == 5) && !strncmp(input, "clear", strlen(input))) {
				printf(CLEAR);
				continue;
			}

			if (!strncmp(input, "historylen", 10)) {
				// The "historylen" command will change the history len. 
				int len = atoi(input + 10);
				linenoiseHistorySetMaxLen(len);
				continue;
			}

			if (luaL_loadbuffer(L, input, strlen(input), "=INVALID COMMAND ") || lua_pcall(L, 0, 0, 0)) {
				fprintf(stderr, "ERROR: %s\n", lua_tostring(L, -1));
				lua_pop(L, 1);	// pop error message from the stack 
			}
			lua_settop(L, 0);	// remove eventual return values

			free(input);
		}
		free(SHELL_HISTORY);
	}

	_Exit(EXIT_SUCCESS);
	// never reached
	return 0;
}


int learn_proto(unsigned long*arg, unsigned long int faultaddr, int reason)
{
	char *vreason = 0;
	char *tag = 0;
	long int offset = 0;
	unsigned int i = 0;
	unsigned int argn = 0;
	symbols_t *s = 0;

	if(!reason) { return 0; }		// No error
	if(!faultaddr) { return 0; }		// No Address
	if(faultaddr < 0x1000) { return 0; }	// Address in first page
	if(faultaddr > 0xf000000000000000) { return 0; }	// Address out of userland
	switch(reason){
	case 1:	// read
		vreason = "read";
		tag = "_input_ptr";
		break;
	case 2: // write
		vreason = "write";
		tag = "_output_ptr";
		break;
	case 4: // Exec
		vreason = "exec";
		tag = "_exec_ptr";
		break;
	default:
		return 0;
	}

	for(i=1; i<=7; i++){
		if((faultaddr & ~0xfff) == (arg[i] & ~0xfff)){ argn = i; }
	}

	if(!argn){ return 0; }	// Can't match fault address with any argument

	for(i=1; i<=7; i++){
		if((arg[i] == arg[argn])&&(argn != i)){ return 0; }	// 2 arguments in same page, can't conclude
	}

	offset = faultaddr - arg[argn];

	if(arg[argn] == 0xffff){ return 0; }
	if(arg[argn] == 0x7fff){ return 0; }
	if(arg[argn] == 0xffffffff){ return 0; }
	if(arg[argn] == 0x7fffffff){ return 0; }

	s = symbol_from_addr(arg[0]);

	if(!wsh->learnfile){
		wsh->learnfile = fopen( wsh->learnlog ? wsh->learnlog : DEFAULT_LEARN_FILE ,"a+");
	}

	fprintf(wsh->learnfile, "TAG %s %s argument%u %s %ld\n", s->libname, s->symbol, argn, tag, offset);
	fflush(wsh->learnfile);

	return 0;
}

int sort_learnt(learn_t *a, learn_t *b)
{
	return memcmp(&a->key, &b->key, sizeof(learn_key_t));
}

/**
* Display learned prototypes
*/
int prototypes(lua_State * L)
{
	char *pattern = 0;
	char *patternlib = 0;
	char *patterntag = 0;
	char line[1024];
	learn_t *l = 0, *p = 0;

	read_arg1(pattern);
	read_arg2(patternlib);
	read_arg3(patterntag);

	if(!wsh->learnfile){
		wsh->learnfile = fopen( wsh->learnlog ? wsh->learnlog : DEFAULT_LEARN_FILE ,"a+");
	}

	fseek(wsh->learnfile, 0, SEEK_SET);

	/**
	* Read all the lines to learnt data structure
	*/
	while (fgets(line, sizeof(line), wsh->learnfile)) {
		l = (learn_t*) calloc(1,sizeof(learn_t));

		sscanf(line, "%10s %200s %200s %20s %200s %20s", l->key.ttype, l->key.tlib, l->key.tfunction, l->key.targ, l->key.tvalue, l->toffset);

		// make sure tag type is correct, else discard
		if(strncmp(l->key.ttype, "TAG", 3)){
			printf(" !! Unknown TAG type: %s\n", l->key.ttype);
			free(l);
			continue;
		}

		// add to linked list if not present, else free
		HASH_FIND(hh, protorecords, &l->key, sizeof(learn_key_t), p);
		if(p){
			free(l);
		}else{
			HASH_ADD(hh, protorecords, key, sizeof(learn_key_t), l);
		}
	}

	/**
	* Sort learnt data structures
	*/
	HASH_SRT(hh, protorecords, sort_learnt);

	printf("\n [*] Prototypes: (from %u tag informations)\n", HASH_COUNT(protorecords));
	HASH_ITER(hh, protorecords, l, p) {
		if((!patternlib) || (strstr(l->key.tlib, patternlib))){
			if((!pattern) || (!strncmp(pattern, l->key.tfunction, strlen(pattern)))){
				if((!patterntag) || (strstr(l->key.tvalue, patterntag))){
					printf("%s\t%s\t%s\t%s\t%s\n", l->key.tlib, l->key.tfunction, l->key.targ, l->key.tvalue, l->toffset);
				}
			}
		}
	}
	return 0;
}

/*
void enable_trace(void){
	if(prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0)){	// Anyone can trace us
		printf(" !! ERROR: prctl() %s\n", strerror(errno));
		return;
	}
}

int pause_on(int syscall_req, int syscall) {
    if(syscall == syscall_req)
        do {
            char buf[2];
            fgets(buf, sizeof(buf), stdin); // waits until enter to continue
        } while(0);
}

#include "reversetrace/syscalls.h"
#include "reversetrace/syscallents.h"

const char *syscall_name(int scn) {
    struct syscall_entry *ent;
    static char buf[128];
    if (scn <= MAX_SYSCALL_NUM) {
        ent = &syscalls[scn];
        if (ent->name)
            return ent->name;
    }
    snprintf(buf, sizeof buf, "sys_%d", scn);
    return buf;
}

long get_register(pid_t child, int off) {
    return ptrace(PTRACE_PEEKUSER, child, 8*off, NULL);
}

long get_syscall_arg(pid_t child, int which) {
    switch (which) {
#ifdef __amd64__
    case 0: return get_register(child, rdi);
    case 1: return get_register(child, rsi);
    case 2: return get_register(child, rdx);
    case 3: return get_register(child, r10);
    case 4: return get_register(child, r8);
    case 5: return get_register(child, r9);
#else
    case 0: return get_register(child, ebx);
    case 1: return get_register(child, ecx);
    case 2: return get_register(child, edx);
    case 3: return get_register(child, esi);
    case 4: return get_register(child, edi);
    case 5: return get_register(child, ebp);
#endif
    default: return -1L;
    }
}

int do_tracer(pid_t child, int syscall_req) {
    int status;
    int retval;

	usleep(100); // Let the child give us permission to trace it via prctl()
	if (ptrace(PTRACE_ATTACH, child, (void *)0L, (void *)0L)) {
	    fprintf(stderr, "Cannot attach to TID %d: %s.\n", child, strerror(errno));
	    return 1;
	}


    waitpid(child, &status, 0);
//	printf("STOPPED\n");

    ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
    while(1) {
        if (wait_for_syscall(child) != 0){
            break;
	}
        print_syscall(child, syscall_req);

        if (wait_for_syscall(child) != 0){
            break;
	}

        retval = get_register(child, eax);

	if((retval >= 0)&&(retval <= 4096)){
	        fprintf(stderr, "%d\n", retval);
	} else if((retval <= -1)&&(retval >= -128)){
	        fprintf(stderr, "%d\n", retval);
	}else{
	        fprintf(stderr, "0x%x\n", retval);
	}
    }
    return 0;
}

int wait_for_syscall(pid_t child) {
    int status;
    while (1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80){
            return 0;
	}
        if (WIFEXITED(status)){
            return 1;
	}
        fprintf(stderr, "[stopped %d (%x)]\n", status, WSTOPSIG(status));
    }
}

void print_syscall_args(pid_t child, int num) {
    struct syscall_entry *ent = NULL;
    int nargs = SYSCALL_MAXARGS;
    int i;
    char *strval;

    if (num <= MAX_SYSCALL_NUM && syscalls[num].name) {
        ent = &syscalls[num];
        nargs = ent->nargs;
    }
    for (i = 0; i < nargs; i++) {
        long arg = get_syscall_arg(child, i);
        fprintf(stderr, "0x%lx", arg);

        if (i != nargs - 1)
            fprintf(stderr, ", ");
    }
}

void print_syscall(pid_t child, int syscall_req) {
    int num;
    num = get_register(child, orig_eax);

    fprintf(stderr, "%s(", syscall_name(num));
    print_syscall_args(child, num);
    fprintf(stderr, ") = ");

    if( syscall_req <= MAX_SYSCALL_NUM) {
        pause_on( num, syscall_req);
    }
}
*/

#ifdef HAS_ASAN
ATTRIBUTE_NO_SANITIZE_ADDRESS
size_t mystrlen(char *ptr)
{
	unsigned int ret = 0;

	while(ptr[ret] != 0){ ret++;}

	return ret;
}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void *mymemset(void *s, int c, size_t n)
{
	unsigned int i = 0;
	char *ptr = 0;

	ptr = s;
	for(i = 0; i < n ; i++){
		ptr[i] = c;
	}

	return s;
}
#else
#define mystrlen strlen
#define mymemset memset
#endif

/**
* Main wrapper around a library call.
* This function returns 9 values: ret (returned by library call), errno, firstsignal, total number of signals, firstsicode, firsterrno, faultaddr, reason, context
*/
static int libcall(lua_State * L)
{
	unsigned long int *arg[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned int i = 0;
	void *(*f) () = 0;
	void *ret = 0;
	int callerrno = 0;
	int argnum = 0;

	alarm(3);

	for (i = 0; i < 10; i++) {
		if (lua_isnil(L, i + 1)) {
			arg[i] = 0;
		} else if (lua_isnumber(L, i + 1)) {
			arg[i] = (unsigned long) lua_tonumber(L, i + 1);
		} else if (lua_isstring(L, i + 1)) {
			arg[i] = luaL_checkstring(L, i + 1);
		} else if (lua_istable(L, i + 1)) {
		} else if (lua_isfunction(L, i + 1)) {
			arg[i] = lua_tocfunction(L, i + 1);
		} else if (lua_iscfunction(L, i + 1)) {
			arg[i] = lua_touserdata(L, i + 1);
		} else if (lua_isuserdata(L, i + 1)) {
			arg[i] = lua_touserdata(L, i + 1);
		} else {
			arg[i] = 0;
		}
	}

	wsh->firstsignal = 0;
	wsh->firstsicode = 0;
	wsh->totsignals = 0;
	errno = 0;
	wsh->btcaller = 0;
	wsh->firsterrno = 0;
	wsh->faultaddr = 0;
	wsh->reason = 0;

	if (!wsh->errcontext) {
		wsh->errcontext = calloc(1, sizeof(ucontext_t));
	}
//	if (!wsh->initcontext) {
//		wsh->initcontext = calloc(1, sizeof(ucontext_t));
//	}

	memset(wsh->errcontext, 0x00, sizeof(ucontext_t));

//	save_context(wsh->initcontext);	// this is saved to initcontext

	/**
	* Handle (reverse-) system calls tracing
	*/
/*	pid_t parent = 0;
	pid_t child = 0;
	if((wsh->trace_strace)||(wsh->trace_rtrace)){
		enable_trace();
		parent = getpid();
		child = fork();
		enable_trace();

		if (child == 0) {	// child process
			if(wsh->trace_rtrace){
			        return do_tracer(parent, syscall);
			}else{
//			        return do_tracee(argc-push, argv+push);
				printf(" -- tracee pid:%d\n", getpid());
//				kill(getpid(), SIGSTOP);
				goto do_tracee;
			}
		} else {		// parent process
			if(wsh->trace_rtrace){
//			        return do_tracee(argc-push-1, argv+push+1);
				printf(" -- tracee2 pid:%d\n", getpid());
//				kill(getpid(), SIGSTOP);
				goto do_tracee;
			}else{
		        	return do_tracer(child, syscall);
			}
		}
	}
*/
//do_tracee:
	/**
	* Make the library call
	*/
	f = arg[0];
	wsh->interrupted = 0;

	if (!sigsetjmp(wsh->longjmp_ptr, 1)){ // This is executed only the first time // save stack context + signals

		// Set align flag
		if(wsh->trace_unaligned){
			wsh->sigbus_count = 0;
			wsh->sigbus_hash = 0;
			set_align_flag();
		}

		// Set trace flag
		if(wsh->trace_singlestep){
			set_trace_flag();
			wsh->singlestep_count = 0;
			wsh->singlestep_hash = 0;
		}

		// Set branch flag
		if(wsh->trace_singlebranch){
			set_branch_flag();
			wsh->singlebranch_count = 0;
			wsh->singlebranch_hash = 0;
			set_trace_flag();
		}

		ret = f(arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8]);
	}else{
//		printf(" + Restored shell execution\n");
		ret = -1;
	}
	unsigned int n = 0, j = 0, notascii = 0;

	// Unset trace flag
	if(wsh->trace_singlestep){
		unset_trace_flag();

		printf("Total: %u instructions traced\n", wsh->singlestep_count);
		printf("Execution hash: ss:%016llx\n", wsh->singlestep_hash);
	}

	// Unset branch flag
	if(wsh->trace_singlebranch){
		unset_trace_flag();
		unset_branch_flag();

		printf("Total: %u blocks traced\n", wsh->singlebranch_count);
		printf("Execution hash: b:%016llx\n", wsh->singlebranch_hash);
	}

	// Unset align flag
	if(wsh->trace_unaligned){
		unset_align_flag();

		printf("Total: %u misaligned access traced\n", wsh->sigbus_count);
		printf("Execution hash: u:%016llx\n", wsh->sigbus_hash);
	}

	callerrno = errno;

	/**
	* Analyse return value
	*/
	n = !msync((long int)ret & (long int)~0xfff, 4096, 0);
	notascii = 0;
	if (n) {
		//      printf("mapped: %p (len:%u)\n", ret, n);
		//      hexdump(ret, n);

		// is it a string ?
		char *ptr = ret;
		for (j = 0; j < mystrlen(ret); j++) {
			if (!isascii((ptr[j]))) {
				notascii = 1;
				break;
			}
		}

#ifdef HAS_ASAN
		if ((!notascii)&&(mystrlen(ret) >= 1)) {
#else
		if (!notascii) {
#endif
			lua_pushstring(L, ret);	// Ascii : push string
		} else {
			lua_pushinteger(L, ret);// Push as a number
		}
	} else {
		lua_pushinteger(L, ret);	// Push as a number
	}

	if (callerrno) {
		fprintf(stderr, "ERROR: %s (%u)\n", strerror(callerrno), callerrno);
	}

	/**
	* Learn prototypes
	*/
	if((wsh->reason)&&(wsh->faultaddr >= 0x40000000)&&(wsh->faultaddr <= 0x7f0000000000)){
		learn_proto(arg, wsh->faultaddr, wsh->reason);
	}

	/**
	*
	* Create output execution context table
	*
	*/
	/* create result table */
	lua_newtable(L);

	/**
	* Push errno to lua table
	*/
	lua_pushstring(L, "errno");		/* push key */
	lua_pushinteger(L, callerrno);
        lua_settable(L, -3);

	/**
	* Push strerror(errno) to lua table
	*/
	lua_pushstring(L, "errnostr");		/* push key */
	lua_pushstring(L, strerror(callerrno));		/* push key */
        lua_settable(L, -3);

	/**
	* Push first signal
	*/
//	lua_pushstring(L, "fsig");		/* push key */
//	lua_pushinteger(L, wsh->firstsignal);
//        lua_settable(L, -3);

	/**
	* Push first signal name
	*/
	lua_pushstring(L, "signal");		/* push key */
	lua_pushstring(L, wsh->firstsignal ? signaltoname(wsh->firstsignal) : "");
        lua_settable(L, -3);

	/**
	* Push total of signals emmited during this libcall
	*/
//	lua_pushstring(L, "nsignals");		/* push key */
//	lua_pushinteger(L, wsh->totsignals);
//        lua_settable(L, -3);

	/**
	* Push first errno
	*/
//	lua_pushstring(L, "ferrno");		/* push key */
//	lua_pushinteger(L, wsh->firsterrno);
//        lua_settable(L, -3);

	/**
	* Push first sicode
	*/
//	lua_pushstring(L, "fcode");		/* push key */
//	lua_pushinteger(L, wsh->firstsicode);
//        lua_settable(L, -3);

	/**
	* Push first sicode name
	*/
	lua_pushstring(L, "sicode");		/* push key */
//	lua_pushinteger(L, wsh->firstsicode);
	siginfo_t *s;
	s = calloc(1, sizeof(siginfo_t));
	s->si_code = wsh->firstsicode;
	lua_pushstring(L, sicode_strerror(wsh->firstsignal, s));		/* push value */
	free(s);
        lua_settable(L, -3);

	/**
	* Address of last caller in backtrace
	*/
	symbols_t *symbt = symbol_from_addr(wsh->btcaller);
	if(symbt){
		lua_pushstring(L, "caller");		/* push key */
		lua_pushstring(L, symbt->symbol);
	        lua_settable(L, -3);
	}

//	if(wsh->btcaller){
		lua_pushstring(L, "calleraddr");		// push key 
//		lua_pushlightuserdata(L, wsh->btcaller);
		lua_pushinteger(L, wsh->btcaller);
		lua_settable(L, -3);
//	}

	/**
	* Push fault address
	*/
	lua_pushstring(L, "faultaddr");		/* push key */
//	if(wsh->faultaddr){
//		lua_pushlightuserdata(L, wsh->faultaddr);
//	}else{
//		lua_pushinteger(L, 0);
//	}
	lua_pushinteger(L, wsh->faultaddr);
        lua_settable(L, -3);

	/**
	* Push reason
	*/
//	lua_pushstring(L, "reason");		/* push key */
//	lua_pushinteger(L, wsh->reason);
//        lua_settable(L, -3);


	/**
	* Push mode
	*/
	lua_pushstring(L, "mode");		/* push key */
	switch(wsh->reason){
	case 1:
		lua_pushstring(L, "READ");		/* push value */
		break;
	case 2:
		lua_pushstring(L, "WRITE");		/* push value */
		break;
	case 4:
		lua_pushstring(L, "EXEC");		/* push value */
		break;
	case 0:
	default:
		lua_pushinteger(L, wsh->reason);	/* push value */
		break;
	}	
        lua_settable(L, -3);

	/** 
	* Push errctx
	*/
//	lua_pushstring(L, "reg");		/* push key */
//	lua_pushinteger(L, wsh->totsignals ? wsh->errcontext : 0);
//        lua_settable(L, -3);

	/**
	* Push pointer to ucontext
	*/
//	lua_pushstring(L, "errctx");		/* push key */
//	lua_pushinteger(L, wsh->errcontext);
//        lua_settable(L, -3);


	/**
	* Push arguments as a new table
	*/

	lua_pushstring(L, "arg");		/* push key */
	lua_createtable(L,6,0);

	argnum = 0;
	for(j=1;j<=6;j++){
		char argname[10];
		memset(argname, 0x00, 10);
		snprintf(argname, 9, "arg%u", j);
		if(arg[j]){
			lua_pushstring(L, argname);		/* push key */
			lua_pushinteger(L, arg[j]);
			lua_settable(L, -3);
			argnum++;
		}
	}

        lua_settable(L, -3);


	/**
	* Push number of non NULL arguments
	*/
	lua_pushstring(L, "argnum");		/* push key */
	lua_pushinteger(L, argnum);
	lua_settable(L, -3);


	/**
	* Push retval
	*/
	lua_pushstring(L, "retval");		/* push key */
	if (n) {
		// is it a string ?
		char *ptr = ret;
		for (j = 0; j < strlen(ret); j++) {
			if (!isascii((ptr[j]))) {
				notascii = 1;
				break;
			}
		}

		if (!notascii) {
			lua_pushstring(L, ret);	// Ascii : push string
		} else {
			lua_pushinteger(L, ret);	// Push as a number
		}
	} else {
		lua_pushinteger(L, ret);	// Push as a number
	}
        lua_settable(L, -3);

	/**
	* Push libcall/libname
	*/
//	lua_pushstring(L,"caller");		/* key */
//	lua_pushinteger(L, arg[0]);
//	lua_settable(L, -3);

	symbols_t *symlib = symbol_from_addr(arg[0]);
	if(symlib){
		lua_pushstring(L,"alibcall");		/* key */
		lua_pushstring(L, symlib->symbol);
		lua_settable(L, -3);

		lua_pushstring(L,"alibname");		/* key */
		lua_pushstring(L, symlib->libname);
		lua_settable(L, -3);
	}



//* This function returns 9 values: ret (returned by library call), errno, firstsignal, total number of signals, firstsicode, firsterrno, faultaddr, reason, context


	/**
	* Invoke store running function on context
	*/

	lua_getglobal(L, "storerun");
	lua_pushvalue(L,-2);
	if(lua_pcall(L, 1, 1, 0)){
		if(wsh->opt_verbose){
			printf("ERROR: calling function storerun() in libcall() %s\n", lua_tostring(L, -1));
		}
	}
	lua_pop(L,1);

	alarm(0);

	return 2;
}

/**
* Append a command to internal lua buffer
*/
int luabuff_append(char *cmd){

	/**
	* Allocate wsh->luabuff if it hasn't been initialized
	*/
	if(!wsh->luabuff){
		wsh->luabuff = calloc(1, 4096);
		wsh->luabuffsz = 4096;
	}

	/**
	* Extend wsh->luabuff by one page if necessary
	*/
	if(strlen(wsh->luabuff) + strlen(cmd) >= wsh->luabuffsz){
		wsh->luabuff = realloc(wsh->luabuff, wsh->luabuffsz + 4096);
		wsh->luabuffsz += 4096;
	}

	/**
	* Append buffer
	*/
	strcat(wsh->luabuff, cmd);

//printf("Appending %s\n", cmd);
	return 0;
}

void scan_syms(char *dynstr, Elf_Sym * sym, unsigned long int sz, char *libname)
{
	unsigned int cnt = 0;
	char *htype = 0;
	unsigned long int address = 0;
	char *demangled = 0, *symname = 0;
	unsigned int func = 0;
	unsigned int j = 0;
	unsigned skip_bl = 0;
	char newname[1024];

	/**
	* Walk symbol table
	*/
	while ((sym)&&(!msync((long unsigned int)sym &~0xfff,4096,0))) {
		func = 0;
		if (sym->st_name >= sz) {
			break;
		}
		cnt++;
		symname = dynstr + sym->st_name;

		/**
		* Extract Symbol type
		*/
		switch (ELF_ST_TYPE(sym->st_info)) {
		case STT_GNU_IFUNC:	// Indirect function call (architecture dependant implementation/optimization)
		case STT_FUNC:
			htype = "Function";
			func = 1;
			break;
		case STT_OBJECT:
			htype = "Object";
			break;
		case STT_SECTION:
			htype = "Section";
			break;
		case STT_FILE:
			htype = "File";
			break;
		case STT_NOTYPE:
		case STT_NUM:
		case STT_LOPROC:
		case STT_HIPROC:
		default:
			htype = 0;
			break;
		}

		/**
		* Resolve address
		*/
		if (symname) {
			address = resolve_addr(symname, libname);
		} else {
			address = (unsigned long int) -1;
		}

		/**
		* Demangle symbol if possible
		*/
		demangled = cplus_demangle(symname, DMGL_ANSI/*DMGL_PARAMS*/);

		// Skip if symbol has no name or no type
		if (strlen(symname) && (htype) && (address != (unsigned long int) -1) && (address)) {

			/**
			* If function name is blackslisted, skip...
			*/
			skip_bl = 0;

			// Lua blacklist
			for(j=0; j < sizeof(lua_blacklist)/sizeof(char*);j++){
				if((strlen(symname) == strlen(lua_blacklist[j]))&&(!strncmp(lua_blacklist[j] ,symname, strlen(lua_blacklist[j])))){
					skip_bl = 1;
				}
			}

			// Lua default functions
			for(j=0; j < sizeof(lua_default_functions)/sizeof(char*);j++){
				if((strlen(symname) == strlen(lua_default_functions[j]))&&(!strncmp(lua_default_functions[j] ,symname, strlen(lua_default_functions[j])))){
					skip_bl = 1;
				}
			}
			
			if(skip_bl){
#ifdef DEBUG
				printf(" * blacklisted function name: %s\n", symname);
#endif
			} else if (func) {
				/*
				* Make C function available from lua
				*/
				memset(newname, 0x00, 1024);
				snprintf(newname, 1023, "reflect_%s", symname);
				lua_pushcfunction(wsh->L, (void *) address);
				lua_setglobal(wsh->L, newname);


				/**
				* Create a wrapper function with the original name
				*/
				char *luacmd = calloc(1, 2048);
				snprintf(luacmd, 2047, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", symname, newname);
				luabuff_append(luacmd);
				free(luacmd);
				// Add function/object to linked list
				scan_symbol(symname, libname);

				/**
				* Handle demangled symbols
				*/
/*
				if(demangled){
					printf(" -- demangled: %s\n", demangled);
					luacmd = calloc(1, 1024);
					snprintf(luacmd,1023, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", demangled, newname);
					luabuff_append(luacmd);
					free(luacmd);
					// Add function/object to linked list
					scan_symbol(demangled, libname);

				}
*/

			} else {
				/**
				* Add function/object to linked list
				*/
				if((!scan_symbol(symname, libname))&&(msync(address &~0xfff,4096,0) == 0)) {	// no errors, mapped
					// Export global as a string of known size
					lua_pushlstring(wsh->L, (char *) address, sym->st_size);
					lua_setglobal(wsh->L, symname);
				}
			}
		}

		free(demangled);
		sym++;
	}
}

void parse_dyn(struct link_map *map)
{
	Elf_Dyn *dyn;
	unsigned int cnt = 0;
	unsigned int done = 0;

	char *dynstr = 0;
	Elf_Sym *dynsym = 0;
	unsigned int dynstrsz = 0;
//	char *sec_init = 0;
//	char *sec_fini = 0;
//	char *sec_initarray = 0;
//	unsigned long int sec_initarraysz = 0;
//	char *sec_finiarray = 0;
//	unsigned long int sec_finiarraysz = 0;

	dyn = map->l_ld;

	/**
	* Walk the array of ELF_Dyn once looking for critical sections
	*/
	while ((dyn) && (!done)) {
		cnt++;

		switch (dyn->d_tag) {

		case DT_NULL:
		case DT_NEEDED:
		case DT_HASH:
		case DT_RELA:
		case DT_RELASZ:
		case DT_RELAENT:
		case DT_SYMENT:
		case DT_SONAME:
		case DT_RPATH:
		case DT_SYMBOLIC:
		case DT_REL:
		case DT_RELSZ:
		case DT_RELENT:
		case DT_PLTREL:
		case DT_DEBUG:
		case DT_TEXTREL:
		case DT_JMPREL:
		case DT_NUM:
		case DT_LOPROC:
		case DT_HIPROC:
		case DT_PROCNUM:
		case DT_VERSYM:
		case DT_VERDEF:
		case DT_VERDEFNUM:
		case DT_VERNEED:
		case DT_VERNEEDNUM:
		case 0x6ffffef5:
			break;

		case DT_STRTAB:
			dynstr = (char *) dyn->d_un.d_val;
			break;
		case DT_SYMTAB:
			dynsym = (Elf_Sym *) dyn->d_un.d_val;
			break;
		case DT_STRSZ:
			dynstrsz = dyn->d_un.d_val;
			break;
		case DT_INIT:
//			sec_init = (char *) dyn->d_un.d_val;
			break;
		case DT_FINI:
//			sec_fini = (char *) dyn->d_un.d_val;
			break;
		case DT_INIT_ARRAY:
//			sec_initarray = (char *) dyn->d_un.d_val;
			break;
		case DT_INIT_ARRAYSZ:
//			sec_initarraysz = dyn->d_un.d_val;
			break;
		case DT_FINI_ARRAY:
//			sec_finiarray = (char *) dyn->d_un.d_val;
			break;
		case DT_FINI_ARRAYSZ:
//			sec_finiarraysz = dyn->d_un.d_val;
			break;

		case DT_PLTGOT:
//                      pltgot  = (void *) dyn->d_un.d_val;
			break;
		case DT_PLTRELSZ:
//                      pltsz  = dyn->d_un.d_val / 16;
			break;
		default:
			done = 1;
			break;
		}
		dyn += 1;
	}
	scan_syms(dynstr, dynsym, dynstrsz, map->l_name);
}


int parse_link_map_dyn(struct link_map *map)
{
	if (!map) {
		if(!wsh->opt_quiet){
			fprintf(stderr, "WARNING: No binary loaded in memory. Try loadbin(). For help type help(\"loadbin\").\n");
		}
		return -1;
	}

	// go to first in linked list...
	while ((map) && (map->l_prev)) {
		map = map->l_prev;
	}

	// skip first entries in an attempt to not display the libs we load for ourselves...

	if(!wsh->opt_appear){
		if (map->l_next) {	// Leave libdl.so apparent in verbose mode
			map = map->l_next;
		}
		if (map->l_next) {	// expose ourselved
			map = map->l_next;
		}
	}

	while (map) {
		parse_dyn(map);
		map = map->l_next;
	}

	return 0;
}

/**
* Execute internal lua buffer
*/
int exec_luabuff(void)
{
	int err = 0;

	if(wsh->luabuffsz == 0){ return 0; }

	/**
	* Load buffer in lua
	*/
	if ((err = luaL_loadbuffer(wsh->L, wsh->luabuff, strlen(wsh->luabuff), "=Wsh internal lua buffer")) != 0) {
		printf("ERROR: Wsh internal lua initialization (%s): %s\n", lua_strerror(err), lua_tostring(wsh->L, -1));
		fatal_error(wsh->L, "Wsh internal lua initialization failed");
	}

	/**
	* Execute buffer
	*/
	if(lua_pcall(wsh->L, 0, 0, 0)){
		fprintf(stderr, "ERROR: lua_pcall() failed with %s\n",lua_tostring(wsh->L, -1));
	}

	/**
	* Release internal lua buffer
	*/
//	printf(" -- Executing internal buffer:\n%s\n",wsh->luabuff);

	free(wsh->luabuff);
	wsh->luabuff = 0;
	wsh->luabuffsz = 0;

	return 0;
}

void parse_link_vdso(void)
{
	// add extra vdso
	struct link_map *vdso = 0;
	vdso = dlopen(EXTRA_VDSO, wsh->opt_global ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW);
	if(wsh->opt_verbose){ printf(" -- Adding extra (arch specific) vdso library: %s at %p\n", EXTRA_VDSO, vdso); }
	parse_link_map_dyn(vdso);
}

/**
* Rescan address space
*/
void rescan(void)
{
	reload_elfs();

	empty_symbols();
	wsh->opt_rescan = 1;
	parse_link_map_dyn(wsh->mainhandle);
	parse_link_vdso();
	wsh->opt_rescan = 0;
	exec_luabuff();
}

/**
* Display content of /proc/self/maps
*/
int print_procmap(unsigned int pid)
{
	char path[100];
	int n = 0;
	int fd = 0;
	char *buff = 0;

	memset(path, 0x00, sizeof(path));
	snprintf(path, 99, "/proc/%u/maps", pid);
	buff = calloc(1, 4096);
	fd = open(path, O_RDONLY);
	if(fd < 0){ printf(" !! ERROR: open %s : %s\n", path, strerror(errno)); return -1; }

	while ((n = read(fd, buff, 4096)) > 0){
		write(1, buff, n);
		memset(buff, 0x00, 4096);
	}
	free(buff);
	close(fd);

	return 0;
}

int procmap_lua(void)
{
	return print_procmap(getpid());
}

int execlib(lua_State * L)
{
	int ret = 0;
	int child = 0;
	int status = 0;
	int pid = 0;
	siginfo_t si;

	child = fork();
	if (child == 0) {	// child
		ptrace(PTRACE_TRACEME, 0, 0, 0);

		ret = libcall(L);
		_Exit(EXIT_SUCCESS);
	} else if (child == -1) {
		fprintf(stderr, "ERROR: fork() : %s\n", strerror(errno));
		_Exit(EXIT_FAILURE);
	} else {		// parent
		ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEVFORKDONE | PTRACE_O_TRACEEXIT);

		while (1) {
			if (waitpid(-1, &status, __WALL) == -1) {
				printf(" [*] traced process exited with status %d\n", WEXITSTATUS(status));
				_Exit(EXIT_FAILURE);
			}
			if (WIFSTOPPED(status)) {
				pid = child;	// save child's pid
				if (ptrace(PTRACE_CONT, child, 0, 0) == -1) {
					fprintf(stderr, "ERROR: ptrace() : %s\n", strerror(errno));
					_Exit(EXIT_FAILURE);
				}
				// check return signal/error code
				ptrace(PTRACE_GETSIGINFO, pid, NULL, &si);
				if (si.si_signo || si.si_errno || si.si_code) {
					printf("[*] Child stopped with signal: %i" " errno: %i code: %i\n", si.si_signo, si.si_errno, si.si_code);
					break;
				}
			}
		}
	}

	return 0;
}

int traceback(lua_State * L)
{
	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	printf("%s\n", lua_tostring(L, -1));

	return 1;
}

void print_backtrace(void)
{
	void *traceptrs[100];
	char **funcnames = 0;
	size_t count = 0;
	unsigned int i = 0;
	char *p = 0;

	count = backtrace(traceptrs, 100);
	funcnames = backtrace_symbols(traceptrs, count);
	if (count > SKIP_BOTTOM) {
		count -= SKIP_BOTTOM;
	}
	for (i = SKIP_INIT; i < count; i++) {
		// truncate at first space
		p = strchr(funcnames[i], 0x20);
		if (p) {
			p[0] = 0x00;
		}
		printf("\t%p    %s\n", traceptrs[i], funcnames[i]);
	}
	free(funcnames);
}

char *sicodetoname(int code)
{
	return "Unknown";
}

char *signaltoname(int signal)
{
	unsigned int i;
	for (i = 0; i < sizeof(signames) / sizeof(signame_t); i++) {
		if (signames[i].signal == signal) {
			return signames[i].name;
		}
	}

	return "Unknown Signal";
}


inline void unset_align_flag(void)
{
#ifdef __amd64__
	// Unset Align flag
	asm(".intel_syntax noprefix;"
		"pushf;"
		"pop rax;"
		"xor rax, 0x40000;"
		"push rax;"
		"popf;"
	);
#endif
}


inline void set_align_flag(void)
{
#ifdef __amd64__
	// Set Align flag
	asm(".intel_syntax noprefix;"
		"pushf;"
		"pop rax;"
		"or rax, 0x40000;"
		"push rax;"
		"popf;"
	);
#endif
}

inline void unset_trace_flag(void)
{
#ifdef __amd64__
	// Unset trace flag
	asm(".intel_syntax noprefix;"
		"pushf;"
		"pop rax;"
		"xor rax, 0x100;"
		"push rax;"
		"popf;"
	);
#endif
}


inline void set_trace_flag(void)
{
#ifdef __amd64__
	// Set Trace flag
	asm(".intel_syntax noprefix;"
		"pushf;"
		"pop rax;"
		"or rax, 0x100;"
		"push rax;"
		"popf;"
	);
#endif
}

/**
* Set affinity of a thread to a given CPU
*/
void affinity(int procnum)
{
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(procnum, &set);

        if (sched_setaffinity(getpid(), sizeof(set), &set) == -1){
		fprintf(stderr, " !! ERROR: sched_setaffinity(%u): %s\n", procnum, strerror(errno));
	}
}

/**
* Enable Branch Tracing
*/
void btr_enable(int procnum)
{
	char cpupath[200];
	uint64_t data = 0x02;
	int fd = 0, ret = 0;

	memset(cpupath, 0x00, 200);
	snprintf(cpupath, 199, "/dev/cpu/%d/msr", procnum);
	fd = open(cpupath, O_WRONLY);
	if(fd <= 0){ fprintf(stderr, "ERROR: open(%s): %s\n", cpupath,strerror(errno)); return; }
	ret = lseek(fd, 0x00, SEEK_SET);
	if(ret != 0x00){ fprintf(stderr, "ERROR: lseek(): %s\n", strerror(errno)); return; }
	ret = pwrite(fd, &data, sizeof(data), 0x1d9);
	if(ret != sizeof(data)){ fprintf(stderr, "ERROR: write(): %s\n", strerror(errno)); return; }
	ret = close(fd);
	if(ret != 0){ fprintf(stderr, "ERROR: close(): %s\n", strerror(errno)); return; }
}

/**
* Disable Branch Tracing
*/
void btr_disable(int procnum)
{
	char cpupath[200];
	uint64_t data = 0x00;
	int fd = 0, ret = 0;

	memset(cpupath, 0x00, 200);
	snprintf(cpupath, 199, "/dev/cpu/%d/msr", procnum);
	fd = open(cpupath, O_WRONLY);
	if(fd <= 0){ fprintf(stderr, "ERROR: open(%s): %s\n", cpupath,strerror(errno)); return; }
	ret = lseek(fd, 0x00, SEEK_SET);
	if(ret != 0x00){ fprintf(stderr, "ERROR: lseek(): %s\n", strerror(errno)); return; }
	ret = pwrite(fd, &data, sizeof(data), 0x1d9);
	if(ret != sizeof(data)){ fprintf(stderr, "ERROR: write(): %s\n", strerror(errno)); return; }
	ret = close(fd);
	if(ret != 0){ fprintf(stderr, "ERROR: close(): %s\n", strerror(errno)); return; }
}


inline void set_branch_flag(void)
{
/*
//
// The following code only works in ring0
//

	// Enable LBR
	asm(".intel_syntax noprefix;"
	"xor rdx, rdx;"
	"xor rax, rax;"
	"inc rax;"
	"mov rcx, 0x1d9;"
	"wrmsr;"
	);
*/

	// set affinity to processor MY_CPU
	affinity(MY_CPU);

	// enable Branch Tracing (BTR) via msr on processor MY_CPU
	btr_enable(MY_CPU);
}

inline void unset_branch_flag(void)
{

	// disable btranch tracing
	btr_disable(MY_CPU);
}

/**
* SIGBUS handler
*/
void bushandler(int signal, siginfo_t * s, void *ptr)
{
	ucontext_t *u = (ucontext_t *) ptr;

	unset_align_flag();
/*
//
// The faulty address is NOT passed on to the user via si->si_addr :
//
//http://lxr.free-electrons.com/source/arch/x86/kernel/traps.c#L217
//
//

	unsigned int fault = 0;
	char *hfault = "";

	if (u->uc_mcontext.gregs[REG_ERR] & 0x2) {
		fault = FAULT_WRITE;	// Write fault
		hfault = "WRITE";
	} else if (s->si_addr == u->uc_mcontext.gregs[REG_RIP]) {
		fault = FAULT_EXEC;	// Exec fault
		hfault = "EXEC";
	} else {
		fault = FAULT_READ;	// Read fault
		hfault = "READ";
	}

	printf(" -- SIGBUS: %llx\t%llx:%s\n", u->uc_mcontext.gregs[REG_RIP], s->si_addr, hfault);
*/

#ifndef __arm__
	if(wsh->trace_unaligned){
		if(wsh->opt_verbosetrace){
			symbols_t *s = symbol_from_addr(u->uc_mcontext.gregs[REG_RIP]);
			if(s){
				fprintf(stderr, " -- SIGBUS[%03u] %llx\t%s()+%llu\t%s\n", wsh->sigbus_count+1, u->uc_mcontext.gregs[REG_RIP], s->symbol, u->uc_mcontext.gregs[REG_RIP] - s->addr, s->libname);
			}else{
				fprintf(stderr, " -- SIGBUS[%03u] %llx\n", wsh->sigbus_count+1, u->uc_mcontext.gregs[REG_RIP]);
			}
		}

		wsh->sigbus_count++;

		wsh->sigbus_hash = (wsh->sigbus_hash >> 2) ^ (~u->uc_mcontext.gregs[REG_RIP]);

		u->uc_mcontext.gregs[REG_EFL] ^= 0x40000;	// Unset Align flag
		u->uc_mcontext.gregs[REG_EFL]|= 0x100;		// Set Trace flag
	}
#endif
}


void alarmhandler(int signal, siginfo_t * s, void *u)
{
	write(1, BLUE, strlen(BLUE));
	write(1, "\n[SIGALRM]\tTimeout",18);
	write(1, NORMAL, strlen(NORMAL));
	write(1, "\n", 1);
	errno = ECANCELED;
	alarm(1);
	longjmp(wsh->longjmp_ptr, 1);
}

void inthandler(int signal, siginfo_t * s, void *u)
{
	write(1, MAGENTA, strlen(MAGENTA));
	write(1, "\n[SIGINT]\tInterrupted",21);
	write(1, NORMAL, strlen(NORMAL));
	write(1, "\n", 1);
	errno = ECANCELED;
	if(wsh->interrupted++ < 2){
		alarm(1);
		longjmp(wsh->longjmp_ptr, 1);		// Soft interrupt
	}else{
		alarm(0);
		longjmp(wsh->longjmp_ptr_high, 1);	// Hard interrupt
	}
}

int mk_backtrace(void)
{
	void *bt[20];
	int bt_size;
	char **bt_syms;
	int i;

	bt_size = backtrace(bt, 20);
	bt_syms = backtrace_symbols(bt, bt_size);

	for (i = 2; i < bt_size; i++) {
		write(1, "    ", 4);
		write(1, bt_syms[i], strlen(bt_syms[i]));
		write(1, "\n", 1);
	}
	free(bt_syms);

	return 0;
}

/**
* generic function to restore from exit()
*/
void restore_exit(void)
{
	errno = ECANCELED;
	longjmp(wsh->longjmp_ptr, 1);
}

void exit(int status)
{
	fprintf(stderr, " + Called exit(%d), restoring...\n", status);
	restore_exit();
}

#ifndef __arm__
void _exit(int status)
{
	fprintf(stderr, " + Called _exit(%d), restoring...\n", status);
	restore_exit();
}
#endif

void exit_group(int status)
{
	fprintf(stderr, " + Called exit_group(%d), restoring...\n", status);
	restore_exit();
}


int printarg(unsigned long int val)
{
	if(msync(val &~0xfff,4096,0) == 0){ // Mapped
		int nlen = 0, noflag = 0, k = 0;
		char *ptrx = 0;
		noflag = 0;
		ptrx = val;
		nlen = strnlen(ptrx, 4096 - ((unsigned long int)ptrx & ~0xfff));
		if(nlen){
			for(k=0;k<nlen;k++){ if(!isprint(ptrx[k] & 0xff)){ /*printf(" not printable: ptrx[k] = %02x\n", ptrx[k] & 0xff);*/ ;noflag = 1; }; }
		}
		if(!noflag){
			fprintf(stderr,"\"%s\"", ptrx);
		}else{
			fprintf(stderr,"0x%lx", val);
		}
	}else{
		fprintf(stderr,"0x%lx", val);
	}

	return 0;
}

void traphandler(int signal, siginfo_t * s, void *ptr)
{
	unsigned int i = 0;
	char *ptrd = 0x00;
	ucontext_t *u = 0;
	unsigned int fault = 0;
	char *hfault = 0;
	char *signame = 0;

	u = (ucontext_t *) ptr;

	if(wsh->trace_singlebranch){	// Stop tracing ourselves
		unset_branch_flag();
//		u->uc_mcontext.gregs[REG_EFL] ^= 0x100;		// Set Trace flag
	}

#ifndef __arm__
	/**
	* Search corresponding Breakpoint
	*/
	for (i = 0; i < wsh->bp_num; i++) {
		if (wsh->bp_array[i].ptr == (char*)(u->uc_mcontext.gregs[REG_RIP] - 1)) {
			printf(" ** EXECUTED BREAKPOINT[%u] at %llx	weight:%u	<", i + 1, u->uc_mcontext.gregs[REG_RIP] - 1, wsh->bp_array[i].weight);
			info_function(u->uc_mcontext.gregs[REG_RIP] - 1);
			ptrd = u->uc_mcontext.gregs[REG_RIP] - 1;
			ptrd[0] = wsh->bp_array[i].backup;
			wsh->bp_points += wsh->bp_array[i].weight;

			// Update bp_points
			lua_pushnumber(wsh->L, wsh->bp_points);
			lua_setglobal(wsh->L, "bp_points");

		}
	}

	if (ptrd) {
		/**
		* This is a breakpoint
		*/

		printf(" ** Restoring execution from  %p\n", ptrd);
		u->uc_mcontext.gregs[REG_RIP]--;	// TODO : decrease by full instruction size

	} else if(wsh->trace_singlebranch) {
		/**
		* We are single branching
		*/

		if((u->uc_mcontext.gregs[REG_RIP] & ~0xffffff) != ((long long int)traphandler & ~0xffffff)){	// Make sure we are not tracing ourselves
			if(wsh->opt_verbosetrace){
				symbols_t *s = symbol_from_addr(u->uc_mcontext.gregs[REG_RIP]);
				if((s)&&(u->uc_mcontext.gregs[REG_RIP] == (long long int)s->addr)){
					fprintf(stderr, " -- Branch[%03d] = 0x%llx\t%s(", wsh->singlebranch_count + 1, u->uc_mcontext.gregs[REG_RIP], s->symbol);
#ifdef DEBUG
#ifdef __amd64__
					printarg(u->uc_mcontext.gregs[REG_RDI]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_RSI]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_RDX]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_RCX]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_R8]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_R9]);
#endif
#endif

					fprintf(stderr, ")\t%s\n", s->libname);
				}else if(s){
					fprintf(stderr, " -- Branch[%03d] = 0x%llx\t%s()+%llu\t%s\n", wsh->singlebranch_count + 1, u->uc_mcontext.gregs[REG_RIP], s->symbol,
						u->uc_mcontext.gregs[REG_RIP] - s->addr, s->libname);
				}else{
					fprintf(stderr, " -- Branch[%03d] = 0x%llx\n", wsh->singlebranch_count + 1, u->uc_mcontext.gregs[REG_RIP]);
				}
			}

			wsh->singlebranch_hash = (wsh->singlebranch_hash >> 2) ^ (~u->uc_mcontext.gregs[REG_RIP]);
			wsh->singlebranch_count++;
		}

		set_branch_flag();
		u->uc_mcontext.gregs[REG_EFL] |= 0x100;		// Set Trace flag
		return ;

	} else if(wsh->trace_singlestep) {
		/**
		* We are single stepping
		*/

		if((u->uc_mcontext.gregs[REG_RIP] & ~0xffffff) != ((long long int)traphandler & ~0xffffff)){	// Make sure we are not tracing ourselves
			if(wsh->opt_verbosetrace){
				symbols_t *s = symbol_from_addr(u->uc_mcontext.gregs[REG_RIP]);

				if((s)&&(u->uc_mcontext.gregs[REG_RIP] == (long long int)s->addr)){
					fprintf(stderr, " -- Step[%03d] = 0x%llx\t%s(", wsh->singlebranch_count + 1, u->uc_mcontext.gregs[REG_RIP], s->symbol);
#ifdef DEBUG
#ifdef __amd64__

					printarg(u->uc_mcontext.gregs[REG_RDI]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_RSI]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_RDX]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_RCX]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_R8]);
					fprintf(stderr, ", ");
					printarg(u->uc_mcontext.gregs[REG_R9]);
#endif
#endif
					fprintf(stderr, ")\t%s\n", s->libname);
				}else if(s){
					fprintf(stderr, " -- Step[%03d] = 0x%llx\t%s()+%llu\t%s\n", wsh->singlestep_count + 1, u->uc_mcontext.gregs[REG_RIP], s->symbol, u->uc_mcontext.gregs[REG_RIP] - s->addr, s->libname);
				}else{
					fprintf(stderr, " -- Step[%03d] = 0x%llx\n", wsh->singlestep_count + 1, u->uc_mcontext.gregs[REG_RIP]);
				}

			}
			wsh->singlestep_count++;
			wsh->singlestep_hash = (wsh->singlestep_hash >> 2) ^ (~u->uc_mcontext.gregs[REG_RIP]);
			u->uc_mcontext.gregs[REG_EFL] |= 0x100;		// Set Trace flag
		}
		return ;
	} else if(wsh->trace_unaligned) {
		/**
		* We are tracing unaligned access via SIGBUS, single step once
		*/
		u->uc_mcontext.gregs[REG_EFL] |= 0x40000;	// Set Align flag
		u->uc_mcontext.gregs[REG_EFL] ^= 0x100;		// Unset Trace flag
		return;
	} else {

		/**
		* This is an unhandled exception : exit
		*/
		if (u->uc_mcontext.gregs[REG_ERR] & 0x2) {
			fault = FAULT_WRITE;	// Write fault
			hfault = "WRITE";
		} else if (s->si_addr == (void *)u->uc_mcontext.gregs[REG_RIP]) {
			fault = FAULT_EXEC;	// Exec fault
			hfault = "EXEC";
		} else {
			fault = FAULT_READ;	// Read fault
			hfault = "READ";
		}
		signame = signaltoname(signal);

		fprintf(stderr, "%s\t(%u)\trip:%llx	%s\t%p\t", signame, signal, u->uc_mcontext.gregs[REG_RIP], hfault, s->si_addr);
		psiginfo(s, "");
		print_backtrace();
		printf(" -- No corresponding breakpoint (among %u), exiting\n", wsh->bp_num);
		_Exit(EXIT_SUCCESS);
	}

#endif
	if (signal) {
		if (!wsh->firstsignal) {
			wsh->firstsignal = signal;
		}
		wsh->totsignals += 1;
	}
}

char *sicode_strerror(int signal, siginfo_t * s)
{
	char *sicode = 0;

	switch (signal) {
	case SIGBUS:
		switch (s->si_code) {
		case BUS_ADRALN:
			sicode = "invalid address alignment";
			break;
		case BUS_ADRERR:
			sicode = "non-existent physical address";
			break;
		case BUS_OBJERR:
			sicode = "object specific hardware error";
			break;
		}
		break;
	case SIGCHLD:
		switch (s->si_code) {
		case CLD_EXITED:
			sicode = "child has exited";
			break;
		case CLD_KILLED:
			sicode = "child was killed";
			break;
		case CLD_DUMPED:
			sicode = "child terminated abnormally";
			break;
		case CLD_TRAPPED:
			sicode = "traced child has trapped";
			break;
		case CLD_STOPPED:
			sicode = "child has stopped";
			break;
		case CLD_CONTINUED:
			sicode = "stopped child has continued";
			break;
		}
		break;
	case SIGILL:
		switch (s->si_code) {
		case ILL_ILLOPC:
			sicode = "illegal opcode";
			break;
		case ILL_ILLOPN:
			sicode = "illegal operand";
			break;
		case ILL_ILLADR:
			sicode = "illegal addressing mode";
			break;
		case ILL_ILLTRP:
			sicode = "illegal trap";
			break;
		case ILL_PRVOPC:
			sicode = "privileged opcode";
			break;
		case ILL_PRVREG:
			sicode = "privileged register";
			break;
		case ILL_COPROC:
			sicode = "coprocessor error";
			break;
		case ILL_BADSTK:
			sicode = "internal stack error";
			break;
		}
		break;
	case SIGFPE:
		switch (s->si_code) {
		case FPE_INTDIV:
			sicode = "integer divide by zero";
			break;
		case FPE_INTOVF:
			sicode = "integer overflow";
			break;
		case FPE_FLTDIV:
			sicode = "floating point divide by zero";
			break;
		case FPE_FLTOVF:
			sicode = "floating point overflow";
			break;
		case FPE_FLTUND:
			sicode = "floating point underflow";
			break;
		case FPE_FLTRES:
			sicode = "floating point inexact result";
			break;
		case FPE_FLTINV:
			sicode = "invalid floating point operation";
			break;
		case FPE_FLTSUB:
			sicode = "subscript out of range";
			break;
		}
		break;
	case SIGSEGV:
		switch (s->si_code) {
		case SEGV_MAPERR:
			sicode = "address not mapped to object";
			break;
		case SEGV_ACCERR:
			sicode = "invalid permissions for mapped object";
			break;
		default:
			sicode = "segmentation fault";
			break;
		}
		break;
	}

	return sicode;
}

void sighandler(int signal, siginfo_t * s, void *ptr)
{
	ucontext_t *u = (ucontext_t *) ptr;
	unsigned int fault = 0;
	char *hfault = 0;
	char *signame = 0;
	char *sicode = 0;
	char defsicode[200];
	unsigned int r = 0;
	char *accesscolor = "";

#ifndef __arm__
	/**
	* Get access type
	*/
	if (u->uc_mcontext.gregs[REG_ERR] & 0x2) {
		fault = FAULT_WRITE;	// Write fault
		hfault = "Write";
		r = 2;
		accesscolor = YELLOW;
	} else if (s->si_addr == (void*)u->uc_mcontext.gregs[REG_RIP]) {
		fault = FAULT_EXEC;	// Exec fault
		hfault = "Exec";
		r = 4;
		accesscolor = RED;
	} else {
		fault = FAULT_READ;	// Read fault
		hfault = "Read";
		r = 1;
		accesscolor = GREEN;
	}

	/**
	* Get signal name
	*/
	signame = signaltoname(signal);

	/**
	* Get signal code
	*/
	sicode = sicode_strerror(signal, s);
	if (!sicode) {
		memset(defsicode, 0x00, 200);
		snprintf(defsicode, 199, "Error code %d", s->si_code);
		sicode = defsicode;
	}

	if ((wsh->totsignals == 0) || (wsh->opt_verbose)) {
		fprintf(stderr, "\n%s[%s]\t%s\t%p" BLUE "        (%s)\n" NORMAL, accesscolor, signame, hfault, s->si_addr, sicode);

		if((fault != FAULT_EXEC)||(!msync(u->uc_mcontext.gregs[REG_RIP]&~0xfff, getpagesize(), 0))){	// Avoid segfaults on generating backtraces...
			print_backtrace();
		}

	}

	if (!wsh->totsignals) {	// Save informations relative to first signal
		wsh->firstsignal = signal;
		wsh->firstsicode = s->si_code;
		wsh->faultaddr = s->si_addr;
		wsh->reason = r;
		memcpy(wsh->errcontext, u, sizeof(ucontext_t));
		wsh->btcaller = u->uc_mcontext.gregs[REG_RIP];
	}

	if (!wsh->firsterrno) {
		wsh->firsterrno = errno;
	}			// Save first errno as firsterrno (treated separately)

	wsh->totsignals += 1;
	wsh->globalsignals += 1;

#ifdef DEBUG
	fprintf(stderr, " !! FATAL ERROR: Instruction Pointer 0x%012llx addr:%012llx\n", u->uc_mcontext.gregs[REG_RIP], s->si_addr);
#endif

#endif // end arm

	/**
	* Restore execution from known good point
	*/
	errno = ENOTRECOVERABLE; /*EFAULT;*/
	longjmp(wsh->longjmp_ptr, 1);
}

/**
* Set all signal handlers
*/
int set_sighandlers(void)
{
	struct sigaction sa;

	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = sighandler;

	if (sigaction(SIGSEGV, &sa, NULL) == -1) {
		perror("sigaction");
		_Exit(EXIT_FAILURE);
	}

	if (sigaction(SIGABRT, &sa, NULL) == -1) {
		perror("sigaction");
		_Exit(EXIT_FAILURE);
	}

	if (sigaction(SIGILL, &sa, NULL) == -1) {
		perror("sigaction");
		_Exit(EXIT_FAILURE);
	}

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = traphandler;
	if (sigaction(SIGTRAP, &sa, NULL) == -1) {
		perror("sigaction");
		_Exit(EXIT_FAILURE);
	}

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = alarmhandler;
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		perror("sigaction");
		_Exit(EXIT_FAILURE);
	}

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = inthandler;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		_Exit(EXIT_FAILURE);
	}

	sa.sa_sigaction = bushandler;
	sa.sa_flags = SA_SIGINFO ;
        sigfillset(&sa.sa_mask);
	if (sigaction(SIGBUS, &sa, NULL) == -1) {
		perror("sigaction");
		_Exit(EXIT_FAILURE);
	}

	return 0;
}

/**
* Set global variable is_stdinscript to 1 if there is data on stdin
*/
int test_stdin(void)
{
	struct pollfd fds;
	int ret = 0;

	fds.fd = 0;		/* fd corresponding to STDIN */
	fds.events = POLLIN;
	ret = poll(&fds, 1, 0);
	if (ret == 1) {
		wsh->is_stdinscript = 1;
		wsh->opt_hollywood = 0;
	} else if (ret == 0) {
		wsh->is_stdinscript = 0;
	} else {
		wsh->is_stdinscript = 0;
	}

	return 0;
}

int wsh_appear(lua_State * L)
{
	wsh->opt_appear = 1;
	rescan();
	parse_link_vdso();

	return 0;
}

int wsh_hide(lua_State * L)
{
	wsh->opt_appear = 0;
	rescan();

	return 0;
}

int verbose(lua_State * L)
{
	void *arg = 0;

	if (lua_isnumber(L, 1)) {
		arg = (unsigned long) lua_tonumber(L, 1);
	}

	printf(" -- Setting verbosity to %lu\n", (unsigned long)arg);
	wsh->opt_verbose = arg;

	return 0;
}

int hollywood(lua_State * L)
{
	void *arg = 0;

	if (lua_isnumber(L, 1)) {
		arg = (unsigned long) lua_tonumber(L, 1);
	}

	printf(" -- Setting hollywood to %lu\n", (unsigned long)arg);
	wsh->opt_hollywood = arg;

	if (wsh->opt_hollywood == 2) {
		printf(GREEN);
	}

	if (wsh->opt_hollywood == 1) {
		printf(NORMAL);
	}

	return 0;
}


/**
* Display mapped sections
*/
int map(lua_State * L)
{
	unsigned int count = 0;
	char *sizes[] = { "b", "Kb", "Mb", "Gb", "Tb", "Pb", "Hb" };
	double len = 0;
	int order = 0;
	struct section *s;

	s = zfirst;
	while (s != 0x00) {
		if (wsh->opt_hollywood) {
			char *pcolor = DARKGRAY;	// NORMAL

			switch (s->perms) {
			case 2:	// r--
				pcolor = GREEN;
				break;
			case 6:	// rw-
				pcolor = BLUE;
				break;
			case 3:	// r-x
				pcolor = RED;
				break;
			case 7:	// rwx
				pcolor = MAGENTA;
				break;
			default:
				break;
			}

			printf(GREEN "%012llx-%012llx" NORMAL "    %s    %s%s" NORMAL "\t\t%lu\n", s->init, s->end, s->hperms, pcolor, s->name, s->size / sysconf(_SC_PAGE_SIZE));
		} else {
			printf("%012llx-%012llx    %s    %s\t\t%lu\n", s->init, s->end, s->hperms, s->name, s->size / sysconf(_SC_PAGE_SIZE));
		}
		if (s->perms) {
			count += s->size / sysconf(_SC_PAGE_SIZE);
		}
		s = s->next;
	}

	len = count * sysconf(_SC_PAGE_SIZE);
	order = 0;
	while ((len >= 1024) && (order <= 3)) {
		order++;
		len = len / 1024;
	}

	printf(" --> total: %u pages mapped (%d %s)\n", count, (unsigned int) len, sizes[order]);

	return 0;
}

/**
* Pollute .bss sections
*/
int bsspolute(lua_State * L)
{
	sections_t *s = 0, *stmp = 0;
	char poison = 0xff;
	unsigned int num = 0;

	DL_FOREACH_SAFE(wsh->shdrs, s, stmp) {
		if((s->name)&&(!strncmp(s->name,".bss",4))){
			num++;
			if(num >= 4){
				printf("[%02u] 0x%012lx-0x%012lx    %s:%s\t%02x\t%s:%lu\t\t\n",num, s->addr, s->addr + s->size, s->name, s->perms, poison, s->libname, s->size);
				memset(s->addr, poison--, s->size);
			}
		}
		s = s->next;
	}

	return 0;
}


/**
* Search a pattern in memory
*/
static char *searchmem(char *start, char *pattern, unsigned int patternlen, unsigned int memsz)
{
	unsigned int i = 0;
	char *ptr = 0;
	unsigned int uplim = 0;

	ptr = start;
	uplim = memsz - patternlen;

	for (i = 0; (i < uplim) && (uplim > 0); i++) {
		if (!memcmp(ptr + i, pattern, patternlen)) {
			return ptr + i;
		}
	}

	return 0;
}

/**
* ralloc(unsigned int size, unsigned char poison);
* allocate 1 page set to 0x00, set size bytes to poison, remap the page R only
*/
int ralloc(lua_State * L)
{
	unsigned int size = 0;
	unsigned char poison = 0;
	unsigned long int ret = 0;
	char *ptr = 0;
//	unsigned long int *ptr2 = 0;
	unsigned int sz = 0;
	unsigned long int baseaddr = 0;

	read_arg1(size);
	read_arg2(poison);

	sz = getpagesize();

	baseaddr = (default_poison + global_xalloc)*0x1010101000; //0x81818181000-0x1000
	ptr = mmap(baseaddr, sz, PROT_WRITE|PROT_READ, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);

	if(ptr == 0){
		fprintf(stderr, " !! ERROR: malloc() : %s",strerror(errno));
		return 0;
	}

	ret = ptr;		// compute return address

	if(wsh->opt_verbosetrace){
		printf("-- ralloc() ptr:%p, size:%u, ret:%lx\t[%lx-%lx]\n", ptr, sz, ret, ret, ret + size);
	}

	mprotect(ptr, sz, PROT_EXEC | PROT_READ | PROT_WRITE);

	mymemset(ptr, poison ? poison : default_poison + global_xalloc, sz);	// map with default poison bytes
	global_xalloc++;

	mymemset(ptr,poison, size % 4096);

	mprotect(ptr, sz, PROT_READ);

	lua_pushlightuserdata(L, ret);

	return 1;
}

/**
* xalloc(unsigned int size,  unsigned char poison, unsigned int perms);
* Allocate size bytes (% getpagesize())
*
* The mapping auto-references itself, unless a poison byte is given
*
* [page unmaped]
* [mapped][OURPTR, size]
* [page unmaped]
*/

ATTRIBUTE_NO_SANITIZE_ADDRESS
int xalloc(lua_State * L)
{
	unsigned int size = 0;
	unsigned int perms = 0;
	unsigned char poison = 0;
	unsigned long int ret = 0;
	char *ptr = 0;
	unsigned long int *ptr2 = 0;
	unsigned int sz = 0;
	unsigned long int baseaddr = 0;

	read_arg1(size);
	read_arg2(poison);
	read_arg3(perms);

	size = size % 4096;

	sz = getpagesize()*3;

	baseaddr = (default_poison + global_xalloc)*0x1010101000-0x1000; //0x616161616000
	ptr = mmap(baseaddr, sz, PROT_WRITE|PROT_READ, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);

	if(ptr == 0){
		fprintf(stderr, " !! ERROR: malloc() : %s",strerror(errno));
		return 0;
	}

	ret = ptr + 2*getpagesize() - size;		// compute return address

	if(wsh->opt_verbosetrace){
		printf("-- ptr:%p, size:%u, ret:%lx\t[%lx-%lx]\n", ptr, sz, ret, ret, ret + size);
	}

	mprotect(ptr, sz, PROT_EXEC | PROT_READ | PROT_WRITE);

	mymemset(ptr, poison ? poison : default_poison + global_xalloc, sz);	// map with default poison bytes
	global_xalloc++;

	if(!poison){	// If autoref, overwrite all the content with address of our own buffer

		for(ptr2 = ptr; ptr2 < (unsigned long*)(ptr + sz) ; ptr2++){	// all 3 pages
			*ptr2 = ret;
		}

		for(ptr2 = ret; ptr2 < (unsigned long*)(ret + size) ; ptr2++){	// just our small allocade part
			*ptr2 = ret;
		}
	}

	mprotect(ptr, sz, perms ? perms : PROT_READ | PROT_WRITE | PROT_EXEC);

	mprotect(ptr+2*getpagesize(), getpagesize(), PROT_NONE);	// Third page is remapped with no permissions

	lua_pushlightuserdata(L, ret);

	return 1;
}

/**
* Release a bloc allocated via xalloc()
*/
void xfree(lua_State * L)
{
	void *ptr = 0, *trueptr = 0;
	unsigned int sz = 0;

	sz = 3*getpagesize();
	read_arg1(ptr);

	trueptr = ((unsigned long int)ptr & ~0xfff)-0x1000;
	mprotect(trueptr, sz, PROT_EXEC | PROT_READ | PROT_WRITE);
	memset(trueptr, 0x00, sz);
	munmap(trueptr, sz);
}


/**
* Resize a xallocated memory zone
*/
//void xrealloc(lua_State * L)
//{
//}

void traceunaligned(lua_State * L)
{
	wsh->trace_singlebranch = 0;
	wsh->trace_singlestep = 0;
	wsh->trace_unaligned = 1;
}

void untraceunaligned(lua_State * L)
{
	wsh->trace_singlebranch = 0;
	wsh->trace_singlestep = 0;
	wsh->trace_unaligned = 0;
}

void singlestep(lua_State * L)
{
	wsh->trace_singlebranch = 0;
	wsh->trace_singlestep = 1;
	wsh->trace_unaligned = 0;
}

void unsinglestep(lua_State * L)
{
	wsh->trace_singlebranch = 0;
	wsh->trace_singlestep = 0;
	wsh->trace_unaligned = 0;
}


void systrace(lua_State * L)
{
	wsh->trace_strace = 1;
	wsh->trace_rtrace = 0;
}

void rtrace(lua_State * L)
{
	wsh->trace_rtrace = 1;
	wsh->trace_strace = 0;
}

void unsystrace(lua_State * L)
{
	wsh->trace_rtrace = 0;
	wsh->trace_strace = 0;
}

void unrtrace(lua_State * L)
{
	wsh->trace_rtrace = 0;
	wsh->trace_strace = 0;
}


void verbosetrace(lua_State * L)
{
	wsh->opt_verbosetrace = 1;
}

void unverbosetrace(lua_State * L)
{
	wsh->opt_verbosetrace = 0;
}

void singlebranch(lua_State * L)
{

	//
	// Technically, it may be possible to give wsh apabilities to run BTR without uid 0
	//
	// sudo setcap cap_sys_rawio=ep ./wsh

	if(getuid() != 0){
		fprintf(stderr, "!! ERROR: You need root privileges to use Branch Tracing\n");
		return;
	}

	// Load LKMs in kernel land
	system("sudo modprobe cpuid");
	system("sudo modprobe msr");


	wsh->trace_singlebranch = 1;
	wsh->trace_singlestep = 0;
	wsh->trace_unaligned = 0;
}

void unsinglebranch(lua_State * L)
{
	wsh->trace_singlebranch = 0;
	wsh->trace_singlestep = 0;
	wsh->trace_unaligned = 0;
}

/**
* Search a given value in memory
*
* grepptr(Pattern, patternlen, hexadumplen, nbytesbeforematch)
*
*/
int grepptr(lua_State * L)
{
//	char *ptr = 0;
//	unsigned long int maxlen = 0, i = 0;
	char *match = 0;
	int count = 1;
	unsigned int dumplen = 200;
	unsigned int k = 0;

	unsigned long int p = 0;
	char pattern[9];
	unsigned int patternsz = 0;
	unsigned int aligned = 0;

	sections_t *s = 0, *stmp = 0;

	read_arg1(p);
	read_arg2(patternsz);
	read_arg3(aligned);

	if (!patternsz) {
		patternsz = sizeof(unsigned long int);
	}
	if (patternsz > 8) {
		fprintf(stderr, "ERROR: Wrong pattern size:%u > 8\n", patternsz);
	}

	printf(" -- Searching Pointer: 0x%lx (length:%u aligned:%u)\n", p, patternsz, aligned);
	memset(pattern, 0x00, 9);
	memcpy(pattern, &p, patternsz);

	/* create result table */
	lua_newtable(L);

	DL_FOREACH_SAFE(wsh->shdrs, s, stmp) {
		k = 0;
		if (!msync(s->addr&~0xfff, s->size, 0)) {
		      searchagain:
			match = searchmem(s->addr + k, pattern, patternsz, s->size - k);
			if (match) {
				if (wsh->opt_hollywood) {
					printf("    match[" GREEN "%d" NORMAL "] at " GREEN "%p" NORMAL " %lu bytes within:%lx-%lx:" GREEN "%s:%s" NORMAL ":%s\n\n", count, match,
					       match - (char *) s->addr, s->addr, s->addr + s->size, s->libname, s->name, s->perms);
				} else {
					printf("    match[%d] at %p %lu bytes within:%lx-%lx:%s:%s\n\n", count, match, match - (char *) s->addr, s->addr, s->addr + s->size, s->name, s->perms);
				}
				int delta = (char *) (s->addr+s->size) - match;
				if (delta > (int)dumplen) {
					delta = dumplen;
				};
				hexdump((unsigned char*)match, patternsz + delta, 0, patternsz);	// Colorize match
				printf("\n");

				/* Add symbol to Lua table */
				lua_pushnumber(L, count);		/* push key */
				lua_pushinteger(L, (unsigned long int)match);		/* push value : matching address */
			        lua_settable(L, -3);

				count++;
				k = match - s->addr + 1;
				goto searchagain;
			}
		}
//		s = s->next;
	}

	// Push number of results as lua return variable
//	lua_pushinteger(L, count);

	return 1;
}

/**
* Load a binary into the address space
*/
int loadbin(lua_State * L)
{
	char *libname = 0;

	read_arg1(libname);
	if(!libname){
		printf("ERROR: missing name of binary to load\n");
		return 0;
	}

	do_loadlib(libname);
	rescan();
	
	return 0;
}


/**
* search a pattern over all sections mapped in memory
*/
int grep(lua_State * L)
{				// Pattern, patternlen, hexadumplen, nbytesbeforematch
//	char *ptr = 0;
//	unsigned int maxlen = 0, i = 0;
	char *match = 0;
	int count = 0;
	char *pattern = 0;
	unsigned int patternlen = 0;
	unsigned int dumplen = 0;
	unsigned int nbytesbeforematch = 0;
	unsigned int k = 0;

	sections_t *s, *stmp;

	read_arg1(pattern);
	read_arg2(patternlen);
	read_arg3(dumplen);
	read_arg(nbytesbeforematch, 4);

	// Enforce sane defaults on optional arguments
	if (!patternlen) {
		patternlen = strlen(pattern);
	}
	if (!dumplen) {
		dumplen = 200;
	}

	/* create result table */
	lua_newtable(L);

	DL_FOREACH_SAFE(wsh->shdrs, s, stmp) {
		k = 0;
		if (!msync(s->addr&~0xfff, s->size, 0)) {
		      searchagain:
			match = searchmem(s->addr + k, pattern, patternlen, s->size - k);
			if (match) {
				if (wsh->opt_hollywood) {
					printf("    match[" GREEN "%d" NORMAL "] at " GREEN "%p" NORMAL " %lu bytes within:%lx-%lx:" GREEN "%s:%s" NORMAL ":%s\n\n", count + 1, match,
					       match - (char *) s->addr, s->addr, s->addr + s->size, s->libname, s->name, s->perms);
				} else {
					printf("    match[%d] at %p %lu bytes within:%lx-%lx:%s:%s\n\n", count + 1, match, match - (char *) s->addr, s->addr, s->addr + s->size, s->name, s->perms);
				}
				int delta = (char *) (s->addr+s->size) - match;
				if (delta > (int)dumplen) {
					delta = dumplen;
				};
				hexdump((unsigned char*)(match - nbytesbeforematch), patternlen + delta, nbytesbeforematch, patternlen);	// Colorize match
				printf("\n");

				/* Add symbol to Lua table */
				lua_pushnumber(L, count + 1);		/* push key */
				lua_pushinteger(L, (unsigned long int)match);		/* push value : matching address */
			        lua_settable(L, -3);

				count++;
				k = match - s->addr + 1;
				goto searchagain;
			}
		}
	}

	// Push number of results as lua return variable
//	lua_pushinteger(L, count);

	return 1;
}

/*
* Return a section from an address
*/
static struct section *sec_from_addr(unsigned long int addr)
{
	struct section *s = zfirst;

	while (s != 0x00) {
		if ((s->init <= addr) && (s->end > addr)) {
			return s;
		}
	}

	return 0;
}

/**
* Our own version of memcpy callable from LUA
*/
int priv_memcpy(lua_State * L)
{
	void *arg1 = 0, *arg2 = 0, *arg3 = 0;
//	char *ptr = 0;
//	char *addr = 0;
	int ret = 0;

	read_arg1(arg1);
	read_arg2(arg2);
	read_arg3(arg3);

	ret = memcpy(arg1, arg2, arg3);

	// Push number of results as lua return variable
	lua_pushinteger(L, ret);

	return 1;
}

/**
* Our own version of strcpy callable from LUA
*/
int priv_strcpy(lua_State * L)
{
	void *arg1 = 0, *arg2 = 0;
//	char *ptr = 0;
//	char *addr = 0;
	int ret = 0;

	read_arg1(arg1);
	read_arg2(arg2);

	ret = strcpy(arg1, arg2);

	// Push number of results as lua return variable
	lua_pushinteger(L, ret);

	return 1;
}

/**
* Our own version of strcat callable from LUA
*/
int priv_strcat(lua_State * L)
{
	void *arg1 = 0, *arg2 = 0;
//	char *ptr = 0;
//	char *addr = 0;
	int ret = 0;

	read_arg1(arg1);
	read_arg2(arg2);

	ret = strcat(arg1, arg2);

	// Push number of results as lua return variable
	lua_pushinteger(L, ret);

	return 1;
}

/**
* Set a breakpoint
*/
int breakpoint(lua_State * L)
{
	void *arg1 = 0, *arg2 = 0;
	char *ptr = 0;
	char *addr = 0;
	char bk = 0;

	read_arg1(arg1);
	read_arg1(arg2);

	/**
	* Make sure destination address is mapped
	*/
	if ((!arg1) || (msync((long int)arg1 & (long int)~0xfff, 4096, 0))) {
		fprintf(stderr, "ERROR: Address %p is not mapped\n", arg1);
		return 0;
	}

	/**
	* Change memory protections to RWX on destionation's page
	*/
	ptr = arg1;
	addr = ((unsigned long int) ptr & (unsigned long int) ~0xfff);
	printf(" ** Setting  BREAKPOINT[%u]  (weigth:%lu)	<", wsh->bp_num + 1, (unsigned long int) arg2);
	info_function(arg1);
	mprotect(addr, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE | PROT_EXEC);

	/**
	* Backup byte at destination
	*/
	bk = ptr[0x00];

	/**
	* Write Breakpoint
	*/
	ptr[0x00] = 0xcc;

	/**
	* Save breakpoint informations
	*/
	if (!wsh->bp_num) {
		wsh->bp_array = calloc(1, sizeof(breakpoint_t));
	} else {
		wsh->bp_array = realloc(wsh->bp_array, sizeof(breakpoint_t) * (wsh->bp_num + 1));
	}

	wsh->bp_array[wsh->bp_num].ptr = ptr;
	wsh->bp_array[wsh->bp_num].backup = bk;
	wsh->bp_array[wsh->bp_num].weight = arg2;
	wsh->bp_num++;

	return 0;
}

void declare_func(void *addr, char *name)
{
	lua_pushcfunction(wsh->L, addr);
	lua_setglobal(wsh->L, name);
}

void declare_num(int val, char *name)
{
	lua_pushnumber(wsh->L, val);
	lua_setglobal(wsh->L, name);
}

/**
* Export functions to lua
*/
void declare_internals(void)
{
//	tuple_t *t;
	unsigned int i;

	/**
	* Create definitions for internal functions
	*/
	for(i=0;i<sizeof(exposed)/sizeof(tuple_t);i++){
		declare_func(exposed[i].addr, exposed[i].name);	// Declare function within Lua
	}

	declare_num(wsh->bp_points, "bp_points");

	/**
	* Create a wrapper functions for other internal functions
	*/
	char *luacmd = calloc(1, 1024);
	snprintf(luacmd, 1023, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", "hexdump", "lhexdump");
	luabuff_append(luacmd);
	memset(luacmd, 0x00, 1024);
	snprintf(luacmd, 1023, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, string.len(a), c, d, e, f, g, h); return j, k; end\n", "hex", "lhexdump");
	luabuff_append(luacmd);
	memset(luacmd, 0x00, 1024);
	snprintf(luacmd, 1023, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", "execlib", "lexeclib");
	luabuff_append(luacmd);
	memset(luacmd, 0x00, 1024);
	snprintf(luacmd, 1023, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", "disasm", "ldisasm");
	luabuff_append(luacmd);
	memset(luacmd, 0x00, 1024);
	snprintf(luacmd, 1023, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", "deref", "lderef");
	luabuff_append(luacmd);
	memset(luacmd, 0x00, 1024);
	snprintf(luacmd, 1023, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", "strace", "lstrace");
	luabuff_append(luacmd);
	memset(luacmd, 0x00, 1024);
	snprintf(luacmd, 1023, "function %s (a, b, c, d, e, f, g, h) j,k = libcall(%s, a, b, c, d, e, f, g, h); return j, k; end\n", "script", "lscript");	
	luabuff_append(luacmd);
	free(luacmd);
}


int set_alloc_opt(void)
{
	setenv("LIBC_FATAL_STDERR_", "yes", 1);
	mallopt(M_CHECK_ACTION, 3);

	return 0;
}

/**
* Generate a core file
*/
int gencore(lua_State * L)
{
	enable_core(L);
	if(!fork()){
		kill(getpid(), SIGQUIT);
	}

	return 0;
}

/**
* Disable core files generation
*/
int disable_core(lua_State * L)
{

	int err = 0;

	errno = 0;
	err = prctl(PR_SET_DUMPABLE, (long)0);

	if(err){
		printf("ERROR: prctl() %s\n", strerror(errno));
	}

	return 0;
}

/**
* Enable core files generation
*/
int enable_core(lua_State * L)
{
	int err = 0;
	errno = 0;

	err = prctl(PR_SET_DUMPABLE, (long)1);
	if(err){
		printf("ERROR: prctl() %s\n", strerror(errno));
	}

	return 0;
}

int wsh_init(void)
{
	set_sighandlers();

	// create context
	wsh = calloc(1, sizeof(wsh_t));

	// test if we're reading a script from stdin
	test_stdin();

	// create lua state
	wsh->L = luaL_newstate();	/* Create Lua state variable */

	// make sure version of lua matches
	luaL_checkversion(wsh->L);

	luaL_openlibs(wsh->L);	/* Load Lua libraries */

	// Declare internal functions
	declare_internals();

	// Load indirect functions
	load_indirect_functions(wsh->L);

	// Default is to disable core files
//	disable_core(wsh->L);

	// Set malloc options
	set_alloc_opt();

	// Set process name
	prctl(PR_SET_NAME,"wsh",NULL,NULL,NULL);

	return 0;
}

static char *lua_strerror(int err)
{
	switch (err) {
	case 1:
		return "Lua Yield";
	case 2:
		return "Runtime Error";
	case 3:
		return "Synthax Error";
	case 4:
		return "Memory Error";
	case 5:
		return "Fatal Error";
	default:
		return "Unknown Error";
	};

	return NULL;		// Never reached
}

/**
* Run a lua script
*/
int run_script(char *name)
{
	char myerror[200];
	int err = 0;

	if(!name){ return -1;}

	if(wsh->opt_verbose){
		printf("  * Running lua script %s\n", name);
	}

	memset(myerror, 0x00, 200);
	err = 0;
	if ((err = luaL_loadfile(wsh->L, name) != 0)) {	/* Load but don't run the Lua script */
		snprintf(myerror, 199, "error %d : %s", err, lua_strerror(err));
		printf(stderr, "luaL_loadfile() failed for script %s (%s)\n", name, errno ? strerror(errno) : myerror);	/* Error out if file can't be read */
		return -1;
	}
	if (wsh->opt_verbose) {
		printf("  * Running lua script %s\n", name);
	}

	memset(myerror, 0x00, 200);

	if (lua_pcall(wsh->L, 0, 0, 0)) {	/* Run the loaded Lua script */
		fprintf(stderr, "lua_pcall() failed with %s, for: %s\n",lua_tostring(wsh->L, -1), name);	/* Error out if Lua file has an error */
		lua_pop(wsh->L, 1);	// pop error message from the stack 
	}
	lua_settop(wsh->L, 0);	// remove eventual returns 

	return 0;
}

/**
* Verify ELF signature in a binary
*/
unsigned int read_elf_sig(char *fname, struct stat *sb)
{
	int fd = 0;
	char sig[5];
	char validelf[4] = "\177ELF";
	if (sb->st_size < MIN_BIN_SIZE) {
		return 0;	// Failure
	}
	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 0;
	}
	memset(sig, 0x00, 5);
	read(fd, sig, 4);
	close(fd);

	return strncmp(sig, validelf, 4) ? 0 : 1;
}

/**
* Verify ar archive signature in a binary
*/
unsigned int read_archive_sig(char *fname, struct stat *sb)
{
	int fd = 0;
	char sig[10];
//	char validelf[4] = "\177ELF";
	if (sb->st_size < 20) {
		return 0;	// Failure
	}
	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 0;
	}
	memset(sig, 0x00, 10);
	read(fd, sig, 10);
	close(fd);

	return strncmp(sig, "!<arch>", 7) ? 0 : 1;
}


/**
* Execute default internal scripts
*/
int exec_default_scripts(void)
{
	int err = 0;

	if ((err =luaL_loadfile(wsh->L, DEFAULT_SCRIPT_INDEX)) != 0) {
		printf("ERROR: %s in script %s (%s)\n", lua_strerror(err), DEFAULT_SCRIPT_INDEX, lua_tostring(wsh->L, -1));
		fatal_error(wsh->L, "luaL_loadfile() failed");
	}
	if(lua_pcall(wsh->L, 0, 0, 0)){
		fprintf(stderr, "ERROR: lua_pcall() failed with %s\n",lua_tostring(wsh->L, -1));
	}

	return 0;
}


int load_home_user_file(char *fname)
{
	char pathname[255];
	struct stat sb;
	int err = 0;

	memset(pathname, 0x00, 255);

	if(!getenv("HOME")){
		printf("WARNING: HOME environment variable not set : skipping %s file", fname);
		return -1;
	}

	snprintf(pathname, 254, "%s/%s", getenv("HOME"), fname);

	if (stat(pathname, &sb) == -1) {
		if(wsh->opt_verbose){
			printf("WARNING: %s file not found\n", pathname);
		}
		errno = 0;
		return -1;
	}

	if(wsh->opt_verbose){
		printf("  * Running user startup script %s\n", pathname);
	}

	// load file from home user directory if present
	err = 0;
	if ((err = luaL_loadfile(wsh->L, pathname)) != 0) {
		printf("WARNING: %s while running startup script %s (%s)\n", lua_strerror(err), pathname, lua_tostring(wsh->L, -1));
	}

	if(lua_pcall(wsh->L, 0, 0, 0)){
		fprintf(stderr, "ERROR: lua_pcall() failed with %s\n",lua_tostring(wsh->L, -1));
	}

	return 0;
}


/**
* Load .wsh_profile script if it exists
* We search for it in the directory
* corresponding to environment variable HOME
*/
int load_profile(void)
{
	load_home_user_file(DEFAULT_WSH_PROFILE);
	return 0;
}

/**
* Load .wshrc script if it exists
* We search for it in the directory
* corresponding to environment variable HOME
*/
int load_wshrc(void)
{
	load_home_user_file(DEFAULT_WSHRC);
	return 0;
}

/**
* Run a lua shell/script
*/
int wsh_run(void)
{
	struct script_t *s = 0;
	unsigned int scriptcount = 0;

	DL_COUNT(wsh->scripts, s, scriptcount);

	read_maps(getpid());
	parse_link_map_dyn((struct link_map *)wsh->mainhandle);
	parse_link_vdso();

	/**
	* run internal lua buffers
	*/
	exec_luabuff();

	if (wsh->opt_verbose) {
		printf(" -- Running startup lua scripts\n");
	}

	/**
	* Execute default internal scripts
	*/
	exec_default_scripts();

	/**
	* load .wshrc if present
	*/
	load_wshrc();

	/**
	* Run all the scripts specified in the command line
	*/
	if (wsh->opt_verbose) {
		printf(" -- %u user scripts in queue\n", scriptcount);
	}

	script_t *ss, *stmp;
	DL_FOREACH_SAFE(wsh->scripts, ss, stmp) {
		run_script(ss->name);
	}

	/**
	* Run a lua shell
	*/
	if (!sigsetjmp(wsh->longjmp_ptr_high, 1)){ // This is executed only the first time // save stack context + signals
		run_shell(wsh->L);
	}else{
		if(wsh->longjmp_ptr_high_cnt++ < 3){
			printf("RESTORING FROM SANE STACK STATE (%u/3)\n", wsh->longjmp_ptr_high_cnt);
			run_shell(wsh->L);
		}else{
			printf("\n%s[FATAL]\tInterrupted too many times : exiting%s\n",RED,NORMAL);
			_Exit(EXIT_FAILURE);
		}
	}

	lua_close(wsh->L);	/* Clean up, free the Lua state var */
	return 0;

}


int add_script_arguments(int argc, char **argv, unsigned int i)
{
	unsigned int j = 0;

	if (i >= (unsigned int)argc) {
		return -1;
	}			// Should not happen

	wsh->script_argnum = argc - i;

	wsh->script_args = calloc(sizeof(void *), argc);

	for (j = 0; j < wsh->script_argnum; j++) {
		wsh->script_args[j] = strdup(argv[j + i]);
		printf("argument[%u]: %s\n", j, wsh->script_args[j]);
	}
	return wsh->script_argnum;
}

/**
* Add a script to the execution queue
*/
int add_script_exec(char *name)
{
	struct script_t *s;

	s = calloc(1, sizeof(struct script_t));
	s->name = strdup(name);
	DL_APPEND(wsh->scripts, s);

	return 0;
}

/**
* Add a binary to the list of binaries to preload
*/
int add_binary_preload(char *name)
{
	struct preload_t *p;

	p = calloc(1, sizeof(struct preload_t));
	p->name = strdup(name);
	DL_APPEND(wsh->preload, p);

	return 0;
}

/**
* Turn a static library into a dynamic library
*/
char *ar2lib(char *name)
{
	char cmd[1024];
	char *tmp_dirname = 0;

	/**
	* Create temporary directory under /tmp/
	*
	* NOTE: The directory name written to is predictable.
	*/
	tmp_dirname = calloc(20, 1);
	snprintf(tmp_dirname, 19, "/tmp/.wsh-%u", getpid());
	if(mkdir(tmp_dirname, 0700)){
		fprintf(stderr, "!! ERROR : mkdir(%s, ...) %s\n", tmp_dirname, strerror(errno));
		return NULL;
	}

	// Recompile static library as dynamic library
	memset(cmd, 0x00, 1024);
	snprintf(cmd, 1023, "gcc -Wl,--whole-archive %s -Wl,--no-whole-archive  -o /tmp/.wsh-%u/%s.so -shared -rdynamic -ggdb -g3", name, getpid(), basename(name));
	system(cmd);

	memset(cmd, 0x00, 1024);
	snprintf(cmd, 1023, "/tmp/.wsh-%u/%s.so", getpid(), basename(name));

	if(verbose){
		printf(" -- relinked static library (ar archive) %s into dynamic shared library %s\n", name, cmd);	
	}
	return strdup(cmd);
}

/**
* Patch ELF ehdr->e_type to ET_DYN
*/
int mk_lib(char *name)
{
  int fd = 0;
  struct stat sb;
  char *map = 0;
  Elf32_Ehdr *ehdr32;
  Elf64_Ehdr *ehdr64;

  Elf_Ehdr *elf = 0;
  Elf_Phdr *phdr = 0;
  Elf_Dyn *dyn;
  unsigned int phnum = 0;
  unsigned int i = 0, j = 0;

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

  /**
  * Work around : https://patchwork.ozlabs.org/project/glibc/patch/20190312130235.8E82C89CE49C@oldenburg2.str.redhat.com/
  * We need to find and nullify entries DT_FLAGS, DT_FLAGS_1 and DT_BIND_NOW in dynamic section
  * Use segments instead of sections so we may load ELFs without section headers.
  */
  elf = (Elf_Ehdr *) map;
  phdr = (Elf_Phdr *) (map + elf->e_phoff);
  phnum = elf->e_phnum;

  for ( i = 0; i < phnum ; i++) {
    // Find dynamic segment
    if(phdr[i].p_type == PT_DYNAMIC){
      dyn = map + phdr[i].p_offset;
      // Patch dynamic segment
      for ( j = 0; j < (phdr[i].p_filesz/sizeof(Elf_Dyn)) ; j++) {
        switch (dyn->d_tag) {
        case DT_FLAGS:
        case DT_POSFLAG_1:
        case DT_BIND_NOW:
            dyn->d_tag = DT_NULL;
            dyn->d_un.d_val = -1;
            break;
        default:
          break;
        }
        dyn += 1;
      }
      break;
    }
  }  

  munmap(map, sb.st_size);
  close(fd);
  return 0;
}

/**
* Attempt to patch a ET_EXEC binary as ET_DYN,
* making it suitable for use with dlopen()
*/
int attempt_to_patch(char *libname)
{
	struct stat sb;
	int fdin = 0, fdout = 0;
	unsigned int copied = 0;
	char *tmp_dirname = 0;
	char *outlib = 0;

	/**
	* Verify file exists
	*/
	if (stat(libname, &sb) == -1) {
		if(wsh->opt_verbose){
			printf("WARNING: %s file not found\n", libname);
		}
		return 0; // Fail
	}

//	printf("%s : %u bytes\n", libname, sb.st_size);

	fdin = open(libname, O_RDONLY, 0700);
	if (fdin < 0) {
		fprintf(stderr, "!! ERROR : open(%s, O_RDONLY) %s\n", libname, strerror(errno));
		return 0; // Fail
	}

	/**
	* Create temporary directory under /tmp/
	*
	* NOTE: The directory name written to is predictable. Bug ?
	*/
	tmp_dirname = calloc(20, 1);
	snprintf(tmp_dirname, 19, "/tmp/.wsh-%u", getpid());
	if(mkdir(tmp_dirname, 0700)){
		fprintf(stderr, "!! ERROR : mkdir(%s, ...) %s\n", tmp_dirname, strerror(errno));
	}

	/**
	* Open destination file
	*/

	outlib = calloc(1, 300);
	snprintf(outlib, 299, "/%s/%s", tmp_dirname, basename(libname));

	printf(" ** libifying %s to %s (%lu bytes)\n", libname, outlib, sb.st_size);

	fdout = open(outlib, O_RDWR|O_CREAT|O_TRUNC, 0700);
	if (fdout < 0) {
		fprintf(stderr, "!! ERROR : open(%s, O_RDWR) %s\n", outlib, strerror(errno));
		return 0; // Fail
	}

	/**
	* Copy binary under newly created directory, with Zero copy data (sendfile())
	*/
	copied = sendfile(fdout, fdin, 0, sb.st_size);
	if(copied != sb.st_size){
		fprintf(stderr, "!! ERROR: sendfile(); Copy failed: %s\n", strerror(errno));
		return 0; // Fail
	}

	close(fdin);
	close(fdout);

	/**
	* Patch ET_EXEC into ET_DYN
	*/
	mk_lib(outlib);

	if(wsh->libified++ != 0){
		printf("\n\n Libifying more than once per process is likely to crash...\n\n");
	}

	return dlopen(outlib, wsh->opt_global ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW);
}


/**
* Do load a shared binary into the address space
*/
struct link_map *do_loadlib(char *libname)
{
	struct link_map *handle = 0;
//	unsigned long int ret = 0;

	if((!libname)||(!strlen(libname))){
		printf("ERROR: missing name of binary to load\n");
		return 0;
	}

	if (wsh->opt_verbose) {
		printf("  * Loading %s\n", libname);
	}

	handle = dlopen(libname, wsh->opt_global ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW);
	if (!handle) {
		fprintf(stderr, "ERROR: dlopen() %s \n", dlerror());

		// attempt to patch binary as ET_DYN if was ET_EXEC
		handle = attempt_to_patch(libname);
		if(!handle){
			fprintf(stderr, "ERROR: dlopen() of patched file! %s \n", dlerror());
			return 0;
		}else{
			printf(" ** loading of libified binary succeeded\n");
		}
	}

	if (wsh->opt_verbose) {
//		printf("  * Base address: %p for %s\n", (void *) handle->l_addr, libname);
		printf("  * Base address: %p\n", (void *) handle->l_addr);
	}

	dlerror();		// Clear any existing load error
	wsh->mainhandle = handle;	// Last loaded object is always the new handle
	return handle;
}

/**
* Load all preload libraries
*/
int wsh_loadlibs(void)
{
	struct preload_t *p = 0, *tmp = 0;
	unsigned int count = 0;

	DL_COUNT(wsh->preload, p, count);

	if (wsh->opt_verbose) {
		printf(" -- Preloading %u binaries\n", count);
	}

	DL_FOREACH_SAFE(wsh->preload, p, tmp) {
		do_loadlib(p->name);
	}

	return 0;
}

/**
* Parse command line
*/
int wsh_getopt(int argc, char **argv)
{
	const char *short_opt = "hqvVxg";
	int count = 0;
	struct stat sb;
	int c = 0, i = 0;

	struct option long_opt[] = {
		{"help", no_argument, NULL, 'h'},
		{"args", no_argument, NULL, 'x'},
		{"quiet", no_argument, NULL, 'q'},
		{"global", no_argument, NULL, 'g'},
		{"verbose", no_argument, NULL, 'v'},
		{"version", no_argument, NULL, 'V'},
		{NULL, 0, NULL, 0}
	};

	wsh->opt_hollywood = 1;	// Set sane default

	wsh->selflib = realpath("/proc/self/exe", 0);

	while ((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		count++;
		switch (c) {
		case -1:	/* no more arguments */
		case 0:
			break;

		case 'h':
			wsh_usage(argv[0]);
			_Exit(EXIT_SUCCESS);
			break;

		case 'g':
			wsh->opt_global = 1;
			break;

		case 'q':
			wsh->opt_quiet = 1;
			break;

		case 'v':
			wsh->opt_verbose = 1;
			break;

		case 'V':
			wsh_print_version();
			_Exit(EXIT_SUCCESS);
			break;

		case 'x':
			goto nomoreargs;
			break;

		default:
			fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			_Exit(EXIT_FAILURE);
		};
	};

nomoreargs:

	if (count >= argc - 1) {
		return 0;	// no file argument
	}

	if (wsh->opt_verbose) {
		printf(" -- Parsing command line\n");
	}

	for (i = count + 1; i < argc; i++) {

		if (!strncmp(argv[i], "-x", strlen(argv[i]))) {	// Is this an argument stopper ?
			// Every remaining argument is a script argument
			add_script_arguments(argc, argv, i + 1);
			break;
		} else if (!stat(argv[i], &sb)) {	// file exists. Determine if this is a binary or a script
			if (read_elf_sig(argv[i], &sb)) {		// Process as ELF file
				if (wsh->opt_verbose) {
					printf("  * User binary %s\n", argv[i]);
				}
				add_binary_preload(argv[i]);
			}else if (read_archive_sig(argv[i], &sb)) {	// Process as ar archive
				if (wsh->opt_verbose) {
					printf("  * ar archive %s\n", argv[i]);
				}
				add_binary_preload(ar2lib(argv[i]));
			} else {					// Process as a script
				if (wsh->opt_verbose) {
					printf("  * User script %s\n", argv[i]);
				}
				add_script_exec(argv[i]);
			}
		} else {	// Every remaining argument is a script argument
			add_script_arguments(argc, argv, i);
			break;
		}
	}

	// Load user profile
	load_profile();

	return 0;
}

/**
* Print software version
*/
int wsh_print_version(void)
{
	printf("%s version:%s    (%s %s)\n", WNAME, WVERSION, WTIME, WDATE);
	return 0;
}

/**
* Print usage
*/
int wsh_usage(char *name)
{
	printf("Usage: %s [script] [-h|-q|-v|-V|-g] [binary1] [binary2] ... [-x [script_arg1] [script_arg2] ...]\n", name);
	printf("\n");
	printf("Options:\n\n");
	printf("    -x, --args                Optional script argument separator\n");
	printf("    -q, --quiet               Display less output\n");
	printf("    -v, --verbose             Display more output\n");
	printf("    -g, --global              Bind symbols globally\n");
	printf("    -V, --version             Display version and build, then exit\n");
	printf("\n");
	printf("Script:\n\n");
	printf("    If the first argument is an existing file which is not a known binary file format,\n");
	printf("    it is assumed to be a lua script and gets executed.\n");
	printf("\n");
	printf("Binaries:\n\n");
	printf("    Any binary file name before the -x tag gets loaded before running the script.\n");
	printf("    The last binary loaded is the main binary analyzed.\n");
	printf("\n");
	return 0;
}


int teletype(lua_State * L)
{
	char *str = 0;
	unsigned int i = 0;

	read_arg1(str);
	printf("%s", GREEN);
	fflush(stdout);
	for(i = 0; i < strlen(str); i++){
		printf("%c", str[i] & 0xff);
		usleep(10000);
		fflush(stdout);
	}
	printf("%s", NORMAL);
	printf("\n");
	fflush(stdout);
	return 0;
}


/***** Mapping of buffers *****/


/**
* string res rawmemread(addr, len)
*
* Read len bytes at address addr and return them as a lua string.
*/
int rawmemread (lua_State *L) {
	char *addr = 0;
	size_t len = 0;
	read_arg1(addr);
	read_arg2(len);

	if(addr == NULL){ return 0; }
	if((unsigned long int)addr < 4096){ return 0; }	// 1st page detection

	lua_pushlstring(L, addr, len);
	return 1;
}

/**
* int written rawmemwrite(addr, data, len)
*
* Raw write to addr of len bytes of data
* returns number of bytes written.
*/
int rawmemwrite (lua_State *L) {
	char *addr = 0;
	char *data = 0;
	size_t len = 0;
	read_arg1(addr);
	read_arg2(data);
	read_arg3(len);

	if(addr == NULL){ return 0; }
	if((unsigned long int)addr < 4096){ printf("ERROR: Write to first page forbidden\n"); return 0; }	// 1st page detection

	memmove(addr, data, len);
	lua_pushinteger(L, len);
	return 1;
}

/**
* Returns a string, from an address passed as argument.
*/
int rawmemstr (lua_State *L) {
	char *addr = 0;
	read_arg1(addr);

	if(addr == NULL){ return 0; }
	if((unsigned long int)addr < 4096){ printf("ERROR: Read first page forbidden\n"); return 0; }	// 1st page detection

	lua_pushstring(L, addr);
	return 1;
}

/**
* Display memory usage.
*/
int rawmemusage (lua_State *L) {
	struct rusage usage;
	getrusage(RUSAGE_SELF,&usage);

	printf("  %s, %li\n",  " maximum resident set size "    , usage.ru_maxrss  );
	printf("  %s, %li\n",  " page reclaims "                , usage.ru_minflt  );
	printf("  %s, %li\n",  " block input operations "       , usage.ru_inblock );
	printf("  %s, %li\n",  " block output operations "      , usage.ru_oublock );
	printf("  %s, %li\n",  " voluntary context switches "   , usage.ru_nvcsw   );
	printf("  %s, %li\n",  " involuntary context switches " , usage.ru_nivcsw  );

	printf("Memory usage: %ld Kb\n",usage.ru_maxrss);
	lua_pushinteger(L, usage.ru_maxrss);
	return 1;
}

/**
* int addr rawmemaddr(obj)
*
* Return the address in memory of the object passed as argument.
* Or returns an address itself if an address is given as argument.
*/
int rawmemaddr(lua_State *L) {
	unsigned long int len = 0;
	read_arg1(len);

	lua_pushinteger(L, len);
	return 1;
}

/**
* int rawmemstrlen(addr)
* Returns the length of a string passed as argument
*/
int rawmemstrlen(lua_State *L) {
	char *addr = 0;
	read_arg1(addr);

	lua_pushinteger(L, strlen(addr));
	return 1;
}

/**
* int wrptr(addr, data, len)
*
* returns number of bytes written.
*/

int wrptr (lua_State *L) {
	char *data = 0;
	size_t offset = 0;
	char *ptr = 0;
	read_arg1(data);
	read_arg2(offset);
	read_arg3(ptr);

	if(data == NULL){ return 0; }
	if((unsigned long int)data < 4096){ printf("ERROR: Write to first page forbidden\n"); return 0; }	// 1st page detection

	data[offset] = ptr;
	lua_pushinteger(L, data);
	return 1;
}

/**
* Set default environment variables in constructor
*/

__attribute__((constructor))
static void initialize_wsh() {
//	printf("init\n");
	setenv("LIBC_FATAL_STDERR_", "1", 1);
	setenv("MALLOC_CHECK_", "3", 1);
}


/**
* Methods for manipulating arrays of strings from lua
*


// metatable method for handling "array[index]"
static int array_index (lua_State* L) { 
   int** parray = luaL_checkudata(L, 1, "array");
   int index = luaL_checkinteger(L, 2);
   lua_pushnumber(L, (*parray)[index-1]);
   return 1; 
}

// metatable method for handle "array[index] = value"
static int array_newindex (lua_State* L) { 
   int** parray = luaL_checkudata(L, 1, "array");
   int index = luaL_checkinteger(L, 2);
   int value = luaL_checkinteger(L, 3);
   (*parray)[index-1] = value;
   return 0; 
}

void luaL_openlib(lua_State *L, const char *libname, const luaL_Reg *l, int nup) {

  if ( libname ) {
    lua_getglobal(L, libname); 
    if (lua_isnil(L, -1)) { 
      lua_pop(L, 1); 
      lua_newtable(L); 
    }
  }
  luaL_setfuncs(L, l, 0); 
  if ( libname )   lua_setglobal(L, libname);
}

// create a metatable for our array type
static void create_array_type(lua_State* L) {
   static const struct luaL_reg array[] = {
      { "__index",  array_index  },
      { "__newindex",  array_newindex  },
      NULL, NULL
   };
   luaL_newmetatable(L, "array");
   luaL_openlib(L, NULL, array, 0);
}

// expose an array to lua, by storing it in a userdata with the array metatable
static int expose_array(lua_State* L, int array[]) {
   int** parray = lua_newuserdata(L, sizeof(int**));
   *parray = array;
   luaL_getmetatable(L, "array");
   lua_setmetatable(L, -2);
   return 1;
}

// test data
int mydata[] = { 1, 2, 3, 4 };

// test routine which exposes our test array to Lua 
static int getarray (lua_State* L) { 
   return expose_array( L, mydata ); 
}

int  luaopen_array (lua_State* L) {
   create_array_type(L);

   // make our test routine available to Lua
   lua_register(L, "array", getarray);
   return 0;
}

*/





/**
* Get a pointer to a variable
*/
int getptr(lua_State * L)
{
	char *vname = 0;

	read_arg1(vname);
	if(!vname){
		printf("ERROR: missing name of variable\n");
		return 0;
	}

	lua_pushlightuserdata(L, vname);

	return 1;
}

/**
* Create a pointer to a variable
*/
int mkptr(lua_State * L)
{
	char *vname = 0;
	char **newptr = 0;

	read_arg1(vname);
	if(!vname){
		printf("ERROR: missing name of variable\n");
		return 0;
	}

	newptr = calloc(1, sizeof(char*));
	newptr[0] = vname;

	lua_pushinteger(L, newptr);

	return 1;
}


