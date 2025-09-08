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
#define __USE_GNU
#define _GNU_SOURCE
#include <bfd.h>
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/procfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <utlist.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libelf.h>
#include <gelf.h>

#include <nametotype.h>
#include <nametoalign.h>
#include <nametoentsz.h>
#include <nametolink.h>
#include <nametoinfo.h>
#include <arch.h>
#include <aux.h>
#include <inttypes.h>
#include <capstone/capstone.h>

#include <config.h>

#define DEFAULT_STRNDX_SIZE 4096

// Valid flags for msec_t->flags
#define FLAG_BSS        1
#define FLAG_NOBIT      2
#define FLAG_NOWRITE    4
#define FLAG_TEXT       8

#define EXTRA_CREATED_SECTIONS 5


#define RELOC_X86_64 1
#define RELOC_X86_32 2

//#ifdef __x86_64__
#ifdef __LP64__			// Generic 64b
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
#else				// Generic 32b
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
#endif


#define nullstr "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
unsigned int maxoldsec = 0, maxnewsec = 0;
unsigned int deltastrtab = 0;

char *allowed_sections[] = {
	".rodata",
	".data",
	".text",
	".load",
	".strtab",
	".symtab",
	".comment",
	".note.GNU-stack",
	".rsrc",
	".bss",
//      ".rela.plt",
//      ".rela.dyn"
};

char *blnames[] = {
	"__init_array_start",
	"__init_array_end",
	"__libc_csu_init",
	"__libc_csu_fini",
	"__x86.get_pc_thunk.bx",	// this is 32b only
	".bss",
	".comment",
	".data",
	".dynamic",
	".fini",
	".fini_array",
	".got",
	".got.plt",
	".init",
	".init_array",
	".jcr",
	".plt",
	".plt.got",
	".text",
	"__GNU_EH_FRAME_HDR",
	"__FRAME_END__",
	".interp",
	".note.ABI-tag",
	".note.gnu.build-id",
	".gnu.hash",
	".dynsym",
	".dynstr",
	".gnu.version",
	".gnu.version_r",
	".rela.dyn",
	".rela.plt",
	".rodata",
	".eh_frame_hdr",
	".eh_frame",
	"_ITM_registerTMCloneTable",
	"_ITM_deregisterTMClone",
	"_ITM_deregisterTMCloneTab",
	"_Jv_RegisterClasses",
	"_ITM_registerTMCloneTa",
	"__cxa_finalize",
	"_DYNAMIC",
	"_GLOBAL_OFFSET_TABLE_",
	"__JCR_END__",
	"__JCR_LIST__",
	"__TMC_END__",
	"__bss_start",
	"__data_start",
	"_IO_stdin_used",
	"__do_global_dtors_aux",
	"__do_global_dtors_aux_fini_array_entry",
	"__dso_handle",
	"__frame_dummy_init_array_entry",
	"__libc_csu_fini",
	"_edata",
	"_end",
	"_fini",
	"__fini",
	"_init",
	"_start",
	"data_start",
	"deregister_tm_clones",
	"frame_dummy",
	"register_tm_clones",
//"__libc_start_main",
	"__gmon_start__",
	"main"
};

/**
* Meta section header
*/
typedef struct msec_t {
	char *name;
	unsigned long int len;
	unsigned char *data;
	unsigned long int outoffset;
	unsigned int flags;	// See above

	asection *s_bfd;
	Elf_Shdr *s_elf;

	struct msec_t *prev;	// utlist.h
	struct msec_t *next;	// utlist.h

} msec_t;


/**
* Meta segment header
*/
typedef struct mseg_t {
	Elf_Word p_type;
	Elf_Word p_flags;
	Elf_Off p_offset;	// Segment file offset
	Elf_Addr p_vaddr;	// Segment virtual address
	Elf_Addr p_paddr;	// Segment physical address
	Elf_Xword p_filesz;	// Segment size in file
	Elf_Xword p_memsz;	// Segment size in memory
	Elf_Xword p_align;	// Segment alignment, file & memory

	struct msec_t *prev;	// utlist.h
	struct msec_t *next;	// utlist.h

} mseg_t;


typedef struct ctx_t {

	/**
	* Internal options
	*/
	char *binname;
	unsigned int archsz;	// Architecture size (64 or 32)
	unsigned int shnum;
	unsigned int phnum;	// Number of program headers
	char *strndx;		// pointer to section string table in memory
	unsigned int strndx_len;	// length of section string table in bytes
	unsigned int strndx_index;	// offset of sections string table in binary
	unsigned int start_shdrs;	// Offset of section headers in output binary
	unsigned int start_phdrs;	// Offset of Program headers in output binary
	int fdout;
	bfd *abfd;
	unsigned int corefile;	// 1 if file is a core file

	unsigned int base_address;	// VMA Address of first PT_LOAD segment in memory

	// Meta section headers (double linked list)
	msec_t *mshdrs;
	unsigned int mshnum;

	// Meta segment headers (double linked list)
	mseg_t *mphdrs;
	unsigned int mphnum;

	unsigned int has_relativerelocations;	// 1 if binary has relative relocations (R_X86_64_RELATIVE)
	/**
	* User options
	*/
	char *opt_binname;
	char *opt_interp;
	unsigned int opt_arch;
	unsigned int opt_static;
	unsigned int opt_reloc;
	unsigned int opt_strip;
	unsigned int opt_sstrip;
	unsigned int opt_exec;
	unsigned int opt_core;
	unsigned int opt_shared;
	unsigned int opt_verbose;
	unsigned long int opt_entrypoint;
	unsigned char opt_poison;
	unsigned int opt_original;
	unsigned int opt_keep_main;
	unsigned int opt_debug;
	unsigned int opt_asmdebug;
	unsigned int opt_flags;	// used in setting eabi

} ctx_t;

struct symaddr {
	struct symaddr *next;
	char *name;
	int addr;
} *symaddrs;

typedef struct gimport_t {
	char *sname;
	msec_t *sec;
	Elf_Rela *r;
	int rtype;
	unsigned int sindex;
} gimport_t;

gimport_t **gimports = 0;
unsigned int gimportslen = 0;


/**
* Forward prototypes declarations
*/
int craft_section(ctx_t * ctx, msec_t * m);
unsigned int secindex_from_name(ctx_t * ctx, const char *name);
msec_t *section_from_name(ctx_t * ctx, char *name);
msec_t *section_from_addr(ctx_t * ctx, unsigned long int addr);
int print_bfd_sections(ctx_t * ctx);
unsigned int secindex_from_name(ctx_t * ctx, const char *name);
msec_t *section_from_index(ctx_t * ctx, unsigned int index);
unsigned int secindex_from_name_after_strip(ctx_t * ctx, const char *name);
int analyze_text(ctx_t * ctx, char *data, unsigned int datalen, unsigned long int addr);
int save_reloc(ctx_t * ctx, Elf_Rela * r, unsigned int sindex, int has_addend);
int analyze_data(ctx_t *ctx, msec_t *s);
void analyze_problematic_values(ctx_t *ctx);
void scan_all_data_sections(ctx_t *ctx);
int verify_critical_relocations(ctx_t *ctx);
void debug_crash_analysis(ctx_t *ctx);

/**
* Globals
*/
char *globalsymtab = 0;
int globalsymtablen = 0;
unsigned int globalsymtableoffset = 0;

char *globalstrtab = 0;
unsigned int globalstrtablen = 0;
unsigned int globalstrtableoffset = 0;

unsigned int globalsymindex = 0;

char *globalreloc = 0;
unsigned int globalreloclen = 0;
unsigned int globalrelocoffset = 0;

char *globaldatareloc = 0;
unsigned int globaldatareloclen = 0;
unsigned int globaldatarelocoffset = 0;

unsigned int extrasectionnum = 0;

unsigned long int mintext = -1;
unsigned long int maxtext = 0;
unsigned long int textvma = 0;

unsigned long int mindata = -1;
unsigned long int maxdata = 0;
unsigned long int datavma = 0;

unsigned long int orig_text = 0;
unsigned long int orig_sz = 0;

/**
* Convert BFD permissions into regular octal perms
*/
static int parse_bfd_perm(int perm)
{
	int heal_perm = 0;

	heal_perm |= (perm & SEC_CODE ? 1 : 0);
	heal_perm |= (perm & SEC_DATA ? 2 : 0);
	heal_perm |= (perm & SEC_ALLOC ? 4 : 0);
	heal_perm = (perm & SEC_READONLY ? heal_perm : 4);

	return heal_perm;
}

/**
* Convert octal permissions into permissions consumable by mprotect()
*/
unsigned int protect_perms(unsigned int perms)
{
	unsigned int memperms = 0;

	switch (perms) {
	case 7:
		memperms = PROT_READ | PROT_WRITE | PROT_EXEC;
		break;
	case 6:
		memperms = PROT_READ;
		break;
	case 5:
		memperms = PROT_READ | PROT_EXEC;
		break;
	case 4:
		memperms = PROT_READ | PROT_WRITE;
		break;
	default:
		memperms = 0;
		break;
	}
	return memperms;
}

void add_symaddr(ctx_t *ctx, const char *name, int addr, char symclass)
{
	struct symaddr *sa = 0;
	Elf_Sym *s = 0;
	unsigned long int nameptr = 0;
	unsigned int i = 0;

	if (*name == '\0')
		return;

	// search this address in symbol table : duplicates here trigger a NULL ptr dereference in ld
	for (i = 0; i < globalsymtablen / sizeof(Elf_Sym); i++) {
		s = (Elf_Sym *) (globalsymtab + i * sizeof(Elf_Sym));
		//
		if ((s->st_value == addr - textvma) && (s->st_value != 0)) {
			return;	// already in symtab
		}
	}

	// check if name is in blacklist
	for (i = 0; i < sizeof(blnames) / sizeof(char *); i++) {
		if (str_eq(name, blnames[i])) {
			// Keep "main" symbol if user asked so
			if ((!str_eq(name,"main"))||(!ctx->opt_keep_main)){
				return;
			}
		}
	}
	sa = (struct symaddr *) malloc(sizeof(struct symaddr));
	memset(sa, 0, sizeof(struct symaddr));
	sa->name = strdup(name);
	sa->addr = addr;
	sa->next = symaddrs;
	symaddrs = sa;

	if (ctx->opt_verbose) {
		printf("%-20s\t\t%x\t\t%c\n", sa->name, sa->addr, symclass);
	}

	/**
	* Append name to global string table
	*/
	if (globalstrtab == 0) {
		globalstrtab = calloc(1, strlen(sa->name) + 3);
		globalstrtablen++;	// Start with a null byte
	} else {
		globalstrtab = realloc(globalstrtab, globalstrtablen + strlen(sa->name) + 2);
	}
	memcpy(globalstrtab + globalstrtablen, sa->name, strlen(sa->name) + 1);
	nameptr = globalstrtablen;
	globalstrtablen += strlen(sa->name) + 1;

	/**
	* Append symbol to global symbol table
	*/
	if (globalsymtab == 0) {
		globalsymtab = calloc(1, sizeof(Elf_Sym) * 2);
		memset(globalsymtab, 0x00, sizeof(Elf_Sym) * 2);
		globalsymtablen += sizeof(Elf_Sym);	// Skip 1 NULL entry
	} else {
		globalsymtab = realloc(globalsymtab, sizeof(Elf_Sym) + globalsymtablen);
	}

	s = (Elf_Sym *) (globalsymtab + globalsymtablen);
	s->st_name = nameptr;
	s->st_size = 100;	// default function size... (in bytes)
	s->st_value = addr;
	s->st_info = 0;
	s->st_other = 0;
	s->st_shndx = 0;

	switch (symclass) {
	case 'T':
	case 't':

	case 'C':
	case 'c':
		;
		// adjust value from vma
		msec_t *t = section_from_name(ctx, ".text");
		if (ctx->opt_reloc) {
			s->st_value -= t->s_bfd->vma;
			s->st_shndx = 1;	// index to .text
		}
		s->st_info = STT_FUNC;
		break;

	case 'D':
	case 'd':
	case 'B':
	case 'b':
	case 'V':
	case 'v':
		s->st_info = STT_OBJECT;
		break;

	case 'A':
	case 'a':
		s->st_info = STT_FILE;
		break;

	case 'R':
	case 'r':
		s->st_size = 0;
		s->st_info = STT_SECTION;
		break;

	default:
		break;
	}

	if (isupper(symclass)) {
		s->st_info += 0x10;
	}

	globalsymtablen += sizeof(Elf_Sym);
	return;
}

/**
* Add extra symbols
*/
int add_extra_symbols(ctx_t *ctx)
{
	add_symaddr(ctx, "old_plt", textvma, 0x54);
	add_symaddr(ctx, "old_text", orig_text, 0x54);
	add_symaddr(ctx, "old_text_end", orig_text + maxtext - mintext, 0x54);

	return 0;
}

/**
* Read symbol table.
* This is a two stages process : allocate the table, then read it
*/
int rd_symbols(ctx_t *ctx)
{
	long storage_needed = 0;
	asymbol **symbol_table = NULL;
	long number_of_symbols = 0;
	long i = 0;
	int ret = 0;

	const char *sym_name = 0;
	int symclass = 0;
	int sym_value = 0;

	if (ctx->opt_verbose) {
		printf("\n\n -- Reading symbols\n\n");
		printf(" Symbol\t\t\t\taddress\t\tclass\n");
		printf(" -----------------------------------------------------\n");
	}

	/**
	* Process symbol table
	*/
	storage_needed = bfd_get_symtab_upper_bound(ctx->abfd);
	printf("storage_needed: %ld\n", storage_needed);	

	if (storage_needed < 0) {
		bfd_perror(" !! WARNING: bfd_get_symtab_upper_bound");
		ret = 0;
		goto dynsym;
	}
	if (storage_needed == 0) {
		fprintf(stderr, " !! WARNING: no symbols\n");
		goto dynsym;
	}

	if (storage_needed == 1) {
		fprintf(stderr, " !! WARNING: no symbols\n");
		goto dynsym;
	}
	
	symbol_table = (asymbol **) malloc(storage_needed);
	number_of_symbols = bfd_canonicalize_symtab(ctx->abfd, symbol_table);
	if (number_of_symbols < 0) {
		bfd_perror(" !! WARNING: bfd_canonicalize_symtab");
		ret = 0;
		goto dynsym;
	}
	for (i = 0; i < number_of_symbols; i++) {
		asymbol *asym = symbol_table[i];
		sym_name = bfd_asymbol_name(asym);
		symclass = bfd_decode_symclass(asym);
		sym_value = bfd_asymbol_value(asym);
		if (*sym_name == '\0') {
			continue;
		}
		if (bfd_is_undefined_symclass(symclass)) {
			continue;
		}

		if (!ctx->opt_strip) {	// process additional symbols from symbol table
			add_symaddr(ctx, sym_name, sym_value, symclass);
		}
	}

	/**
	* Process dynamic symbol table
	*/
      dynsym:
	if (symbol_table) {
		free(symbol_table);
	}
	symbol_table = NULL;

	if(!ctx->abfd){
		fprintf(stderr, " !! WARNING: no symbols\n");
		goto out;	
	}

	storage_needed = bfd_get_dynamic_symtab_upper_bound(ctx->abfd);
	printf("storage_needed: %ld\n", storage_needed);	
	if (storage_needed < 0) {
		bfd_perror(" !! WARNING: bfd_get_dynamic_symtab_upper_bound");
		ret = 0;
		exit(EXIT_FAILURE);
	}
	if (storage_needed == 0) {
		fprintf(stderr, " !! WARNING: no symbols\n");
		goto out;
	}
	
	if (storage_needed == 1) {
		fprintf(stderr, " !! WARNING: no symbols\n");
		goto dynsym;
	}
	
	symbol_table = (asymbol **) malloc(storage_needed);
	number_of_symbols = bfd_canonicalize_dynamic_symtab(ctx->abfd, symbol_table);
	if (number_of_symbols < 0) {
		bfd_perror(" !! WARNING: bfd_canonicalize_symtab");
		ret = 0;
		goto dynsym;
	}
	for (i = 0; i < number_of_symbols; i++) {
		asymbol *asym = symbol_table[i];
		sym_name = bfd_asymbol_name(asym);
		symclass = bfd_decode_symclass(asym);
		sym_value = bfd_asymbol_value(asym);
		if (*sym_name == '\0') {
			continue;
		}
		if (bfd_is_undefined_symclass(symclass)) {
			continue;
		}
	}
out:
	if (symbol_table) {
		free(symbol_table);
	}

	if (ctx->opt_verbose) {
		printf("\n");
	}

	return ret;
}

/**
* Return section entry size from name
*/
int entszfromname(const char *name)
{
	unsigned int i = 0;

	for (i = 0; i < sizeof(nametosize) / sizeof(assoc_nametosz_t); i++) {
		if (str_eq(nametosize[i].name, name)) {
			return nametosize[i].sz;
		}
	}
	return 0;
}

/**
* Return max of two unsigned integers
*/
unsigned int max(unsigned int a, unsigned int b)
{
	return a < b ? b : a;
}

/**
* Return a section from its name
*/
msec_t *section_from_name(ctx_t *ctx, char *name)
{
	msec_t *s = 0;

	DL_FOREACH(ctx->mshdrs, s) {
		if (str_eq(s->name, name)) {
			return s;
		}
	}
	return 0;
}

/**
* Return a section from its address
*/
msec_t *section_from_addr(ctx_t *ctx, unsigned long int addr)
{
	msec_t *s = 0;

	DL_FOREACH(ctx->mshdrs, s) {
		if ((s->s_bfd->vma) && (s->s_bfd->vma <= addr)
		    && (s->s_bfd->vma + s->s_bfd->size > addr)) {
			return s;
		}
	}
	return 0;
}

/**
* Return a section from its index
*/
msec_t *section_from_index(ctx_t *ctx, unsigned int index)
{
	msec_t *s = 0;
	unsigned int i = 1;	// We count from 1
	DL_FOREACH(ctx->mshdrs, s) {
		if (index == i) {
			return s;
		}
		i++;
	}
	return 0;
}

/**
* Return a section index from its name
*/
unsigned int secindex_from_name(ctx_t *ctx, const char *name)
{
	msec_t *s = 0;
	unsigned int i = 1;	// We count from 1

	DL_FOREACH(ctx->mshdrs, s) {
		if (str_eq(s->name, name)) {
			return i;
		}
		i++;
	}
	return 0;
}

/**
* Return a section index (after strip) from its name
*/
unsigned int secindex_from_name_after_strip(ctx_t *ctx, const char *name)
{
	msec_t *s = 0;
	unsigned int i = 1;	// We count from 1
	unsigned int j = 0;

	DL_FOREACH(ctx->mshdrs, s) {
		if (str_eq(s->name, name)) {
			return i;
		}

		for (j = 0; j < sizeof(allowed_sections) / sizeof(char *); j++) {
			if (str_eq(s->name, allowed_sections[j])) {
				i++;	// Ok, this section is allowed
				break;
			}
		}
	}
	return 0;
}

char *sec_name_from_index_after_strip(ctx_t *ctx, unsigned int index)
{

	msec_t *s = 0;
	unsigned int i = 0;
	unsigned int j = 0;

	DL_FOREACH(ctx->mshdrs, s) {

		for (j = 0; j < sizeof(allowed_sections) / sizeof(char *); j++) {
			if (str_eq(s->name, allowed_sections[j])) {
				i++;	// Ok, this section is allowed
				break;
			}
		}

		if (i == index) {
			return s->name;
		}

	}
	return NULL;
}

/**
* Return a section link from its name
*/
int link_from_name(ctx_t *ctx, const char *name)
{
	unsigned int i = 0;
	char *destsec = 0;
	unsigned int d = 0;

	for (i = 0; i < sizeof(nametolink) / sizeof(assoc_nametolink_t); i++) {
		if (str_eq(nametolink[i].name, name)) {
			destsec = nametolink[i].dst;
		}
	}

	if (!destsec) {
		return 0;
	}

	d = secindex_from_name(ctx, destsec);
	return d;
}

/**
* Return a section info from its name
*/
int info_from_name(ctx_t *ctx, const char *name)
{
	unsigned int i = 0;
	char *destsec = 0;
	unsigned int d = 0;

	for (i = 0; i < sizeof(nametoinfo) / sizeof(assoc_nametoinfo_t); i++) {
		if (str_eq(nametoinfo[i].name, name)) {
			destsec = nametoinfo[i].dst;
		}
	}

	if (!destsec) {
		return 0;
	}

	d = secindex_from_name(ctx, destsec);
	return d;
}

/**
* Return a section type from its name
*/
int typefromname(const char *name)
{
	unsigned int i = 0;

	for (i = 0; i < sizeof(nametotype) / sizeof(assoc_nametotype_t); i++) {
		if (str_eq(nametotype[i].name, name)) {
			return nametotype[i].type;
		}
	}
	return SHT_PROGBITS;
}

/**
* Return a section alignment from its name
*/
unsigned int alignfromname(const char *name)
{
	unsigned int i = 0;

	for (i = 0; i < sizeof(nametoalign) / sizeof(assoc_nametoalign_t); i++) {
		if (str_eq(nametoalign[i].name, name)) {
			return nametoalign[i].alignment;
		}
	}
	return 8;
}

/**
* Return Segment ptype
*/
unsigned int ptype_from_section(msec_t *ms)
{
	// Return type based on section name

	if (str_eq(ms->name, ".interp")) {
		return PT_INTERP;
	}

	if (str_eq(ms->name, ".dynamic")) {
		return PT_DYNAMIC;
	}

	if (str_eq(ms->name, ".eh_frame_hdr")) {
		return PT_GNU_EH_FRAME;
	}

	switch (ms->s_elf->sh_type) {
	case SHT_NULL:
		return PT_NULL;
	case SHT_PROGBITS:
		return PT_LOAD;
	case SHT_NOTE:
		return PT_NOTE;
	case SHT_DYNAMIC:
		return PT_DYNAMIC;
	case SHT_SYMTAB:
	case SHT_STRTAB:
	case SHT_RELA:
	case SHT_HASH:
	case SHT_NOBITS:
	case SHT_REL:
	case SHT_SHLIB:
	case SHT_DYNSYM:
	case SHT_NUM:
	case SHT_LOSUNW:
	case SHT_GNU_verdef:
	case SHT_GNU_verneed:
	case SHT_GNU_versym:
	default:
		break;
	}
	return PT_LOAD;
}

/**
* Return Segment flags based on a section
*/
unsigned int pflag_from_section(msec_t *ms)
{
	unsigned int dperms = 0;
	dperms = 0;

	switch (ms->s_elf->sh_flags) {
	case SHF_ALLOC | SHF_WRITE | SHF_EXECINSTR:
		dperms = PF_R | PF_W | PF_X;	// "rwx";
		break;
	case SHF_ALLOC:
		dperms = PF_R;	//"r--";
		break;
	case SHF_ALLOC | SHF_EXECINSTR:
		dperms = PF_R | PF_X;	// "r-x";
		break;
	case SHF_ALLOC | SHF_WRITE:
		dperms = PF_R | PF_W;	// "rw-"
		break;
	default:
		dperms = 0;	// "---"
		break;
	}
	return dperms;
}

/**
* Helper sort routine for ELF Phdrs (pre-merge)
*/
int phdr_cmp_premerge(mseg_t *a, mseg_t *b)
{
	if (a->p_type != b->p_type) {
		return a->p_type - b->p_type;	// Sort by type
	}
	return a->p_vaddr - b->p_vaddr;	// else by vma
}

/**
* Helper sort routine for ELF Phdrs
*/
int phdr_cmp(mseg_t *a, mseg_t *b)
{
	return a->p_vaddr - b->p_vaddr;	// This is correct, see elf.pdf
}

/**
* Reorganise Program Headers :
* sort by p_offset
*/
int sort_phdrs(ctx_t *ctx)
{
	DL_SORT(ctx->mphdrs, phdr_cmp);
	return 0;
}

/**
* Helper sort routine for ELF Phdrs
*/
int sort_phdrs_premerge(ctx_t *ctx)
{
	DL_SORT(ctx->mphdrs, phdr_cmp_premerge);
	return 0;
}

/**
* Allocate Phdr
*/
mseg_t *alloc_phdr(msec_t *ms)
{
	mseg_t *p = 0;
	Elf_Shdr *s = 0;

	s = ms->s_elf;
	p = calloc(1, sizeof(mseg_t));

	p->p_type = ptype_from_section(ms);
	p->p_flags = pflag_from_section(ms);
	p->p_offset = s->sh_offset;
	p->p_vaddr = s->sh_addr;
	p->p_paddr = s->sh_addr;
	p->p_filesz = s->sh_size;
	p->p_memsz = s->sh_size;
	p->p_align = s->sh_addralign;

	return p;
}

/**
* Create Program Headers based on ELF section headers
*/
int create_phdrs(ctx_t *ctx)
{
	msec_t *ms = 0, *tmp = 0;
	mseg_t *p = 0;

	DL_FOREACH_SAFE(ctx->mshdrs, ms, tmp) {

		p = alloc_phdr(ms);

		if (p->p_type == PT_LOAD) {
			unsigned int r = 0;	// reminder
			p->p_align = 0x200000;
			// We need to align segment p_vaddr - p_offset on page boundaries
			r = (p->p_vaddr - p->p_offset) % 4096;
			p->p_vaddr -= r;	// Adjust initial address
			p->p_paddr -= r;	// Adjust initial address
			p->p_filesz += r;	// Adjust size
			p->p_memsz += r;	// Adjust size
		}

		if (p->p_flags) {
			// Add to linked list of segments
			DL_APPEND(ctx->mphdrs, p);
			ctx->mphnum++;
			ctx->phnum++;
		} else {
			// Sections not mapped have no segment
			free(p);
		}
	}
	return 0;
}

/**
* Merge two consecutive Phdrs if:
* - their vma ranges overlap
* - Permissions match
* - Type of segment matches
*
* Note: assume phdrs have been sorted by increasing p_vaddr first
*/
int merge_phdrs(ctx_t *ctx)
{
	mseg_t *ms = 0, *n = 0;
      retry:
	ms = ctx->mphdrs;
	while (ms) {
		if (ms->next) {
			n = (mseg_t *) ms->next;
			if (ms->p_flags != n->p_flags) {
				goto skipseg;
			}
			if (ms->p_type != n->p_type) {
				goto skipseg;
			}
			// merge sections into the first one :
			// extend section
			ms->p_filesz = n->p_filesz + (n->p_offset - ms->p_offset);
			ms->p_memsz = ms->p_memsz + (n->p_offset - ms->p_offset);
			// unlink deleted section from double linked list
			if (n->next) {
				n->next->prev = (void *) ms;
			}
			ms->next = n->next;
			free(n);
			ctx->mphnum--;
			ctx->phnum--;
			goto retry;

		}
	      skipseg:
		ms = (mseg_t *) ms->next;
	}
	return 0;
}

int adjust_baseaddress(ctx_t *ctx)
{
	mseg_t *ms = 0;

	// find base address (first allocated PT_LOAD chunk)
	ms = ctx->mphdrs;
	while (ms) {
		if ((ms->p_type == PT_LOAD) && (ms->p_flags == (PF_R))) {
			if (ctx->base_address > (ms->p_vaddr & ~0xfff)) {
				ctx->base_address = ms->p_vaddr & ~0xfff;
			}
		}
		ms = (mseg_t *) ms->next;
	}

	if (ctx->base_address == 0) {
		ctx->base_address = ctx->mphdrs->p_vaddr & ~0xfff;
	}

	if (ctx->opt_debug) {
		printf("\n * first loadable segment at: 0x%x\n", ctx->base_address);
	}
	// patch load address of first chunk PT_LOAD allocated RX
	ms = ctx->mphdrs;
	while (ms) {
		if ((ms->p_type == PT_LOAD) && (ms->p_flags == (PF_R | PF_X))) {
			if (ctx->opt_debug) {
				printf(" -- patching base load address of first PT_LOAD Segment: %lu   -->>   %u\n", ms->p_vaddr, ctx->base_address);
			}
			ms->p_vaddr = ctx->base_address;
			ms->p_paddr = ctx->base_address;
			ms->p_memsz += ms->p_offset;
			ms->p_filesz += ms->p_offset;
			ms->p_offset = 0;
			break;
		}
		ms = (void *) ms->next;
	}
	return 0;
}

/**
* Read Program Headers from disk
*/
static unsigned int rd_phdrs(ctx_t *ctx)
{
	struct stat sb;
	char *p = 0;
	int fdin = 0;
	Elf_Ehdr *e = 0;
	unsigned int i = 0;
	int nread = 0;
	Elf_Phdr *phdr = 0, *eph = 0;

	if (stat(ctx->binname, &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	p = calloc(1, sb.st_size);
	fdin = open(ctx->binname, O_RDONLY);
	if (fdin <= 0) {
		perror("open");
		exit(-1);
	}
	nread = read(fdin, p, sb.st_size);
	if (nread != sb.st_size) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	close(fdin);

	printf(" -- read: %u bytes\n", nread);

	e = (Elf_Ehdr *) p;
	phdr = (Elf_Phdr *) (p + e->e_phoff);
	eph = phdr + e->e_phnum;
	for (; phdr < eph; phdr++) {
		// Add to linked list

		// Create Meta section
		mseg_t *ms = calloc(1, sizeof(mseg_t));
		if (!ms) {
			perror("calloc");
			exit(EXIT_FAILURE);
		}

		memcpy(ms, phdr, sizeof(Elf_Phdr));

		// Add to double linked list of msec_t Meta sections
		DL_APPEND(ctx->mphdrs, ms);
		ctx->mphnum++;
		ctx->phnum++;
		i++;
	}

	printf(" -- Original: %u\n", i);
	return 0;
}

/**
* Create Program Headers from Sections
*/
static unsigned int mk_phdrs(ctx_t *ctx)
{
	/**
	* Create a segment per section
	*/
	create_phdrs(ctx);

	/**
	* Sort segments for merging
	*/
	sort_phdrs_premerge(ctx);

	/**
	* Merge segments with overlapping/consecutive memory chunks
	*/
	merge_phdrs(ctx);

	sort_phdrs(ctx);

	adjust_baseaddress(ctx);

	sort_phdrs(ctx);	// Need to resort after patching
	merge_phdrs(ctx);
	sort_phdrs(ctx);	// Need to resort after patching

	return 0;
}

/**
* Write Program Headers to disk
*/
static unsigned int write_phdrs(ctx_t *ctx)
{
	unsigned int tmpm = 0;

	// Goto end of file, align on 8 bytes boundaries
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	write(ctx->fdout, nullstr, 20);
	if ((tmpm % 8) == 0) {
		tmpm += 8;
	}
	tmpm &= ~0xf;
	tmpm += sizeof(Elf_Phdr);	// Prepend NULL section
	ftruncate(ctx->fdout, tmpm);

	ctx->start_phdrs = lseek(ctx->fdout, 0x00, SEEK_END);

	ctx->phnum += 2;

	if (ctx->opt_verbose) {
		printf(" -- Writting %u segment headers\n", ctx->phnum);
	}
	// first entry is the program header itself
	Elf_Phdr *phdr = calloc(1, sizeof(Elf_Phdr));
	phdr->p_vaddr = ctx->base_address;
	phdr->p_paddr = ctx->base_address;
	phdr->p_type = PT_PHDR;
	phdr->p_offset = ctx->start_phdrs;
	phdr->p_flags = 5;
	phdr->p_filesz = ctx->phnum * sizeof(Elf_Phdr);
	phdr->p_memsz = ctx->phnum * sizeof(Elf_Phdr);
	phdr->p_align = 8;
	write(ctx->fdout, phdr, sizeof(Elf_Phdr));

	// Copy all the Phdrs
	mseg_t *p;
	DL_FOREACH(ctx->mphdrs, p) {
		write(ctx->fdout, p, sizeof(Elf_Phdr));
	}

	// Append a Program Header for the stack
	phdr->p_vaddr = 0;
	phdr->p_paddr = 0;
	phdr->p_type = PT_GNU_STACK;
	phdr->p_offset = 0;
	phdr->p_flags = 3;
	phdr->p_filesz = 0;
	phdr->p_memsz = 0;
	phdr->p_align = 0x10;
	write(ctx->fdout, phdr, sizeof(Elf_Phdr));

	return ctx->start_phdrs;
}

/**
* Write Original Program Headers to disk
*/
static unsigned int write_phdrs_original(ctx_t *ctx)
{
	unsigned int tmpm = 0;

	// Goto end of file, align on 8 bytes boundaries
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	write(ctx->fdout, nullstr, 20);
	if ((tmpm % 8) == 0) {
		tmpm += 8;
	}
	tmpm &= ~0xf;

	ftruncate(ctx->fdout, tmpm);

	ctx->start_phdrs = lseek(ctx->fdout, 0x00, SEEK_END);

	mseg_t *p;
	unsigned int i = 0;
	DL_FOREACH(ctx->mphdrs, p) {
		if (i == 0) {	// First Phdr is the Program Header itself
			p->p_offset = ctx->start_phdrs;	// Patch offset of Program header
			i = 1;
		}
		write(ctx->fdout, p, sizeof(Elf_Phdr));
	}
	return ctx->start_phdrs;
}

msec_t *mk_section(void)
{
	msec_t *ms = 0;

	// allocate memory
	ms = calloc(1, sizeof(msec_t));
	if (!ms) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	ms->s_elf = calloc(1, sizeof(Elf_Shdr));
	if (!ms->s_elf) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	return ms;
}

static int write_strtab_and_reloc(ctx_t *ctx)
{
	unsigned int tmpm = 0;

	if (ctx->opt_debug) {
		printf(" * .strtab length:\t\t\t%u\n", globalstrtablen);
		printf(" * .symtab length:\t\t\t%u\n", globalsymtablen);
	}
	// Goto end of file
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	write(ctx->fdout, nullstr, 20);
	// align on 8 bytes boundaries
	if ((tmpm % 8) == 0) {
		tmpm += 8;
	};
	tmpm &= ~0xf;
	// truncate
	ftruncate(ctx->fdout, tmpm);

	// write .text relocations to binary
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	globalrelocoffset = tmpm;
	write(ctx->fdout, globalreloc, globalreloclen);

	// write .data relocations to binary
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	globaldatarelocoffset = tmpm;
	write(ctx->fdout, globaldatareloc, globaldatareloclen);

	// write string table to binary
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	globalstrtableoffset = tmpm;
	write(ctx->fdout, globalstrtab, globalstrtablen);

	// write symbol table to binary
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	globalsymtableoffset = tmpm;
	memset(globalsymtab, 0x00, sizeof(Elf_Sym));	// Make sure first entry is NULL	
	write(ctx->fdout, globalsymtab, globalsymtablen);

	return 0;
}


char *symclass(char sclass)
{
	switch(sclass){
	case 0:
		return "local";
		break;
	case 1:
		return "global";
		break;
	case 2:
		return "weak";
		break;

	case 10:
		return "loos";
		break;

	case 12:
		return "hios";
		break;

	case 13:
		return "loproc";
		break;

	case 15:
		return "hiproc";
		break;

	default:
		return "unknown";
		break;
	}
}

/**
* Ensure symtab doesn't feature locals after strtab->sh_info value
* Transform non compliant locals into XXX
*/
static int fix_symtab_bindings(ctx_t *ctx)
{
	char *sname = 0;
	Elf_Sym *s = 0;
	unsigned int i = 0, sindex = 0;
	char sclass = 0;

	if (!globalsymtab) {
		return 0;
	}

	if (!globalstrtab) {
		return 0;
	}


      	if (ctx->opt_debug) {
	      	printf("\n-- Displaying final symbols\n");
	}

	for (sindex = maxnewsec + 1 + extrasectionnum; sindex < (globalsymtablen / sizeof(Elf_Sym)); sindex++) {

		// get symbol name from index
		s = globalsymtab + sindex * sizeof(Elf_Sym);
      		sname = (char *) (globalstrtab + s->st_name);
                sclass = GELF_ST_BIND(s->st_info);

		/**
		* Symbols with an index >= strtab->sh_info cannot be local symbols. Make them weak instead
		*/
		if (sclass == 0){
			s->st_info += 0x20;
			sclass = GELF_ST_BIND(s->st_info);
		}

	      	if (s->st_name < globalstrtablen) {
			if (ctx->opt_debug) {
      				printf(" * name: %s at index %u info: %x class: %s\n", sname, sindex, s->st_info, symclass(sclass));
			}
      		}
	}

	return 0;
}

char *reloc_htype_x86_64(int thetype)
{
	char *htype = 0;

	switch (thetype) {
	case R_X86_64_NONE:
		htype = "R_X86_64_NONE";
		break;

	case R_X86_64_64:
		htype = "R_X86_64_64";
		break;

	case R_X86_64_32:
		htype = "R_X86_64_32";
		break;

	case R_X86_64_32S:
		htype = "R_X86_64_32S";
		break;

	case R_X86_64_PC32:
		htype = "R_X86_64_PC32";
		break;

	case R_X86_64_GOT32:
		htype = "R_X86_64_GOT32";
		break;

	case R_X86_64_PLT32:
		htype = "R_X86_64_PLT32";
		break;

	case R_X86_64_COPY:
		htype = "R_X86_64_COPY";
		break;

	case R_X86_64_GLOB_DAT:
		htype = "R_X86_64_GLOB_DAT";
		break;

	case R_X86_64_JUMP_SLOT:
		htype = "R_X86_64_JUMP_SLOT";
		break;

	case R_X86_64_RELATIVE:
		htype = "R_X86_64_RELATIVE";
		break;

	case R_X86_64_GOTPCREL:
		htype = "R_X86_64_GOTPCREL";
		break;

	case R_X86_64_16:
		htype = "R_X86_64_16";
		break;

	case R_X86_64_PC16:
		htype = "R_X86_64_PC16";
		break;

	case R_X86_64_8:
		htype = "R_X86_64_8";
		break;

	case R_X86_64_PC8:
		htype = "R_X86_64_PC8";
		break;

	case R_X86_64_NUM:
		htype = "R_X86_64_NUM";
		break;

	default:
		htype = "Unknown";
		break;
	}

	return htype;
}

char *reloc_htype_x86_32(int thetype)
{
	char *htype = 0;

	switch (thetype) {
	case R_386_NONE:
		htype = "R_386_NONE";
		break;

	case R_386_32:
		htype = "R_386_32";
		break;

	case R_386_PC32:
		htype = "R_386_PC32";
		break;

	case R_386_GOT32:
		htype = "R_386_GOT32";
		break;

	case R_386_PLT32:
		htype = "R_386_PLT32";
		break;

	case R_386_COPY:
		htype = "R_386_COPY";
		break;

	case R_386_GLOB_DAT:
		htype = "R_386_GLOB_DAT";
		break;

	case R_386_JMP_SLOT:
		htype = "R_386_JMP_SLOT";
		break;

	case R_386_RELATIVE:
		htype = "R_386_RELATIVE";
		break;

	case R_386_GOTOFF:
		htype = "R_386_GOTOFF";
		break;

	case R_386_GOTPC:
		htype = "R_386_GOTPC";
		break;

	case R_386_NUM:
		htype = "R_386_NUM";
		break;

	default:
		htype = "Unknown";
		break;
	}

	return htype;
}

char *reloc_htype(int thetype)
{
	switch (RELOC_MODE) {
	case RELOC_X86_64:
		return reloc_htype_x86_64(thetype);
	case RELOC_X86_32:
		return reloc_htype_x86_32(thetype);
	default:
		return "UNKNOWN_RELOCATION";
		break;
	}
}

/**
* Parse relocations from a given section
*/
static int parse_reloc(ctx_t *ctx, msec_t *s)
{
	Elf_Shdr *shdr = 0;
	Elf_Rela *r = 0;
	unsigned int sz = 0;
	unsigned int i = 0;
	char *htype = 0;
	unsigned int sindex = 0;
	unsigned int has_addend = 0;

	shdr = s->s_elf;
	sz = shdr->sh_size;

	has_addend = (shdr->sh_type == SHT_RELA) ? 1 : 0;	// SHT_RELA has addends, SHT_REL doesn't

	printf(" ** relocations in section: %s\n",s->name);

	if ((shdr->sh_type == SHT_RELA)
	    && ((int) shdr->sh_entsize != entszfromname(".rela.dyn"))) {
		printf(" !! WARNING: strange relocation size: %lu != %u in %s\n", shdr->sh_entsize, entszfromname(".rela.dyn"), s->name);
		return -1;
	}

	if ((shdr->sh_type == SHT_REL)
	    && ((int) shdr->sh_entsize != entszfromname(".rel.dyn"))) {
		printf(" !! WARNING: strange relocation size: %lu != %u in %s\n", shdr->sh_entsize, entszfromname(".rel.dyn"), s->name);
		return -1;
	}

	if ((shdr->sh_type == SHT_RELA)
	    && ((int) shdr->sh_size % entszfromname(".rela.dyn"))) {
		printf(" !! WARNING: strange relocation section size: %lu not a multiple of %u in %s\n", shdr->sh_size, entszfromname(".rela.dyn"), s->name);
		return -1;
	}

	if ((shdr->sh_type == SHT_REL) && (shdr->sh_size % entszfromname(".rel.dyn"))) {
		printf(" !! WARNING: strange relocation section size: %lu not a multiple of %u in %s\n", shdr->sh_size, entszfromname(".rel.dyn"), s->name);
		return -1;
	}

	if (ctx->opt_verbose) {
		printf("\t%s\tsize:%u\t%lu relocations\n", s->name, sz, sz / shdr->sh_entsize);
	}

	for (i = 0; i < sz / shdr->sh_entsize; i++) {
		r = s->data + i * shdr->sh_entsize;

		htype = reloc_htype(ELF_R_TYPE(r->r_info));

		if (ELF_R_TYPE(r->r_info) == R_X86_64_RELATIVE) {
			if (ctx->opt_debug) {
				printf("reloc[%u] %016lx\t%lu\t%s\t%u\taddend:%lx\t\n", i, r->r_offset, r->r_info, htype, sindex, r->r_addend);
				printf(" * Skipping relative relocation\n");
			}
			ctx->has_relativerelocations = 1;	// Binary has relative relocations
			continue;	// Do not save relocation for R_X86_64_RELATIVE
		} else {
			sindex = ELF_R_SYM(r->r_info);

			sindex += maxnewsec;	// account for section references at top of symbol table

			// write back sindex in symtab
			ELF_R_INFO(sindex, ELF_R_TYPE(r->r_info));
		}

		if (ctx->opt_verbose) {
			printf("reloc[%u] %016lx\t%lu\t%s\t%u\taddend:%lu\t\n", i, r->r_offset, r->r_info, htype, sindex, r->r_addend);
		}

		save_reloc(ctx, r, sindex, has_addend);
	}

	if (ctx->opt_verbose) {
		printf("\n");
	}
	return 0;
}

int fixup_strtab_and_symtab(ctx_t *ctx)
{
	char *sname = 0;
	Elf_Sym *s = 0;
	unsigned int i = 0, sindex = 0;

	if (!globalsymtab) {
		return 0;
	}

	if (!globalstrtab) {
		return 0;
	}

	if (ctx->opt_debug) {
		printf("\n -- Fixing strtab and symtab with delta of %u\n\n", deltastrtab);
	}

	for (sindex = maxnewsec + 1 + extrasectionnum; sindex < (globalsymtablen / sizeof(Elf_Sym)); sindex++) {

		// get symbol name from index
		s = globalsymtab + sindex * sizeof(Elf_Sym);

		s->st_name += deltastrtab;	// fix symbol name offset in symbol table

		/**
		* check if name is in blacklist, if so, rename it by prepending "old_" to symbol name
		*/
		for (i = 0; i < sizeof(blnames) / sizeof(char *); i++) {
			sname = (char *) (globalstrtab + s->st_name);

			// Don't rename "main" if opt_keep_main is set			
			if ((ctx->opt_keep_main)&&(str_eq(sname, "main"))) {
				printf(" * keeping symbol : %s\n", blnames[i]);
				continue;
			}
			
			if ((s->st_name < globalstrtablen) && (str_eq(sname, blnames[i]))) {
				if (ctx->opt_debug) {
					printf(" * name blacklisted: %s at index %u\n", sname, sindex);
				}

				// generate new symbol name from old one
				globalstrtab = realloc(globalstrtab, globalstrtablen + strlen(sname) + 5);
				sprintf(globalstrtab + globalstrtablen, "old_%s", sname);
				s->st_name = globalstrtablen;
				globalstrtablen += strlen(globalstrtab + globalstrtablen) + 1;

				// change type and link information
				s->st_info = ELF64_ST_INFO(STB_WEAK, STT_NOTYPE);
				s->st_shndx = 0;
			}
		}

		/**
		* Adjust symbol addresses 
		* For addresses within the .TEXT segment: substract base load address of .TEXT segment
		* For addresses within the .DATA segment: substract base load address of .DATA segment
		*/
		if ((s->st_value)&&(s->st_value >= mintext)&&(s->st_value <= maxtext)) {
			printf(" * adjusting .TEXT symbol: %s\n", (char *) (globalstrtab + s->st_name));
			s->st_value -= textvma;
//		} else if ((s->st_value)&&(s->st_value >= mindata)&&(s->st_value <= maxdata)) {
		} else if(s->st_value){
			printf(" * adjusting .DATA symbol: %s\n", (char *) (globalstrtab + s->st_name));
			s->st_value -= datavma;
		} else if(!s->st_value){
			printf(" * no adjustment symbol: %s\n", (char *) (globalstrtab + s->st_name));		
		}

	}
	return 0;
}

int fixup_text(ctx_t *ctx)
{
	msec_t *s = 0;

	if (ctx->opt_debug) {
		printf(" -- Fixup .text\n\n");
	}

	DL_FOREACH(ctx->mshdrs, s) {
		if (str_eq(s->name, ".text")) {
			unsigned int newsz = datavma - textvma + maxdata - mindata;
			if (ctx->opt_debug) {
				printf(" * .text section found, increasing size from 0x%lx to 0x%x (0x%lx)\n", s->s_elf->sh_size, newsz, s->len);
			}
			s->s_elf->sh_size = newsz;
			// extend data
			s->data = realloc(s->data, newsz);
			if (!s->data) {
				printf(" ERROR: realloc() %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			if (ctx->opt_debug) {
				printf("newsize:%x s->len:%lx\n", newsz, s->len);
			}

			// pad
			memset(s->data + s->len, 0x00, newsz - s->len);
			s->s_elf->sh_offset = orig_sz;
			s->outoffset = orig_sz;
			// extend output file
			ftruncate(ctx->fdout, s->outoffset + s->s_elf->sh_size);
			s->len = newsz;
			break;
		}
	}
	return 0;
}

/**
* Parse relocations
*/
static unsigned int parse_relocations(ctx_t *ctx)
{
	msec_t *s = 0;

	if (ctx->opt_verbose) {
		printf("\n -- Parsing existing relocations\n\n");
	}
	DL_FOREACH(ctx->mshdrs, s) {
		if ((s->s_elf) && (s->s_elf->sh_type == SHT_RELA)) {	// relocations with addends
			parse_reloc(ctx, s);
		} else if ((s->s_elf) && (s->s_elf->sh_type == SHT_REL)) {	// relocations without addends
			parse_reloc(ctx, s);
		}
	}

	if (ctx->opt_verbose) {
		printf("\n");
	}

	return 0;
}

/**
* Append a symbol to global symbol table
*/
unsigned int append_sym(Elf_Sym *s)
{
		printf(" -- global symtab length: %u\n", globalsymtablen);

	if (globalsymtab == 0) {
		globalsymtab = calloc(1, sizeof(Elf_Sym) * 2);
		memset(globalsymtab, 0x00, sizeof(Elf_Sym) * 2);
		globalsymtablen += sizeof(Elf_Sym);	// Skip 1 NULL entry
	} else {
		globalsymtab = realloc(globalsymtab, sizeof(Elf_Sym) + globalsymtablen);
	}

	memcpy(globalsymtab + globalsymtablen, s, sizeof(Elf_Sym));

	globalsymtablen += sizeof(Elf_Sym);

	return 0;

}

/**
* Append a string to symbol table, reports offset in strtab where this symbol will start
*/
unsigned int append_strtab(char *str)
{
	unsigned int nameptr = 0;

	if (globalstrtab == 0) {
		globalstrtab = calloc(1, strlen(str) + 3);
		globalstrtablen++;	// Start with a null byte
	} else {
		globalstrtab = realloc(globalstrtab, globalstrtablen + strlen(str) + 2);
	}
	memcpy(globalstrtab + globalstrtablen, str, strlen(str) + 1);
	nameptr = globalstrtablen;
	globalstrtablen += strlen(str) + 1;

	return nameptr;
}

/**
* Create sections in symbol table/string table
*/
static int create_section_symbols(ctx_t *ctx)
{
	msec_t *tmp = 0;
	msec_t *s = 0;
	unsigned int nameptr = 0;
	Elf_Sym *sym = 0;
	unsigned int i = 0, n = 0;

	sym = calloc(1, sizeof(Elf_Sym));

	DL_COUNT(ctx->mshdrs, tmp, maxoldsec);	// count before stripping

	for (i = 1; i <= maxoldsec; i++) {
		s = section_from_index(ctx, i);
		n = secindex_from_name_after_strip(ctx, s->name);
		if (n > maxnewsec) {
			maxnewsec = n;
		}
	}

//      maxnewsec += EXTRA_CREATED_SECTIONS;    // count sections not yet created
	if (ctx->opt_debug) {
		printf(" -- Max section index after stripping: %u\n", maxnewsec);
	}

	// Create symbol for existing sections
	for (i = 1; i <= maxnewsec; i++) {

		char *newsname = 0;

		newsname = sec_name_from_index_after_strip(ctx, i);
		if (!newsname) {

			switch (i - EXTRA_CREATED_SECTIONS - 1) {
			case 1:
				newsname = ".rela.all";
				break;
			case 2:
				newsname = ".strtab";
				break;
			case 3:
				newsname = ".symtab";
				break;
			case 4:
				newsname = ".shstrtab";
				break;

			default:
				newsname = ".unknown";
				break;

			}
		}

		nameptr = append_strtab(newsname);

		if (ctx->opt_debug) {
			printf(" [%02u] %s\n", i, newsname);
		}

		sym->st_name = nameptr;
		sym->st_size = 0;
		sym->st_value = 0;
		sym->st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
		sym->st_other = 0;
		sym->st_shndx = i;

		append_sym(sym);
	}

	free(sym);

	printf("\n");

	deltastrtab = globalstrtablen;

/*	if (ctx->opt_debug) {
		printf(" -- Base sections symbol index: %u\n", 0);
		printf(" -- Delta string table: %u\n", deltastrtab);
	}
*/
	return 0;
}

static unsigned int process_text(ctx_t *ctx)
{
	msec_t *t = 0;
	unsigned int delta = 0;

	t = section_from_name(ctx, ".text");
	delta = orig_text - textvma;

	// parse text for relocations
	analyze_text(ctx, (char *) (t->data + delta), maxtext - mintext - delta, orig_text);

	return 0;
}

static unsigned int process_data(ctx_t *ctx)
{
	msec_t *t = 0;

	t = section_from_name(ctx, ".data");

	// parse .data and re-create missing relocations
	analyze_data(ctx, t);

	return 0;
}

/**
* Create Section Headers
*/
static unsigned int write_shdrs(ctx_t *ctx)
{
	Elf_Shdr *shdr = 0;
	unsigned int tmpm = 0;
	msec_t *s = 0;

	msec_t *tmp = 0;
	unsigned int maxsec = 0;

	// Get final number of sections
	DL_COUNT(ctx->mshdrs, tmp, maxsec);	// count sections

	/**
	* Align section headers on 8 bytes boundaries
	*/
	// Goto end of file
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	write(ctx->fdout, nullstr, 20);
	write(ctx->fdout, nullstr, 20);
	write(ctx->fdout, nullstr, 20);
	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	// align on 8 bytes boundaries
	if ((tmpm % 8) == 0) {
		tmpm += 8;
	};
	tmpm &= ~0xf;
	tmpm += sizeof(Elf_Shdr);	// Prepend a NULL section
	// truncate
	ftruncate(ctx->fdout, tmpm);

	ctx->start_shdrs = lseek(ctx->fdout, 0x00, SEEK_END) - sizeof(Elf_Shdr);	// New start of SHDRs
	ctx->strndx[0] = 0;
	ctx->strndx_len = 1;

	/**
	* Write each ELF section header
	*/
	DL_FOREACH(ctx->mshdrs, s) {

		// append name to strndx
		memcpy(ctx->strndx + ctx->strndx_len, s->name, strlen(s->name) + 1);	// do copy the final "\x00"
		s->s_elf->sh_name = ctx->strndx_len;
		ctx->strndx_len += strlen(s->name) + 1;

		// adjust .text permissions
		if(!strncmp(s->name, ".text", 6)){
				s->s_elf->sh_flags = 6;
		}

		// adjust section links and info
		s->s_elf->sh_link = link_from_name(ctx, s->name);	// Link to another section
		s->s_elf->sh_info = info_from_name(ctx, s->name);	// Additional section information

		// write section header to binary
		write(ctx->fdout, s->s_elf, sizeof(Elf_Shdr));
	}

	/**
	* Add a section header for a .note.GNU-stack section
	*/

	// append name to strndx
	memcpy(ctx->strndx + ctx->strndx_len, ".note.GNU-stack", 16);

	tmpm = lseek(ctx->fdout, 0x00, SEEK_END);
	
	shdr = calloc(1, sizeof(Elf_Shdr));

	shdr->sh_name = ctx->strndx_len;	// index in string table
	shdr->sh_type = SHT_PROGBITS;	// Section type
	shdr->sh_flags = 0;	// Section flags
	shdr->sh_addr = 0;	// Section virtual addr at execution
	shdr->sh_offset = tmpm;//globalrelocoffset;	// Section file offset
	shdr->sh_size = 0;	//globalreloclen;	// Section size in bytes
	shdr->sh_link = 0;	// Link to another section
	shdr->sh_info = 0;
	shdr->sh_addralign = 1;	// Section alignment
	shdr->sh_entsize = 0;

	ctx->strndx_len += 16;

	// append string table section header to binary
	write(ctx->fdout, shdr, sizeof(Elf_Shdr));
	free(shdr);
ctx->shnum++;
maxsec++;
	ctx->strndx_index = ctx->shnum + 1;
	// append sections string table to binary
	//write(ctx->fdout, ctx->strndx, ctx->strndx_len);


	/**
	* Add a section header for relocations
	*/
	// append name to strndx
	memcpy(ctx->strndx + ctx->strndx_len, ".rela.all", 10);

	shdr = calloc(1, sizeof(Elf_Shdr));

	shdr->sh_name = ctx->strndx_len;	// index in string table
	shdr->sh_type = SHT_RELA;	// Section type
	shdr->sh_flags = 2;	// Section flags
	shdr->sh_addr = 0;	// Section virtual addr at execution
	shdr->sh_offset = globalrelocoffset;	// Section file offset
	shdr->sh_size = globalreloclen;	// Section size in bytes
	shdr->sh_link = ctx->shnum + 4;	// Link to another section
	shdr->sh_info = 1;	// Additional section information  : .text
	shdr->sh_addralign = 8;	// Section alignment
	shdr->sh_entsize = entszfromname(".rela.plt");	// Entry size if section holds table

	ctx->strndx_len += 10;

	// append string table section header to binary
	write(ctx->fdout, shdr, sizeof(Elf_Shdr));
	free(shdr);

	ctx->strndx_index = ctx->shnum + 1;
	// append sections string table to binary
	//  write(ctx->fdout, ctx->strndx, ctx->strndx_len);


	/**
	* Add a section header for .data relocations
	*/
	
	// append name to strndx
	memcpy(ctx->strndx + ctx->strndx_len, ".data.rel", 10);

	shdr = calloc(1, sizeof(Elf_Shdr));

	shdr->sh_name = ctx->strndx_len;	// index in string table
	shdr->sh_type = SHT_RELA;	// Section type
	shdr->sh_flags = 2;	// Section flags
	shdr->sh_addr = 0;	// Section virtual addr at execution
	shdr->sh_offset = globaldatarelocoffset;	// Section file offset
	shdr->sh_size = globaldatareloclen;	// Section size in bytes
	shdr->sh_link = ctx->shnum + 4;	// Link to another section
	shdr->sh_info = 3;	// Additional section information  : .data
	shdr->sh_addralign = 8;	// Section alignment
	shdr->sh_entsize = entszfromname(".rela.plt");	// Entry size if section holds table

	ctx->strndx_len += 10;

	// append string table section header to binary
	write(ctx->fdout, shdr, sizeof(Elf_Shdr));
	free(shdr);

	ctx->shnum++;
	maxsec++;

	ctx->strndx_index = ctx->shnum + 1;
	// append sections string table to binary
	//write(ctx->fdout, ctx->strndx, ctx->strndx_len);




	/**
	* Add a section header for string table
	*/
	// append name to strndx
	memcpy(ctx->strndx + ctx->strndx_len, ".strtab", 8);

	shdr = calloc(1, sizeof(Elf_Shdr));

	shdr->sh_name = ctx->strndx_len;	// index in string table
	shdr->sh_type = SHT_STRTAB;	// Section type
	shdr->sh_flags = 0;	// Section flags
	shdr->sh_addr = 0;	// Section virtual addr at execution
	shdr->sh_offset = globalstrtableoffset;	// Section file offset
	shdr->sh_size = globalstrtablen;	// Section size in bytes
	shdr->sh_link = 0;	// Link to another section
	shdr->sh_info = 0;	// Additional section information
	shdr->sh_addralign = 1;	// Section alignment
	shdr->sh_entsize = entszfromname(".strtab");	// Entry size if section holds table

	ctx->strndx_len += 8;

	// append string table section header to binary
	write(ctx->fdout, shdr, sizeof(Elf_Shdr));
	free(shdr);

	ctx->strndx_index = ctx->shnum + 1;


	/**
	* Add a section header for symbol table
	*/
	// append name to strndx
	memcpy(ctx->strndx + ctx->strndx_len, ".symtab", 8);

	shdr = calloc(1, sizeof(Elf_Shdr));

	shdr->sh_name = ctx->strndx_len;	// index in string table
	shdr->sh_type = SHT_SYMTAB;	// Section type
	shdr->sh_flags = 0;	// Section flags
	shdr->sh_addr = 0;	// Section virtual addr at execution
	shdr->sh_offset = globalsymtableoffset;	// Section file offset
	shdr->sh_size = globalsymtablen;	// Section size in bytes
	shdr->sh_link = maxsec + 2;	// Link to another section (strtab)
	shdr->sh_info = maxsec + 2;	// Additional section information
	shdr->sh_addralign = 8;	// Section alignment
	shdr->sh_entsize = entszfromname(".symtab");	// Entry size if section holds table

	ctx->strndx_len += 8;

	// append string table section header to binary
	write(ctx->fdout, shdr, sizeof(Elf_Shdr));
	free(shdr);

	ctx->strndx_index = ctx->shnum + 1;

	/**
	* Append an additional section header for the Section header string table
	*/

	// append name to strndx
	memcpy(ctx->strndx + ctx->strndx_len, ".shstrtab", 10);

	shdr = calloc(1, sizeof(Elf_Shdr));

	shdr->sh_name = ctx->strndx_len;	// index in string table
	shdr->sh_type = SHT_STRTAB;	// Section type
	shdr->sh_flags = 0;	// Section flags
	shdr->sh_addr = 0;	// Section virtual addr at execution
	shdr->sh_offset = lseek(ctx->fdout, 0x00, SEEK_END) + sizeof(Elf_Shdr);	// Section file offset
	shdr->sh_size = ctx->strndx_len + 10;	// Section size in bytes
	shdr->sh_link = 0;	// Link to another section
	shdr->sh_info = 0;	// Additional section information
	shdr->sh_addralign = 1;	// Section alignment
	shdr->sh_entsize = 0;	// Entry size if section holds table

	ctx->strndx_len += 9 + 1;

	// append string table section header to binary
	write(ctx->fdout, shdr, sizeof(Elf_Shdr));
	free(shdr);

	ctx->strndx_index = ctx->shnum + 1;
	// append sections strint table to binary
	write(ctx->fdout, ctx->strndx, ctx->strndx_len);

	if (ctx->opt_debug) {
		printf(" * section headers at:\t\t\t0x%x\n", ctx->start_shdrs);
		printf(" * section string table index:\t\t%u\n", ctx->shnum);
	}
	return ctx->start_shdrs;
}

/**
* Create ELF Headers
*/
static int mk_ehdr(ctx_t *ctx)
{
	Elf_Ehdr *e = 0;

	e = calloc(1, sizeof(Elf_Ehdr));
	if (!e) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	/**
	* Set defaults
	*/

	// Set ELF signature
	memcpy(e->e_ident, "\x7f\x45\x4c\x46\x02\x01\x01", 7);
	e->e_ident[EI_CLASS] = ELFCLASS;	// 64 or 32 bits
	e->e_entry = bfd_get_start_address(ctx->abfd);
	// Set type of ELF based on command line options and compilation target
	e->e_type = ET_DYN;	// Default is shared library
	e->e_machine = ctx->opt_arch ? ctx->opt_arch : ELFMACHINE;	// Default : idem compiler cpu, else, user specified
	e->e_version = 0x1;	// ABI Version, Always 1
	e->e_phoff = ctx->start_phdrs;
	e->e_shoff = ctx->start_shdrs;
	e->e_flags = ctx->opt_flags;	// default is null. Used in setting some EABI versions on ARM
	e->e_ehsize = sizeof(Elf_Ehdr);	// Size of this header
	e->e_phentsize = sizeof(Elf_Phdr);	// Size of each program header
	e->e_phnum = ctx->phnum;
	e->e_shentsize = sizeof(Elf_Shdr);	// Size of each section header
	e->e_shnum = ctx->shnum + 5;	// We added a null section and a string table index + .strtab + .symtab + .rela.all
	e->e_shstrndx = ctx->shnum + 4;	// Sections Seader String table index is last valid

	/**
	* Now apply options
	*/
	if (ctx->opt_sstrip) {
		e->e_shoff = 0;
		e->e_shnum = 0;
		e->e_shstrndx = 0;	// Sections Seader String table index is last valid
		e->e_shentsize = 0;
	}

	if ((ctx->opt_exec) || (ctx->opt_static)) {
		e->e_type = ET_EXEC;	// Executable
	}
	if (ctx->opt_shared) {
		e->e_type = ET_DYN;	// Shared library
	}
	if (ctx->opt_reloc) {
		e->e_type = ET_REL;	// Relocatable object
		e->e_entry = 0;
		e->e_phoff = 0;
		e->e_phnum = 0;
		e->e_phentsize = 0;
	}

	if (ctx->opt_core) {
		e->e_type = ET_CORE;	// Core file
	}
	// write ELF Header
	lseek(ctx->fdout, 0x00, SEEK_SET);
	write(ctx->fdout, e, sizeof(Elf_Ehdr));
	return 0;
}

/**
* Write a section to disk
*/
static int write_section(ctx_t *ctx, msec_t *m)
{
	unsigned int nwrite = 0;

	// Go to correct offset in output binary
	lseek(ctx->fdout, m->outoffset, SEEK_SET);

	// write to fdout
	nwrite = write(ctx->fdout, m->data, m->len);
	if (nwrite != m->len) {
		printf("write failed: %u != %lu %s\n", nwrite, m->len, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return nwrite;
}

static int rd_extended_text(ctx_t *ctx)
{
	unsigned int i = 0;
	asection *s = 0;

	if (ctx->opt_debug) {
		printf(" -- Finding .text segment boundaries\n\n");

		printf(" index\t\tname\t\t\trange\tsize\tpermissions\toffset\n");
		printf("--------------------------------------------------------------------\n");
	}
	s = ctx->abfd->sections;
	for (i = 0; i < ctx->shnum; i++) {
		unsigned perms = parse_bfd_perm(s->flags);
		if (perms == 5) {
			if (ctx->opt_debug) {
				printf(" [%2u] \t%-20s\t%012lx-%012lx %lu\t%s\t%lu\n", i + 1, s->name, s->vma, s->vma + s->size, s->size, "RX", s->filepos);
			}

			if ((unsigned long) s->filepos < mintext) {
				mintext = s->filepos;
				textvma = s->vma;
			}
			if ((unsigned long) s->filepos + s->size > maxtext) {
				maxtext = s->filepos + s->size;
			}
		}
		s = s->next;
	}

	if (ctx->opt_debug) {
		printf(" --> .text future boundaries: offset:%lu sz:%lu vma:%lx\n\n", mintext, maxtext - mintext, textvma);
	}
	return 0;
}

static int rd_extended_data(ctx_t *ctx)
{
	unsigned int i = 0, perms = 0;
	asection *s = 0;

	if (ctx->opt_debug) {
		printf(" -- Finding .data segment boundaries\n\n");

		printf(" index\t\tname\t\t\trange\tsize\tpermissions\toffset\n");
		printf("--------------------------------------------------------------------\n");
	}
	s = ctx->abfd->sections;
	for (i = 0; i < ctx->shnum; i++) {
		perms = parse_bfd_perm(s->flags);
		if (perms == 4) {
			if (ctx->opt_debug) {
				printf(" [%2u] \t%-20s\t%012lx-%012lx %lu\t%s\t%lu\n", i + 1, s->name, s->vma, s->vma + s->size, s->size, "RW", s->filepos);
			}

			if ((unsigned long) s->filepos < mindata) {
				mindata = s->filepos;
				datavma = s->vma;
			}
			if (s->filepos + s->size > maxdata) {
				maxdata = s->filepos + s->size;
			}
		}
		s = s->next;
	}

	if (ctx->opt_debug) {
		printf(" --> .data future boundaries: offset:%lu sz:%lu vma:%lx\n\n", mindata, maxdata - mindata, datavma);
	}
	return 0;
}


static int extend_text(ctx_t *ctx)
{
	unsigned int i = 0;
	asection *s = 0;

	if (ctx->opt_debug) {
		printf(" -- Extending .text\n\n");
	}

	s = ctx->abfd->sections;
	for (i = 0; i < ctx->shnum; i++) {
		if (str_eq(s->name, ".text")) {
			orig_text = s->vma;
			s->vma = textvma;
			s->filepos = mintext;
			s->flags = -1;
			s->size = maxtext - mintext;
			if (ctx->opt_debug) {
				printf(" * extending section %s\n", s->name);
				printf(" * new .text boundaries: offset:%lu sz:%lu vma:%lx\n\n", mintext, s->size, textvma);
			}
		}
		s = s->next;
	}
	return 0;
}

/**
* Display BFD memory sections
*/
int print_bfd_sections(ctx_t *ctx)
{
	unsigned int i = 0;
	asection *s = 0;
	unsigned perms = 0;
	char *hperms = 0;

	if (ctx->opt_verbose) {
		printf("\n -- Input binary sections\n\n");
		printf("             name                      address range     size   perms    offset\n");
		printf(" --------------------------------------------------------------------------------\n");
	}

	s = ctx->abfd->sections;
	for (i = 0; i < ctx->shnum; i++) {
		perms = parse_bfd_perm(s->flags);
		switch (perms) {
		case 7:
			hperms = "rwx";
			break;
		case 6:
			hperms = "r--";
			break;
		case 5:
			hperms = "r-x";
			break;
		case 4:
			hperms = "rw-";
			break;
		default:
			hperms = "---";
			break;
		}

		if (ctx->opt_verbose) {
			printf(" [%2u] %-20s\t%012lx-%012lx %lu\t%s\t%lu\n", i + 1, s->name, s->vma, s->vma + s->size, s->size, hperms, s->filepos);
		}
		s = s->next;
	}

	if (ctx->opt_verbose) {
		printf("\n");
	}
	return 0;
}

/**
 * Simple hexdump routine
 */
void hexdump(unsigned char *data, size_t size)
{
	size_t i = 0, j = 0;

	for (j = 0; j < size; j += 16) {
		for (i = j; i < j + 16; i++) {
			if (i < size) {
				printf("%02x ", data[i] & 255);
			} else {
				printf("   ");
			}
		}
		printf("   ");
		for (i = j; i < j + 16; i++) {
			if (i < size)
				putchar(32 <= (data[i] & 127)
					&& (data[i] & 127) < 127 ? data[i] & 127 : '.');
			else
				putchar(' ');
		}
		putchar('\n');
	}
}

int list_matching_formats(char **q)
{
        char **p = q;

        if (!p || !*p) {
                return -1;
        }

        printf(" ** Matching formats:\n    ");
        while (*p) {
                printf(" %s", *p++);
        }

        printf("\n");

        return 0;
}


/**
* Open a binary the best way we can
*/
unsigned int open_best(ctx_t *ctx)
{
	int formatok = 0;
	int ok = 0;
	char *target = NULL;

        char **matching = 0;
        unsigned int retries = 0;

      retry:
	// Open as object
	formatok = bfd_check_format(ctx->abfd, bfd_object);
	if(!formatok) { 
                fprintf (stderr, " !! WARNING: bfd_check_format() failed : %s\n", bfd_errmsg (bfd_get_error ()));
//        }else{
//        	printf("Fileformat ok: %x\n", formatok);
        }
        ok = bfd_check_format_matches(ctx->abfd, bfd_object, &matching);
	
        if (!ok) {
                fprintf (stderr, " !! WARNING: bfd_check_format_matches() failed : %s\n", bfd_errmsg (bfd_get_error ()));

                if (bfd_get_error() != bfd_error_file_ambiguously_recognized || (matching == NULL && target == NULL)) {
                        fprintf (stderr, " !! WARNING: ambiguous BFD executable file format : %s\n", bfd_errmsg (bfd_get_error ()));
                        return -1;
                } else if (retries < 3) {
                        retries++;
                        printf(" ** Multiple matching BFD file formats\n");
                        list_matching_formats(matching);

                        bfd_close(ctx->abfd);

                        bfd_init();
                        target = strdup(matching[retries]);
                        printf(" ** Using BFD target: %s\n", target);

                        goto retry;
                }

//        }else{
//        	printf(" -- matching formats:\n");
//                list_matching_formats(matching);
//                printf(" -- BFD parsing: ok\n");        
//                printf(" -- arch size: %u\n", bfd_get_arch_size(ctx->abfd));
        }
	
	ctx->shnum = bfd_count_sections(ctx->abfd);
	
	printf("ctx->shnum = %x\n", ctx->shnum);
	
	ctx->corefile = 0;

	// Open as core file
	if ((!formatok) || (!ctx->shnum)) {
		formatok = bfd_check_format(ctx->abfd, bfd_core);
		ctx->shnum = bfd_count_sections(ctx->abfd);
		ctx->corefile = 1;
	}
	// Open as archive
	if ((!formatok) || (!ctx->shnum)) {
		formatok = bfd_check_format(ctx->abfd, bfd_archive);
		ctx->shnum = bfd_count_sections(ctx->abfd);
		ctx->corefile = 0;
	}

	if ((!formatok) || (!ctx->shnum)) {
		printf(" -- couldn't find a format for %s\n", ctx->binname);
		return 0;
	}
	return ctx->shnum;
}

/**
* Open destination binary
*/
int open_target(ctx_t *ctx)
{
	int fd = 0;
	struct stat sb;
	char *newname = 0;
	char *p = 0;

	if (stat(ctx->binname, &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	if ((ctx->opt_binname) && (strlen(ctx->opt_binname))) {
		newname = ctx->opt_binname;
	} else {
		newname = calloc(1, strlen(ctx->binname) + 20);
		sprintf(newname, "a.out");
	}

	if (ctx->opt_debug) {
		printf(" -- Creating output file: %s\n\n", newname);
	}

	fd = open(newname, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd <= 0) {
		printf(" ERROR: open(%s) %s\n", newname, strerror(errno));
		exit(EXIT_FAILURE);
	}
	// set end of file
	ftruncate(fd, sb.st_size);

	// Copy default content : poison bytes or original data
	p = calloc(1, sb.st_size);
	if (ctx->opt_poison) {
		// map entire binary with poison byte
		memset(p, ctx->opt_poison, sb.st_size);
	} else {
		// Default : copy original binary
		int fdin = open(ctx->binname, O_RDONLY);
		read(fdin, p, sb.st_size);
		close(fdin);
	}
	lseek(fd, 0x00, SEEK_SET);
	write(fd, p, sb.st_size);
	free(p);
	lseek(fd, 0x00, SEEK_SET);

	ctx->fdout = fd;
	return fd;
}

/**
* Write sections to disk
*/
int copy_body(ctx_t *ctx)
{
	msec_t *s = 0;

	DL_FOREACH(ctx->mshdrs, s) {
		write_section(ctx, s);
	}
	return 0;
}

/**
* Load a binary using bfd
*/
int load_binary(ctx_t *ctx)
{
	ctx->abfd = bfd_openr(ctx->binname, NULL);
	ctx->shnum = open_best(ctx);
	
	if (!ctx->shnum) {
		printf("error: ctx->shnum is null\n");
		exit(EXIT_FAILURE);
	}
	
	ctx->archsz = bfd_get_arch_size(ctx->abfd);
	if (ctx->opt_verbose) {
		printf(" -- Architecture size: %u\n", ctx->archsz);
	}
	return 0;
}

/**
* Return section flags from its name
*/
int flags_from_name(const char *name)
{
	if (str_eq(name, ".bss")) {
		return FLAG_BSS | FLAG_NOBIT | FLAG_NOWRITE;
	} else if (str_eq(name, ".text")) {
		return FLAG_TEXT;
	}
	return 0;
}

/**
* Craft Section header
*/
int craft_section(ctx_t *ctx, msec_t *m)
{
	asection *s = m->s_bfd;
	Elf_Shdr *shdr = m->s_elf;
	unsigned int dalign = 0;
	unsigned int dperms = 0;

	unsigned perms = parse_bfd_perm(s->flags);
	dperms = 0;
	switch (perms & 0xf) {
	case 7:
		dperms = SHF_ALLOC | SHF_WRITE | SHF_EXECINSTR;	// "rwx";
		break;
	case 6:
		dperms = SHF_ALLOC;	//"r--";
		break;
	case 5:
		dperms = SHF_ALLOC | SHF_EXECINSTR;	// "r-x";
		break;
	case 4:
		dperms = SHF_ALLOC | SHF_WRITE;	// "rw-"
		break;
	default:
		dalign = 1;
		dperms = 0;	// "---"
		break;
	}

	// append name to strndx
	memcpy(ctx->strndx + ctx->strndx_len, s->name, strlen(s->name));

	shdr->sh_name = ctx->strndx_len;	// Section name (string tbl index)
	shdr->sh_type = typefromname(s->name);	// Section type
	shdr->sh_flags = dperms;	// Section flags
	shdr->sh_addr = s->vma;	// Section virtual addr at execution

	if (ctx->opt_reloc) {
		shdr->sh_addr = 0;	// vma is null in relocatable files
	}

	shdr->sh_offset = s->filepos;	// Section file offset
	shdr->sh_size = s->size;	// Section size in bytes
	shdr->sh_addralign = dalign ? dalign : alignfromname(s->name);	// Section alignment
	shdr->sh_entsize = entszfromname(s->name);	// Entry size if section holds table

	ctx->strndx_len += strlen(s->name) + 1;
	return 0;
}

/**
* Read a section from disk
*/
static int read_section(ctx_t *ctx, asection *s)
{
	int fd = 0;
	unsigned int n = 0, nread = 0, nwrite = 0;
	asection *buf = 0;
	unsigned int wantedsz = 0;

	// Open input binary
	fd = open(ctx->binname, O_RDONLY);
	if (fd <= 0) {
		printf("error: open(%s) : %s\n", ctx->binname, strerror(errno));
		exit(0);
	}
	// Go to correct offset
	lseek(fd, s->filepos, SEEK_SET);

	// allocate tmp memory
	wantedsz = s->size;
	if (str_eq(s->name, ".text")) {
		wantedsz = s->size;
	}
	buf = calloc(1, wantedsz);

	// Create Meta section
	msec_t *ms = calloc(1, sizeof(msec_t));
	if (!ms) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	ms->s_elf = calloc(1, sizeof(Elf_Shdr));
	if (!ms->s_elf) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	// read data from disk
	if (str_eq(s->name, ".bss")) {
		// SHT_NOBITS Section contains no data (Global Uninitialized Data)
		n = 0;
		buf = realloc(buf, 0);
	} else {
		// read from disk
		n = 0;
		nread = read(fd, buf, s->size);
		while ((nread != 0) && (n <= s->size)) {
			n += nread;
			nread = read(fd, buf + n, s->size - n);
		}

		if ((n != s->size) && (str_eq(s->name, ".text"))) {
			n = s->size;
			// initialized at 0x00 by calloc
		} else if (n != s->size) {
			printf("read failed: %u != %u\n", n, (unsigned int) s->size);
		}
	}

	// fill Meta section
	ms->s_bfd = s;
	ms->len = n;
	ms->name = strdup(s->name);
	ms->data = (unsigned char *) buf;
	ms->outoffset = s->filepos;

	// fill ELF section
	craft_section(ctx, ms);

	ms->flags = flags_from_name(s->name);

	// Add to double linked list of msec_t Meta sections
	DL_APPEND(ctx->mshdrs, ms);
	ctx->mshnum++;

	// Close file descriptor
	close(fd);
	return nwrite;
}

/**
* Display sections
*/
int print_msec(ctx_t *ctx)
{
	msec_t *ms = 0;
	unsigned int count = 0;

	DL_COUNT(ctx->mshdrs, ms, count);
	printf(" -- %u elements\n", count);

	DL_FOREACH(ctx->mshdrs, ms) {
		printf("%s  %lu\n", ms->name, ms->len);
	}
	return 0;
}

/**
* Read sections from input binary
*/
int rd_sections(ctx_t *ctx)
{
	unsigned int i = 0;

	asection *s = ctx->abfd->sections;
	for (i = 0; i < ctx->shnum; i++) {
		read_section(ctx, s);
		s = s->next;
	}
	return 0;
}

int save_dynstr(ctx_t *ctx, GElf_Shdr shdr, char *binary)
{
	if (globalstrtab == 0) {
		globalstrtab = calloc(1, shdr.sh_size + 3);
		globalstrtablen++;	// Start with a null byte
	} else {
		globalstrtab = realloc(globalstrtab, globalstrtablen + shdr.sh_size + 2);
	}
	memcpy(globalstrtab + globalstrtablen, binary + shdr.sh_offset, shdr.sh_size + 1);
	globalstrtablen += shdr.sh_size + 1;

	return 0;
}

int save_dynsym(ctx_t *ctx, GElf_Shdr shdr, char *binary)
{
	if (globalsymtab == 0) {
		globalsymtab = calloc(1, 2*sizeof(Elf_Sym) + shdr.sh_size);
		memset(globalsymtab, 0x00, sizeof(Elf_Sym));
		globalsymtablen += sizeof(Elf_Sym);	// Skip 1 NULL entry
	} else {
		globalsymtab = realloc(globalsymtab, shdr.sh_size + globalsymtablen);
	}

	memcpy(globalsymtab + globalsymtablen, binary + shdr.sh_offset + sizeof(Elf_Sym), shdr.sh_size - sizeof(Elf_Sym));

	globalsymtablen += shdr.sh_size - sizeof(Elf_Sym);

	return 0;
}

int patch_symbol_index(ctx_t *ctx, Elf_Sym *s)
{
	msec_t *sec = 0;

	sec = section_from_index(ctx, s->st_shndx);	// section related to this object
	if (sec) {
		// patch section index in symbol table
		s->st_shndx = secindex_from_name_after_strip(ctx, sec->name);
	} else {
//              printf(" no section info for symbol: %s\n", sname);
	}

	return 0;
}


int fixup_symtab_section_index(ctx_t *ctx)
{
	Elf_Sym *s = 0;
	unsigned int sindex = 0;

	for (sindex = maxnewsec + 1 + extrasectionnum; sindex < globalsymtablen / sizeof(Elf_Sym); sindex++) {

		// get symbol name from index
		s = globalsymtab + sindex * sizeof(Elf_Sym);

		if (s->st_shndx) {
			patch_symbol_index(ctx, s);
		}

	}
	return 0;
}

int append_reloc(Elf_Rela *r)
{
	unsigned int i = 0;
	Elf_Rela *s = 0;

	// search this address in reloc table
	for (i = 0; i < globalreloclen / sizeof(Elf_Rela); i++) {
		s = globalreloc + i * sizeof(Elf_Rela);
		//
		if (s->r_offset == r->r_offset) {
			printf(" !! WARNING: already have a .text relocation at %lu\n", r->r_offset);
			return -1;	// already in relocation section
		}
	}

	// save relocation
	if (!globalreloc) {
		globalreloc = calloc(1, sizeof(Elf_Rela));
	} else {
		globalreloc = realloc(globalreloc, sizeof(Elf_Rela) + globalreloclen);
	}

	memcpy(globalreloc + globalreloclen, r, sizeof(Elf_Rela));
	globalreloclen += sizeof(Elf_Rela);

	return 0;

}

int append_data_reloc(Elf_Rela *r)
{
	unsigned int i = 0;
	Elf_Rela *s = 0;

	// search this address in reloc table
	for (i = 0; i < globaldatareloclen / sizeof(Elf_Rela); i++) {
		s = globaldatareloc + i * sizeof(Elf_Rela);
		//
		if (s->r_offset == r->r_offset) {
			printf(" !! WARNING: already have a .data relocation at %lu\n", r->r_offset);
			return -1;	// already in relocation section
		}
	}

	// save relocation
	if (!globaldatareloc) {
		globaldatareloc = calloc(1, sizeof(Elf_Rela));
	} else {
		globaldatareloc = realloc(globaldatareloc, sizeof(Elf_Rela) + globaldatareloclen);
	}

	memcpy(globaldatareloc + globaldatareloclen, r, sizeof(Elf_Rela));
	globaldatareloclen += sizeof(Elf_Rela);

	return 0;

}

int save_global_import(ctx_t *ctx, char *sname, msec_t *sec, Elf_Rela *r, unsigned int sindex)
{
	int rtype = 0;
	gimport_t *g = 0;
	Elf_Rela *rnew = 0;

	rtype = ELF_R_TYPE(r->r_info);

	if (ctx->opt_verbose) {
		printf("recording blobal import variable %s in section: %s\t%s\tat:%lu\toff:%lu\n", sname, sec->name, reloc_htype(rtype), r->r_offset, r->r_offset - textvma);
	}

	g = calloc(1, sizeof(gimport_t));
	g->sname = strdup(sname);
	g->sec = sec;
	g->sindex = sindex;	// index of symbol in symbol table

	rnew = calloc(1, sizeof(Elf_Rela));
	memcpy(rnew, r, sizeof(Elf_Rela));
	g->r = rnew;
	g->rtype = rtype;

	if (!gimports) {
		gimports = calloc(1, sizeof(gimport_t *));
	} else {
		gimports = realloc(gimports, sizeof(gimport_t *) * (gimportslen + 1));
	}
	gimports[gimportslen++] = g;
	return 0;
}

/**
* Return index in global import matching this address
*/
int check_global_import(unsigned long int addr)
{
	unsigned int i = 0;

	if (addr < 4096) {
		return -1;
	}

	for (i = 0; i < gimportslen; i++) {
		if ((gimports[i]) && (gimports[i]->r) && (gimports[i]->r->r_offset == addr)) {
			return i;
		}
	}

	return -1;
}

int save_reloc(ctx_t *ctx, Elf_Rela *r, unsigned int sindex, int has_addend)
{
	Elf_Sym *s = 0;
	char *sname = 0;
	unsigned int i = 0;

	int rtype = 0, outtype = 0;
	char *htype = 0;
	msec_t *sec = 0;

	Elf_Rela *rout = 0;

	rout = calloc(1, sizeof(Elf_Rela));	// Work on a copy of the relocation instead of the original one
	memcpy(rout, r, sizeof(Elf_Rela));

	if (!has_addend) {
		rout->r_addend = 0;
	};

	// search symbol corresponding to this index
	if (!globalsymtab) {
		printf(" !! WARNING: no symbol table for relocation index %u\n", sindex);
		return 0;
	}

	if (sindex > globalsymtablen / sizeof(Elf_Sym)) {	// verify index is within bound of symtab...
		printf(" !! WARNING: symbol index %u is out of bounds of symbol table\n", sindex);
		return 0;
	}

	if (!globalstrtab) {
		printf(" !! WARNING: no string table for relocation index %u\n", sindex);
		return 0;
	}
	// get symbol name from index
	s = globalsymtab + sindex * sizeof(Elf_Sym);
	sname = globalstrtab + s->st_name;

	// check if name is "old_" : if so, skip
	if (str_eq(sname, "old_")) {
		return -1;
	}
	// check if name is in blacklist
	for (i = 0; i < sizeof(blnames) / sizeof(char *); i++) {
		if (str_eq(sname, blnames[i])) {
			if (ctx->opt_verbose) {
				printf(" * name blacklisted: %s\n", sname);
			}
			return -1;	// Name in blacklist
		}
	}

	if (ctx->opt_debug) {
		printf(" * adding relocation for: %s\n", sname);
	}

	/**
        * Convert relocation depending on type and source section
        */

	outtype = 0;
	rtype = ELF_R_TYPE(rout->r_info);
	sec = section_from_index(ctx, s->st_shndx);	// section related to this object
	if (sec) {

		if (str_eq(sec->name, ".bss")) {
			// Save global import
			save_global_import(ctx, sname, sec, r, sindex);
			return 0;
		} else {
			// Skip
			if (ctx->opt_debug) {
				printf
				    (" !! WARNING: skipping unknown relocation for symbol: %s in section: %s\t%s\tat:%lu\toff:%lu\n",
				     sname, sec->name, reloc_htype(rtype), rout->r_offset, rout->r_offset - textvma);
			}
			s->st_shndx = 0;
			s->st_value = 0;	// Actual value is null in this case
			return 0;
		}

	} else if (rtype == R_X86_64_RELATIVE) {
		if (ctx->opt_debug) {
			printf(" * Not saving Relative relocation %lu %lu\n", rout->r_offset, rout->r_addend);
		}

		return 0;
	} else {		// Jump slots
		if (ctx->opt_debug) {
			printf(" no section info for symbol: %s\n", sname);
		}

#ifdef __x86_64__
		outtype = R_X86_64_64;
#else
		outtype = R_386_32;
#endif
		rout->r_offset -= textvma;
		rout->r_info = ELF_R_INFO(sindex, outtype);
	}

	if (ctx->opt_debug) {
		htype = reloc_htype(outtype);
		printf("-->>     %016lx\t%lu\t%s\t%u\taddend:%lu\t\n\n", rout->r_offset, rout->r_info, htype, sindex, rout->r_addend);
	}

	append_reloc(rout);
	return 0;
}

int save_data_reloc(ctx_t *ctx, Elf_Rela *r, unsigned int sindex, int has_addend)
{
	Elf_Sym *s = 0;
	char *sname = 0;
	unsigned int i = 0;

	int rtype = 0, outtype = 0;
	char *htype = 0;
	msec_t *sec = 0;

	Elf_Rela *rout = 0;

	rout = calloc(1, sizeof(Elf_Rela));	// Work on a copy of the relocation instead of the original one
	memcpy(rout, r, sizeof(Elf_Rela));

	if (!has_addend) {
		rout->r_addend = 0;
	};

	// search symbol corresponding to this index
	if (!globalsymtab) {
		printf(" !! WARNING: no symbol table for relocation index %u\n", sindex);
		return 0;
	}

	if (sindex > globalsymtablen / sizeof(Elf_Sym)) {	// verify index is within bound of symtab...
		printf(" !! WARNING: symbol index %u is out of bounds of symbol table\n", sindex);
		return 0;
	}

	if (!globalstrtab) {
		printf(" !! WARNING: no string table for relocation index %u\n", sindex);
		return 0;
	}
	// get symbol name from index
	s = globalsymtab + sindex * sizeof(Elf_Sym);
	sname = globalstrtab + s->st_name;

	// check if name is "old_" : if so, skip
	if (str_eq(sname, "old_")) {
		return -1;
	}

	if (ctx->opt_debug) {
		printf("\n * adding .data relocation for: %s\n", sname);
	}

	/**
        * Convert relocation depending on type and source section
        */

	outtype = 0;
//	rtype = ELF_R_TYPE(rout->r_info);
//	sec = section_from_index(ctx, s->st_shndx);	// section related to this object

#ifdef __x86_64__
		outtype = R_X86_64_64;
#else
		outtype = R_386_32;
#endif
		rout->r_offset = r->r_offset;
		rout->r_info = ELF_R_INFO(sindex, outtype);
		rout->r_addend = r->r_addend;

	if (ctx->opt_debug) {
		htype = reloc_htype(outtype);
		printf("-->>     %016lx\t%lu\t%s\t%u\taddend:%lu\t\n\n", rout->r_offset, rout->r_info, htype, sindex, rout->r_addend);
	}

	append_data_reloc(rout);
	return 0;
}

static void print_string_hex(char *comment, unsigned char *str, size_t len)
{
	unsigned char *c = 0;

	printf("%s", comment);
	for (c = str; c < str + len; c++) {
		printf("0x%02x ", *c & 0xff);
	}

	printf("\n");
}



static void print_insn_detail(ctx_t *ctx, csh handle, cs_mode mode, cs_insn *ins)
{
	int count = 0, i = 0;
	cs_x86 *x86 = 0;

	// detail can be NULL on "data" instruction if SKIPDATA option is turned ON
	if (ins->detail == NULL)
		return;

	x86 = &(ins->detail->x86);
	printf("\tAddress: %lu\n", ins->address);
	printf("\tInstruction Length: %u\n", ins->size);

	print_string_hex("\tPrefix:", x86->prefix, 4);

	print_string_hex("\tOpcode:", x86->opcode, 4);

	printf("\trex: 0x%x\n", x86->rex);

	printf("\taddr_size: %ju\n", (uintmax_t) x86->addr_size);
	printf("\tmodrm: 0x%jx\n", (uintmax_t) x86->modrm);
	printf("\tdisp: 0x%jx\n", (uintmax_t) x86->disp);

	// SIB is not available in 16-bit mode
	if ((mode & CS_MODE_16) == 0) {
		printf("\tsib: 0x%x\n", x86->sib);
		if (x86->sib_base != X86_REG_INVALID)
			printf("\t\tsib_base: %s\n", cs_reg_name(handle, x86->sib_base));
		if (x86->sib_index != X86_REG_INVALID)
			printf("\t\tsib_index: %s\n", cs_reg_name(handle, x86->sib_index));
		if (x86->sib_scale != 0)
			printf("\t\tsib_scale: %d\n", x86->sib_scale);
	}
	// SSE code condition
	if (x86->sse_cc != X86_SSE_CC_INVALID) {
		printf("\tsse_cc: %u\n", x86->sse_cc);
	}
	// AVX code condition
	if (x86->avx_cc != X86_AVX_CC_INVALID) {
		printf("\tavx_cc: %u\n", x86->avx_cc);
	}
	// AVX Suppress All Exception
	if (x86->avx_sae) {
		printf("\tavx_sae: %u\n", x86->avx_sae);
	}
	// AVX Rounding Mode
	if (x86->avx_rm != X86_AVX_RM_INVALID) {
		printf("\tavx_rm: %u\n", x86->avx_rm);
	}

	count = cs_op_count(handle, ins, X86_OP_IMM);
	if (count) {
		printf("\timm_count: %u\n", count);
		for (i = 1; i < count + 1; i++) {
			int index = cs_op_index(handle, ins, X86_OP_IMM, i);
			printf("\t\timms[%u]: 0x%" PRIx64 "\n", i, x86->operands[index].imm);
		}
	}

	if (x86->op_count)
		printf("\top_count: %u\n", x86->op_count);
	for (i = 0; i < x86->op_count; i++) {
		cs_x86_op *op = &(x86->operands[i]);

		switch ((int) op->type) {
		case X86_OP_REG:
			printf("\t\toperands[%u].type: REG = %s\n", i, cs_reg_name(handle, op->reg));
			break;
		case X86_OP_IMM:
			printf("\t\toperands[%u].type: IMM = 0x%" PRIx64 "\n", i, op->imm);
			break;
		case X86_OP_MEM:
			printf("\t\toperands[%u].type: MEM\n", i);
			if (op->mem.segment != X86_REG_INVALID)
				printf("\t\t\toperands[%u].mem.segment: REG = %s\n", i, cs_reg_name(handle, op->mem.segment));
			if (op->mem.base != X86_REG_INVALID)
				printf("\t\t\toperands[%u].mem.base: REG = %s\n", i, cs_reg_name(handle, op->mem.base));
			if (op->mem.index != X86_REG_INVALID)
				printf("\t\t\toperands[%u].mem.index: REG = %s\n", i, cs_reg_name(handle, op->mem.index));
			if (op->mem.scale != 1)
				printf("\t\t\toperands[%u].mem.scale: %u\n", i, op->mem.scale);
			if (op->mem.disp != 0)
				printf("\t\t\toperands[%u].mem.disp: 0x%" PRIx64 "\n", i, op->mem.disp);
			break;
		default:
			break;
		}

		// AVX broadcast type
		if (op->avx_bcast != X86_AVX_BCAST_INVALID)
			printf("\t\toperands[%u].avx_bcast: %u\n", i, op->avx_bcast);

		// AVX zero opmask {z}
		if (op->avx_zero_opmask != false)
			printf("\t\toperands[%u].avx_zero_opmask: TRUE\n", i);

		printf("\t\toperands[%u].size: %u\n", i, op->size);
	}

	printf("\n");
}

static int create_text_data_reloc(ctx_t *ctx, cs_insn *ins, msec_t *m, unsigned int soff, int rip_relative, unsigned int argnum)
{
	unsigned int sindex = 0;
	unsigned int wheretowrite = 0;
	unsigned int n = 0;
	int gimport = -1;
	cs_x86 *x86 = 0;

	x86 = &(ins->detail->x86);
	n = x86->op_count;
	wheretowrite = ins->size - 4;

	if (ctx->opt_debug) {
		printf(" --> transforming relocation from section: %s at ", m->name);
		printf("0x%" PRIx64 ":\t%s\t%s\tinslen:%u  argoffset:%u\n", ins->address, ins->mnemonic, ins->op_str, ins->size, wheretowrite);
	}

	sindex = secindex_from_name_after_strip(ctx, m->name);

	if ((str_eq(m->name, ".bss")) && (n == 2) && (!rip_relative)) {
		gimport = check_global_import(x86->operands[1].imm);
	}

	if (rip_relative) {
//              printf(" ** destination: %lx\n", x86->operands[1]->mem.disp + ins->address + 7);
		gimport = check_global_import(x86->operands[1].mem.disp + ins->address + 7);
		if (ctx->opt_debug) {
			printf("** global imports match : %d\n", gimport);
		}
	}

	if ((gimport != -1) && (!rip_relative)) {	// Relocation to .bss with a known global import

		if (ctx->opt_debug) {
			printf(" * known imported global : %s\n", gimports[gimport]->sname);
		}

		Elf_Rela *r = calloc(1, sizeof(Elf_Rela));
		if (!r) {
			perror("calloc");
			exit(-1);
		}

		sindex = gimports[gimport]->sindex;

		// reset string index in symbol table
		Elf_Sym *st = 0;
		st = globalsymtab + gimports[gimport]->sindex * sizeof(Elf_Sym);
		st->st_shndx = 0;
		st->st_value = 0;

		r->r_info = ELF_R_INFO(gimports[gimport]->sindex, R_X86_64_PC32);
		r->r_addend = 0;	//-4;
		r->r_offset = ins->address - textvma + wheretowrite;

		// patch back binary
		msec_t *t = section_from_name(ctx, ".text");
		memset(t->data + r->r_offset, 0x00, 4);

		if (ctx->opt_debug) {
			printf("%" PRIx64 "\t%s+%u\t\t\t(%s %s)\n", ins->address, m->name, soff, ins->mnemonic, ins->op_str);
			printf("%012lx\t%012lx\t%s\t%012x\t%s+%u\n", r->r_offset, r->r_info, "R_X86_64_32", 0, m->name, soff);
		}
		// append relocation
		append_reloc(r);

		free(r);

	} else if ((gimport != -1) && (rip_relative)) {	// Relocation to .bss with a known global import via rip relative

		if (ctx->opt_debug) {
			printf(" * known imported global (rip relative) : %s\n", gimports[gimport]->sname);
		}

		Elf_Rela *r = calloc(1, sizeof(Elf_Rela));
		if (!r) {
			perror("calloc");
			exit(-1);
		}

		sindex = gimports[gimport]->sindex;

		// reset string index in symbol table
		Elf_Sym *st = 0;
		st = globalsymtab + gimports[gimport]->sindex * sizeof(Elf_Sym);
		st->st_shndx = 0;
		st->st_value = 0;

		r->r_info = ELF_R_INFO(gimports[gimport]->sindex, R_X86_64_PC32);
		r->r_addend = -4;
		r->r_offset = ins->address - textvma + wheretowrite;

		// patch back binary
		msec_t *t = section_from_name(ctx, ".text");
		memset(t->data + r->r_offset, 0x00, 4);

		if (ctx->opt_debug) {
			printf("%012lx\t%012lx\t%s\t%012x\t%s+%ld\n", r->r_offset, r->r_info, "R_X86_64_32", 0, gimports[gimport]->sname, r->r_addend);
		}
		// append relocation
		append_reloc(r);

		free(r);

	} else if (sindex) {	// Any other local section relocation (.rodata, .data ...)

		Elf_Rela *r = calloc(1, sizeof(Elf_Rela));
		if (!r) {
			perror("calloc");
			exit(-1);
		}

		if (ctx->opt_debug) {
			printf("new %s sindex:%u addent:%u, off:0x%lx\thasrel:%u\n",
			       rip_relative ? "rip relative local" : "local", sindex, soff, ins->address - textvma + wheretowrite, ctx->has_relativerelocations);
		}

		int newtype;
#ifdef __x86_64__
		newtype = ctx->has_relativerelocations ? R_X86_64_PC32 : R_X86_64_32;
#else
#ifdef __i386__
		newtype = R_386_32;	//R_386_PLT32;//R_386_GOTOFF;//R_386_GOTPC; //R_386_GOT32; //R_386_COPY; //R_386_RELATIVE; //R_386_PC32; //R_386_GLOB_DAT; //R_386_32;
#endif
#endif

		// patch back .text
		msec_t *t = section_from_name(ctx, ".text");

		if (argnum == 0) {	// typically: cmp [rip+0xdeadbeef], 0  or something like mov qword ptr [rip+0xdeadbeef], rax
			/*
			 * find at witch position we shall patch by scanning memory for the memory displacement
			 */
			for (wheretowrite = 0; wheretowrite <= ins->size; wheretowrite++) {
				unsigned int searchval = x86->operands[0].mem.disp;
				if (!memcmp(t->data + (ins->address - textvma + wheretowrite), &searchval, 4)) {
//                                      printf(" * patching instruction at offset: %u addend: %d relative:%d\n", wheretowrite, soff, rip_relative ? rip_relative : 0);
					break;
				}
			}

			// not found ? Fatal error
			if (wheretowrite == ins->size) {
				printf("error: can't find patch location\n");
				exit(-1);
			}

		}

		if (rip_relative) {	// compute new addend and set reloc type
			cs_x86_op *op = &(x86->operands[argnum]);

			r->r_addend = ((op->mem.disp) + ins->address) - m->s_bfd->vma + ins->size -4; // TODO	// (rip + (immediate|displacement)) - m->s_bfd->vma;    // dst - (section vma)
//                      printf("* new rip based addend : %llx\n", r->r_addend);

			newtype = R_X86_64_PC32;

		} else {
			r->r_addend = soff;	// default
		}

		r->r_info = ELF_R_INFO(sindex, newtype);
		r->r_offset = ins->address - textvma + wheretowrite;

		if ((argnum == 0) && (rip_relative)) {	// write at custom location
			memset(t->data + r->r_offset, 0x00, 4);	// write back where computed previously
//                      r->r_addend = -4;
		} else if (rip_relative) {	// patch back register index (from rip to 0x00)
			memset(t->data + r->r_offset, 0x00, 4);	// write back 4 bytes (one 32b address) + 1 if relative (overwrite rip)
//                      r->r_addend = 0;//0x29a;
		} else if (ELFMACHINE == EM_386) {	// R_386_32 doesn't account for the addend : write it back to .text
			memcpy(t->data + r->r_offset, &soff, 4);
		} else {
			memset(t->data + r->r_offset, 0x00, 4);	// write back 4 bytes (one 32b address) + 1 if relative (overwrite rip)
		}
/**
* FIXED : Problem : http://fossies.org/linux/glibc/sysdeps/x86_64/dl-machine.h line 452 : possible overflow in relocation amd64 when using type R_X86_64_32 : should be fixed
*
*/

//              if (ctx->opt_verbose) {
//                      printf("%"PRIx64"\t%s+0x%lx\t\t\t(%s %s)\n",ins->address, m->name, soff, ins->mnemonic, ins->op_str);
//                      printf("%012lx\t%012lx\t%s\t%s+%u\n", r->r_offset, r->r_info, reloc_htype(newtype), m->name, soff);
//              }

		// append relocation
		append_reloc(r);

		free(r);
	} else {		// Unhandled relocation
		if (ctx->opt_debug) {
			printf(" !! WARNING: unknown relocation section: %s at ", m->name);
			printf("0x%" PRIx64 ":\t%s\t%s\n", ins->address, ins->mnemonic, ins->op_str);
		}
	}

	return 0;
}

int internal_function_store(ctx_t *ctx, unsigned long long int addr)
{
	unsigned int i = 0;
	char buff[200];
	Elf_Sym *s = 0;

	memset(buff, 0x00, 200);
	snprintf(buff, 200, "internal_%08llx", addr);

	// search this symbol in string table
	for (i = 0; i < globalstrtablen; i += strlen(globalstrtab + i) + 1) {
		if (str_eq(globalstrtab + i, buff)) {
			return -1;	// already in strtab, hence in symtab
		}
	}

	// search this address in symbol table
	for (i = 0; i < globalsymtablen / sizeof(Elf_Sym); i++) {
		s = globalsymtab + i * sizeof(Elf_Sym);
		//
		if (s->st_value == addr) {
			return -1;	// already in symtab
		}
	}

	add_symaddr(ctx, buff, addr, 0x54);
	return 0;
}

static void parse_text_data_reloc(ctx_t *ctx, csh ud, cs_mode mode, cs_insn *ins)
{
	int i = 0;
	cs_x86 *x86 = 0;
	msec_t *m = 0;
	// detail can be NULL on "data" instruction if SKIPDATA option is turned ON
	if (ins->detail == NULL)
		return;

	x86 = &(ins->detail->x86);

//      if(ctx->opt_debug){
//              printf("\t\tRELOC Address: %lu\n", ins->address);
//              printf("\t\tRELOC operands.type: MEM, opcount:%u\n", x86->op_count);
//      }

	for (i = 0; i < x86->op_count; i++) {
		cs_x86_op *op = &(x86->operands[i]);
		m = 0;
		switch ((int) op->type) {
		case X86_OP_IMM:
			;
			m = section_from_addr(ctx, op->imm);
			if (m) {
				if (i == 1) {	// only match arg=1 // second argument is of form [0xdeadbeef]
					create_text_data_reloc(ctx, ins, m, op->imm - m->s_bfd->vma, 0, i);
				} else if (i == 0) {	// first argument is of form [0xdeadbeef]

					if ((str_eq(ins->mnemonic, "call"))
					    && (str_eq(m->name, ".text"))) {
//                                                      printf(" --> call to internal function at %lx\n", op->imm);
						internal_function_store(ctx, op->imm);
					} else {
// TODO instrument internal/cross sections jumps and calls
//                                                      printf("We have a problem: (dst section: %s)\n", m->name);
//                                                      printf("0x%llx %s %s\n", ins->address, ins->mnemonic, ins->op_str);
					}

				}
			}
			break;
		case X86_OP_MEM:
			;
			m = section_from_addr(ctx, op->mem.disp + ins->address + 10 * ctx->has_relativerelocations);	// assume rip relative
//printf("sec name: %s\t%lx\n", m ? m->name : "-", op->mem.disp + ins->address + 10*ctx->has_relativerelocations);
			if ((m) && (m->name) && (!str_eq(m->name, ".text") || ctx->has_relativerelocations) && (cs_reg_name(ud, op->mem.base)) && (str_eq(cs_reg_name(ud, op->mem.base), "rip"))) {	// destination section can't be .text, must be rip relative
				if (i == 1) {	// only match arg=1
					create_text_data_reloc(ctx, ins, m, op->mem.disp + ins->address - m->s_bfd->vma, 1, i);	// Addressing is rip relative
					break;
				}
			}

			if (str_eq(ins->mnemonic, "nop")) {	// ignore nops
				break;
			}
			// handle write to 1st argument, which is a rip relative mapped section
			if ((i == 0) && (m) && (m->name)
			    && (!str_eq(m->name, ".text") || ctx->has_relativerelocations)
			    && (cs_reg_name(ud, op->mem.base))
			    && (str_eq(cs_reg_name(ud, op->mem.base), "rip"))) {
//                                      printf(" * handling 1st arg access : %llx %s %s (%s)\n",ins->address, ins->mnemonic, ins->op_str, m ? m->name : "-");
				create_text_data_reloc(ctx, ins, m, op->mem.disp + ins->address - m->s_bfd->vma, 1, i);	// Addressing is rip relative
				break;
			}

			break;

		case X86_OP_REG:
//                      case X86_OP_MEM:
		default:
			break;
		}
	}
}

// Simplified analyze_data function focusing on the crash
int analyze_data(ctx_t *ctx, msec_t *s)
{
    unsigned int i = 0;
    msec_t *sec = 0, *tmp = 0;
    unsigned int secindex = 0;
    int total_relocations = 0;
    
    printf(" -- Analyzing .data section (Simplified)\n\n");
    hexdump(s->data, s->len);
    printf("\n");
    
    if (ctx->archsz == 64) {
        for (i = 0; i < s->len; i += 8) {
            unsigned long int val = *(unsigned long int*)(s->data+i);
            printf(" data[%04u] %016lx", i/8, val);
            
            if (val == 0) {
                printf(" (null)\n");
                continue;
            }
            
            // Check if this value points to any section
            // Be less restrictive - check all values above 0x1000
            if (val < 0x1000) {
                printf(" (small value - not a pointer)\n");
                continue;
            }
            
            int found_match = 0;
            DL_FOREACH_SAFE(ctx->mshdrs, sec, tmp) {
                if (!sec->s_bfd || !sec->s_bfd->vma || !sec->s_elf || !sec->s_elf->sh_size) {
                    continue;
                }
                
                // Check if value is within section bounds
                if (val >= sec->s_bfd->vma && val < (sec->s_bfd->vma + sec->s_elf->sh_size)) {
                    printf("  --> %s (%016lx-%016lx)", sec->name, 
                           sec->s_bfd->vma, sec->s_bfd->vma + sec->s_elf->sh_size);
                    
                    // Skip self-references in .data
                    if (str_eq(sec->name, ".data")) {
                        printf(" (self-reference - skipping)");
                        goto next_entry;
                    }
                    
                    // Create relocation
                    Elf_Rela *r = calloc(1, sizeof(Elf_Rela));
                    r->r_offset = i;
                    r->r_info = 0;
                    r->r_addend = val - sec->s_bfd->vma;

                    // Determine section index
                    if (str_eq(sec->name, ".text")) {
                        secindex = 1;
                    } else if (str_eq(sec->name, ".rodata")) {
                        secindex = 2;
                    } else if (str_eq(sec->name, ".data")) {
                        secindex = 3;
                    } else if (str_eq(sec->name, ".bss")) {
                        secindex = 4;
                    } else {
                        // For other sections, try to get their index
                        secindex = secindex_from_name_after_strip(ctx, sec->name);
                        if (secindex == 0) {
                            printf(" // Unknown section %s", sec->name);
                            free(r);
                            goto next_entry;
                        }
                    }

                    printf(" (secindex=%d)", secindex);
                    save_data_reloc(ctx, r, secindex, 1); 
                    *(unsigned long int*)(s->data+i) = 0;
                    free(r);
                    found_match = 1;
                    total_relocations++;
                    break;
                }
            }
            
            next_entry:
            if (!found_match) {
                printf(" (no section match)");
                
                // Special check for the crash location
                // Entry ~116 (offset 0x3a8) should not be null
                if (i/8 >= 110 && i/8 <= 120) {
                    printf(" *** POTENTIAL CRASH CAUSE: data[%04u] ***", i/8);
                }
            }
            printf("\n");
        }
    }
    
    printf("\n=== Summary ===\n");
    printf("Total relocations created: %d\n", total_relocations);
    
    // Find entries that might still be problematic
    printf("\nChecking for values that might need relocation...\n");
    for (i = 0; i < s->len; i += 8) {
        unsigned long int val = *(unsigned long int*)(s->data+i);
        if (val > 0x1000 && val < 0x100000) {  // Reasonable pointer range
            printf("data[%04u] still contains: 0x%lx - check if this should be relocated\n", 
                   i/8, val);
        }
    }
    
    return 0;
}

// Current analyze_data function (from your code)
int analyze_data_old2(ctx_t *ctx, msec_t *s)
{
    unsigned int i = 0;
    msec_t *sec = 0, *tmp = 0;
    unsigned int secindex = 0;
    
    printf(" -- Analyzing .data section\n\n");
    printf("DEBUG: s->len = %zu\n", s->len);  
    printf("DEBUG: expected entries = %zu\n", s->len / 8);  
    
    hexdump(s->data, s->len);
    printf("\n");
    
    // First, let's list all sections available to understand the issue
    printf("\n=== Available sections for matching ===\n");
    DL_FOREACH(ctx->mshdrs, sec) {
        printf("Section: %s, VMA: 0x%lx, Size: 0x%lx (end: 0x%lx)\n", 
               sec->name, 
               sec->s_bfd ? sec->s_bfd->vma : 0, 
               sec->s_elf ? sec->s_elf->sh_size : 0,
               sec->s_bfd ? (sec->s_bfd->vma + sec->s_elf->sh_size) : 0);
    }
    printf("========================================\n\n");
    
    if (ctx->archsz == 64) {
        for (i = 0; i < s->len; i += 8) {
            unsigned long int val = *(unsigned long int*)(s->data+i);
            printf(" data[%04u] %08lx", i/8, val);  // Fixed: use %08lx instead of %08x
            
            // Enhanced debugging: check why DL_FOREACH doesn't match
            printf(" [CHECKING: ");
            int found_any = 0;
            DL_FOREACH_SAFE(ctx->mshdrs, sec, tmp) {
                printf("%s(vma=0x%lx,sz=0x%lx) ", 
                       sec->name, 
                       sec->s_bfd ? sec->s_bfd->vma : 0,
                       sec->s_elf ? sec->s_elf->sh_size : 0);
                
                // Let's check each condition separately
                if (val) {
                    if (sec->s_bfd && sec->s_bfd->vma) {
                        if (val >= sec->s_bfd->vma) {
                            if (sec->s_elf && sec->s_elf->sh_size) {
                                if (val <= sec->s_bfd->vma + sec->s_elf->sh_size) {
                                    printf("MATCH!");
                                    found_any = 1;
                                    break;
                                } else {
                                    printf("(val too big)");
                                }
                            } else {
                                printf("(no size)");
                            }
                        } else {
                            printf("(val too small)");
                        }
                    } else {
                        printf("(no vma)");
                    }
                } else {
                    printf("(val is 0)");
                }
            }
            printf("] ");
            
            if (!found_any) {
                printf("NO MATCHES FOUND");
            }
            
            // Run the actual matching logic
            DL_FOREACH_SAFE(ctx->mshdrs, sec, tmp) {
                if ((val) && (sec->s_bfd->vma) && 
                    (val >= sec->s_bfd->vma) && (sec->s_elf->sh_size) && 
                    (val <= sec->s_bfd->vma + sec->s_elf->sh_size)) {
                    
                    printf("  --> %s (%08lx-%08lx)", sec->name, 
                           sec->s_bfd->vma, sec->s_bfd->vma + sec->s_elf->sh_size);
                    
                    // Create relocation
                    Elf_Rela *r = calloc(1, sizeof(Elf_Rela));
                    r->r_offset = i;
                    r->r_info = 0;
                    r->r_addend = val - sec->s_bfd->vma;

                    if (!strncmp(sec->name, ".text", 5)) {
                        secindex = 1;
                    } else if (!strncmp(sec->name, ".rodata", 7)) {
                        secindex = 2;
                    } else if (!strncmp(sec->name, ".data", 5)) {
                        secindex = 3;
                    } else if (!strncmp(sec->name, ".bss", 4)) {
                        secindex = 4;
                    } else {
                        secindex = 0;
                        printf(" // Unknown section %s", sec->name);
                    }

                    printf(" (secindex=%d)", secindex);
                    save_data_reloc(ctx, r, secindex, 1); 
                    *(unsigned long int*)(s->data+i) = 0;
                    free(r);
                    break;  // Stop at first match
                }
            }
            
            printf("\n");
        }
    }
    
    // Missing relocations check (moved outside main loop)
    printf("\n=== Addresses not relocated (might be missing) ===\n");
    for (unsigned int j = 0; j < s->len; j += 8) {
        unsigned long int val = *(unsigned long int*)(s->data + j);
        if (val >= 0x4000 && val < 0x25000 && val != 0) {
            printf("data[%04u] %08lx - possible missing relocation\n", j/8, val);
        }
    }
    printf("=== End debug ===\n");
    
    return 0;
}

/*
// Enhanced analyze_data that handles more edge cases
int analyze_data_fixed(ctx_t *ctx, msec_t *s)
{
    printf(" -- Analyzing .data section (FIXED VERSION)\n\n");
    hexdump(s->data, s->len);
    printf("\n");
    
    // First pass: backup original data for debugging
    unsigned char *original_data = malloc(s->len);
    memcpy(original_data, s->data, s->len);
    
    if (ctx->archsz == 64) {
        for (unsigned int i = 0; i < s->len; i += 8) {
            unsigned long int val = *(unsigned long int*)(s->data + i);
            printf(" data[%04u] %016lx", i/8, val);
            
            if (val == 0) {
                printf(" (null)\n");
                continue;
            }
            
            // Be less strict about "special values"
            // Only skip obviously invalid values
            if (val < 0x1000 && val != 0) {
                // But check if it might be an index or count
                if (val < 256) {
                    printf(" (likely count/index: %lu - not relocating)\n", val);
                    continue;
                }
            }
            
            // Check for packed values (like 0x0000010100000001)
            if ((val & 0xFFFFFFFF) == 0x00000001 && (val >> 32) == 0x00000101) {
                printf(" (packed value - analyzing parts)\n");
                // This might be two related values packed together
                // For now, skip, but we might need to handle this specially
                continue;
            }
            
            // Don't skip large values - they might be valid addresses
            // The kernel virtual address space on x86_64 is vast
            
            int found_match = 0;
            msec_t *best_match = NULL;
            unsigned long best_addend = 0;
            
            // Try to find the best matching section
            msec_t *sec = 0;
            DL_FOREACH(ctx->mshdrs, sec) {
                if (!sec->s_bfd || !sec->s_bfd->vma || !sec->s_elf || !sec->s_elf->sh_size) {
                    continue;
                }
                
                // Check if value falls within section bounds
                if (val >= sec->s_bfd->vma && val < (sec->s_bfd->vma + sec->s_elf->sh_size)) {
                    // Skip self-references in .data
                    if (str_eq(sec->name, ".data")) {
                        printf(" (self-reference to %s - skipping)", sec->name);
                        goto next_entry;
                    }
                    
                    best_match = sec;
                    best_addend = val - sec->s_bfd->vma;
                    found_match = 1;
                    break;  // Take the first match
                }
            }
            
            if (found_match && best_match) {
                printf("  --> %s (%016lx-%016lx)", best_match->name, 
                       best_match->s_bfd->vma, best_match->s_bfd->vma + best_match->s_elf->sh_size);
                
                // Create relocation
                Elf_Rela *r = calloc(1, sizeof(Elf_Rela));
                r->r_offset = i;
                r->r_info = 0;  // Will be set in save_data_reloc
                r->r_addend = best_addend;

                // Determine section index
                unsigned int secindex = 0;
                if (str_eq(best_match->name, ".text")) {
                    secindex = 1;
                } else if (str_eq(best_match->name, ".rodata")) {
                    secindex = 2;
                } else if (str_eq(best_match->name, ".data")) {
                    secindex = 3;
                } else if (str_eq(best_match->name, ".bss")) {
                    secindex = 4;
                } else {
                    secindex = secindex_from_name_after_strip(ctx, best_match->name);
                    if (secindex == 0) {
                        printf(" // Unknown section %s", best_match->name);
                        free(r);
                        goto next_entry;
                    }
                }

                printf(" (secindex=%d, addend=0x%lx)", secindex, best_addend);
                save_data_reloc(ctx, r, secindex, 1); 
                
                // Zero out the original value
                *(unsigned long int*)(s->data + i) = 0;
                free(r);
            } else {
                printf(" (no section match - value might be encoded differently)");
                
                // Check if this might be a negative offset or encoded value
                if (val > 0x7fffffffffff) {
                    long signed_val = (long)val;
                    printf("\n    Checking as signed: %ld (0x%lx)", signed_val, signed_val);
                    
                    // Check if adding to any section base gives a valid address
                    msec_t *sec = 0;
                    DL_FOREACH(ctx->mshdrs, sec) {
                        if (!sec->s_bfd || !sec->s_bfd->vma) continue;
                        
                        long result = sec->s_bfd->vma + signed_val;
                        if (result > 0 && result < 0x7fffffffffff) {
                            msec_t *target = section_from_addr(ctx, result);
                            if (target) {
                                printf("\n    %s + %ld = 0x%lx (in %s)", 
                                       sec->name, signed_val, result, target->name);
                            }
                        }
                    }
                }
            }
            
            next_entry:
            printf("\n");
        }
    }
    
    // Call our debugging functions
    find_missing_relocation(ctx);
    verify_critical_relocations(ctx);
    
    free(original_data);
    return 0;
}
*/

int analyze_data_old(ctx_t *ctx, msec_t *s)
{
	unsigned int i = 0;
	msec_t *sec = 0, *tmp = 0;
	unsigned int secindex = 0;

	printf(" -- Analyzing .data section\n\n");
	hexdump(s->data, s->len);
	printf("\n");

	if (ctx->archsz == 64) {	// 64 bits

		for (i = 0; i < s->len; i += 8) {
			unsigned long int val = *(unsigned long int*)(s->data+i);
			printf(" data[%04u] %08x", i/8, val);


			DL_FOREACH_SAFE(ctx->mshdrs, sec, tmp) {
			
//				if ((val)&&(sec->s_bfd->vma)&&(val >= sec->s_bfd->vma)&&(sec->s_elf->sh_size)&&(val <= sec->s_bfd->vma + sec->s_elf->sh_size)&&(strncmp(sec->name, ".text", 6))&&(strncmp(sec->name, ".data", 6))) {
//					printf("  --> %s\ (%08x-%08x)", sec->name, sec->s_bfd->vma, sec->s_bfd->vma + sec->s_elf->sh_size);

if ((val) && (sec->s_bfd->vma) && 
    (val >= sec->s_bfd->vma) && (sec->s_elf->sh_size) && 
    (val <= sec->s_bfd->vma + sec->s_elf->sh_size)) {
    
        printf("  [DEBUG] val=0x%lx matches section %s (vma=0x%lx, end=0x%lx)\n", 
               val, sec->name, sec->s_bfd->vma, sec->s_bfd->vma + sec->s_elf->sh_size);
            
    printf("  --> %s (%08lx-%08lx)", sec->name, 
           sec->s_bfd->vma, sec->s_bfd->vma + sec->s_elf->sh_size);
    
    
					// Create relocation
					Elf_Rela *r = 0;
					r = calloc(1, sizeof(Elf_Rela));
					r->r_offset = i;
					r->r_info = 0;	// Computed later
					r->r_addend = val - sec->s_bfd->vma;

					if (!strncmp(sec->name, ".text", 6)) {
						secindex = 1;		/* section 1 is .text */
					} else if (!strncmp(sec->name, ".rodata", 8)) {
						secindex = 2;		/* section 2 is .rodata */
					} else if (!strncmp(sec->name, ".data", 6)) {
						secindex = 3;		/* section 3 is .data */					
					} else if (!strncmp(sec->name, ".bss", 5)) {
						secindex = 4;		/* section 4 is .bss */					
					} else {
						secindex = 0;		/* unknown ? */
						printf(" // Unknown relocation section !!");
					}

					// Save relocation
					save_data_reloc(ctx, r, secindex, 1); 

					// Nullify in .data section
					*(unsigned long int*)(s->data+i) = 0;//0xdeadbeef;					
				}
			}			
			
			printf("\n");
		}

	
	} else {			// 32 bits

	}

	return 0;
}

int analyze_text(ctx_t *ctx, char *data, unsigned int datalen, unsigned long int addr)
{
	csh handle;
	cs_insn *insn = 0;
	size_t count = 0;
	size_t j = 0;

	if (cs_open(CS_ARCH_X86, CS_MODE, &handle)) {
		printf("error: Failed to initialize capstone library\n");
		return -1;
	}
	// request disassembly details
	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

	count = cs_disasm(handle, (const uint8_t *) data, datalen - 1, addr, 0, &insn);
	if (!count) {
		printf("error: Cannot disassemble code\n");
		return -1;
	}

	if (ctx->opt_asmdebug) {
		printf(" -- parsing %lu instructions from %lx (.text) for relocations\n\n", count, addr);
		printf("\n  Offset          Info           Type           Sym. Value    Sym. Name + Addend\n");
	}
	// scan instructions for relocations
	for (j = 0; j < count; j++) {

		if (ctx->opt_asmdebug) {
			printf("0x%" PRIx64 ":\t%s\t%s\n", insn[j].address, insn[j].mnemonic, insn[j].op_str);
			print_insn_detail(ctx, handle, CS_MODE, &insn[j]);
		}

		parse_text_data_reloc(ctx, handle, CS_MODE, &insn[j]);
	}

	cs_free(insn, count);
	cs_close(&handle);

	return 0;
}

/**
* Read original symtab + strtab. BDF doesn't do this
*/
int rd_symtab(ctx_t *ctx)
{
	Elf *elf = 0;
	Elf_Scn *scn = NULL;
	GElf_Shdr shdr;
	int fd = 0;
	const char *binary = 0;
	struct stat sb;
	size_t shstrndx = 0;
	char *sname = 0;

	if (ctx->opt_debug) {
		printf(" -- searching for .strtab/.symtab\n");
	}

	elf_version(EV_CURRENT);

	fd = open(ctx->binname, O_RDONLY);
	fstat(fd, &sb);
	binary = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (binary == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (!elf) {
		printf("error: not a valid ELF\n");
		return -1;
	}

	if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
		printf("error: in elf_getshdrstrndx\n");
		return -1;
	}

	while ((scn = elf_nextscn(elf, scn)) != NULL) {
		gelf_getshdr(scn, &shdr);
		sname = elf_strptr(elf, shstrndx, shdr.sh_name);

		switch (shdr.sh_type) {
		case SHT_SYMTAB:
			if (ctx->opt_debug) {
				printf(" * symbol table at offset:%lu, sz:%lu\n", shdr.sh_offset, shdr.sh_size);
			}
			break;

		case SHT_STRTAB:
			if (str_eq(sname, ".dynstr")) {
				if (ctx->opt_debug) {
					printf(" * dynamic string table at offset:%lu, sz:%lu\n", shdr.sh_offset, shdr.sh_size);
				}
				save_dynstr(ctx, shdr, binary);
			} else {
				if (ctx->opt_debug) {
					printf(" * string table at offset:%lu, sz:%lu\n", shdr.sh_offset, shdr.sh_size);
				}
			}
			break;

		case SHT_DYNSYM:
			if (ctx->opt_debug) {
				printf(" * dynamic symbol table at offset:%lu, sz:%lu\n", shdr.sh_offset, shdr.sh_size);
			}
			save_dynsym(ctx, shdr, binary);
			break;

		default:
			break;
		}

	}

	elf_end(elf);
	close(fd);

	if ((ctx->opt_verbose) || (ctx->opt_debug)) {
		printf("\n");
	}

	return 0;
}

/**
* Suppress a given section
*/
int rm_section(ctx_t *ctx, char *name)
{
	msec_t *s = 0;
	msec_t *rmsec = 0;

	DL_FOREACH(ctx->mshdrs, s) {
		if (str_eq(s->name, name)) {
			rmsec = s;
			break;
		}
	}

	if (!rmsec) {
		return 0;
	}			// Not found

	DL_DELETE(ctx->mshdrs, rmsec);

	ctx->shnum--;
	ctx->mshnum--;

	return 0;
}

/**
* Strip binary relocation data
*/
int strip_binary_reloc(ctx_t *ctx)
{
	msec_t *s = 0, *tmp = 0;
	unsigned int allowed = 0, i = 0;

	if (ctx->opt_verbose) {
		printf("\n -- Stripping\n\n");

	}

	DL_FOREACH_SAFE(ctx->mshdrs, s, tmp) {
		allowed = 0;
		for (i = 0; i < sizeof(allowed_sections) / sizeof(char *); i++) {
			if (str_eq(s->name, allowed_sections[i])) {
				allowed = 1;
				break;
			}
		}

		if (!allowed) {
			if (ctx->opt_verbose) {
				printf(" * %s\n", s->name);
			}

			rm_section(ctx, s->name);
		}
	}

	if (ctx->opt_verbose) {
		printf("\n");
	}
	return 0;
}

/**
* Main routine
*/
unsigned int libify(ctx_t *ctx)
{
	char const *target = NULL;
	int is_pe64 = 0, is_pe32 = 0;

	/**
	*
	* LOAD OPERATIONS
	*
	*/

	/**
	* Load each section of binary using bfd
	*/
	load_binary(ctx);

	/**
	* Print BFD sections
	*/
	print_bfd_sections(ctx);

	/**
	* Read .text segment boundaries
	*/
	rd_extended_text(ctx);
	rd_extended_data(ctx);

	extend_text(ctx);

	/**
	* Open target binary
	*/
	open_target(ctx);

	/**
	* Read sections from disk
	*/
	rd_sections(ctx);


	create_section_symbols(ctx);

	/**
	* Read symtab + strtab : BFD doesn't do this
	*/
	rd_symtab(ctx);


	fixup_strtab_and_symtab(ctx);

	/**
	* Read symbols
	*/
	target = bfd_get_target(ctx->abfd);

	is_pe64 = (strcmp(target, "pe-x86-64") == 0 || strcmp(target, "pei-x86-64") == 0);
	is_pe32 = (strcmp(target, "pe-i386") == 0 || strcmp(target, "pei-i386") == 0 || strcmp(target, "pe-arm-wince-little") == 0 || strcmp(target, "pei-arm-wince-little") == 0);

	if ((is_pe64) || (is_pe32)) {
		printf("target: %s\n", target);
	} else {
		rd_symbols(ctx);
	}

	fixup_text(ctx);

	/**
	* Add extra symbols
	*/
	add_extra_symbols(ctx);

	/**
	* Parse relocations
	*/
	parse_relocations(ctx);

	/**
	* Fix section indexes in symtab
	*/
	fixup_symtab_section_index(ctx);

	process_text(ctx);
	
	process_data(ctx);

	/**
	* Add debugging and verification
	*/
//	if (ctx->opt_debug) {
		verify_critical_relocations(ctx);
		debug_crash_analysis(ctx);


//    analyze_crash_location(ctx);
    analyze_problematic_values(ctx);
    scan_all_data_sections(ctx);

//	}
	
	/**
	*
	* PROCESSING
	*
	*/

	/**
	* Copy each section content in output file
	*/
	copy_body(ctx);

	/**
	* Relocation stripping
	*/

	if ((ctx->opt_static) || (ctx->opt_reloc)) {
		rm_section(ctx, ".interp");
		rm_section(ctx, ".dynamic");
	}

	if (ctx->opt_reloc) {
		strip_binary_reloc(ctx);
	}

	/**
	* Create Program Headers
	*/
	if (!ctx->opt_original) {
		mk_phdrs(ctx);	// Create Program Headers from sections
	} else {
		// Read Original Program Headers
		rd_phdrs(ctx);
	}

	/**
	* Fix symtab
	*/
	fix_symtab_bindings(ctx);


	/**
	*
	* FINAL WRITE OPERATIONS
	*
	*/

	/**
	* Write strtab and symtab
	*/
	write_strtab_and_reloc(ctx);

	/**
	* Add section headers to output file
	*/
	if (!ctx->opt_sstrip) {
		write_shdrs(ctx);
	}

	/**
	* Add segment headers to output file
	*/
	if (!ctx->opt_reloc) {
		if (!ctx->opt_original) {
			write_phdrs(ctx);
		} else {
			write_phdrs_original(ctx);

		}
	}

	/**
	* Add ELF Header to output file
	*/
	mk_ehdr(ctx);

	/**
	* Finalize/Close/Cleanup
	*/

	return 0;
}

/**
* Print content of /proc/pid/maps
*/
int print_maps(void)
{
	char cmd[1024];

	memset(cmd, 0x00, 1024);
	sprintf(cmd, "cat /proc/%u/maps", getpid());
	system(cmd);
	return 0;
}

/**
* Initialize a reversing context
*/
ctx_t *ctx_init(void)
{
	ctx_t *ctx = 0;

	bfd_init();
	errno = 0;
	ctx = calloc(1, sizeof(ctx_t));
	if (!ctx) {
		printf("error: calloc(): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/**
	* Set default values
	*/
	ctx->strndx = calloc(1, DEFAULT_STRNDX_SIZE);
	ctx->mshdrs = NULL;
	return ctx;
}

int usage(char *name)
{
	printf("Usage: %s [options] file\n", name);
	printf("\noptions:\n\n");
	printf("    -o, --output           <output file>\n");
	printf("    -m, --march            <architecture>\n");
	printf("    -e, --entrypoint       <0xaddress>\n");
	printf("    -i, --interpreter      <interpreter>\n");
	printf("    -p, --poison           <poison>\n");
	printf("    -s, --shared\n");
	printf("    -c, --compile\n");
	printf("    -S, --static\n");
	printf("    -x, --strip\n");
	printf("    -X, --sstrip\n");
	printf("    -E, --exec\n");
	printf("    -C, --core\n");
	printf("    -O, --original\n");
	printf("    -k, --keep-main\n");
	printf("    -D, --disasm\n");
	printf("    -d, --debug\n");
	printf("    -h, --help\n");
	printf("    -v, --verbose\n");
	printf("    -V, --version\n");
	printf("\n");
	return 0;
}

int print_version(void)
{
	printf("%s version:%s    (%s %s)\n", WNAME, WVERSION, WTIME, WDATE);
	return 0;
}

int desired_arch(ctx_t *ctx, char *name)
{

	unsigned int i = 0;

	for (i = 0; i < sizeof(wccarch) / sizeof(archi_t); i++) {
		if (str_eq(wccarch[i].name, name)) {
			if (ctx->opt_verbose) {
				printf(" * architecture: %s\n", name);
			}
			return wccarch[i].value;
		}
	}

	printf("error: architecture %s not supported\n", name);
	exit(EXIT_FAILURE);

	return 0;
}

int ctx_getopt(ctx_t *ctx, int argc, char **argv)
{
	const char *short_opt = "ho:i:scSEsxCvVXp:Odm:e:f:Dk";
	int count = 0;
	struct stat sb;
	int c = 0;

	struct option long_opt[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "march", required_argument, NULL, 'm' },
		{ "output", required_argument, NULL, 'o' },
		{ "shared", no_argument, NULL, 's' },
		{ "compile", no_argument, NULL, 'c' },
		{ "debug", no_argument, NULL, 'd' },
		{ "disasm", no_argument, NULL, 'D' },
		{ "static", no_argument, NULL, 'S' },
		{ "exec", no_argument, NULL, 'E' },
		{ "core", no_argument, NULL, 'C' },
		{ "strip", no_argument, NULL, 'x' },
		{ "sstrip", no_argument, NULL, 'X' },
		{ "entrypoint", required_argument, NULL, 'e' },
		{ "interpreter", required_argument, NULL, 'i' },
		{ "poison", required_argument, NULL, 'p' },
		{ "original", no_argument, NULL, 'O' },
		{ "keep-main", no_argument, NULL, 'k' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "version", no_argument, NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};

	// Parse options
	if (argc < 2) {
		print_version();
		printf("\n");
		usage(argv[0]);
		exit(EXIT_SUCCESS);
	}

	while ((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		count++;
		switch (c) {
		case -1:	/* no more arguments */
		case 0:
			break;

		case 'c':
			ctx->opt_reloc = 1;
			break;

		case 'C':
			ctx->opt_core = 1;
			break;

		case 'd':
			ctx->opt_debug = 1;
			ctx->opt_verbose = 1;
			break;

		case 'D':
			ctx->opt_asmdebug = 1;
			break;

		case 'e':
			ctx->opt_entrypoint = strtoul(optarg, NULL, 16);
			count++;
			break;

		case 'E':
			ctx->opt_exec = 1;
			break;

		case 'f':
			ctx->opt_flags = strtoul(optarg, NULL, 16);
			count++;
			break;

		case 'h':
			usage(argv[0]);
			exit(0);
			break;

		case 'i':
			ctx->opt_interp = strdup(optarg);
			count++;
			break;

		case 'k':
			ctx->opt_keep_main = 1;
			break;

		case 'm':
			ctx->opt_arch = desired_arch(ctx, optarg);
			count++;
			break;

		case 'o':
			ctx->opt_binname = strdup(optarg);
			count++;
			break;

		case 'O':
			ctx->opt_original = 1;
			break;

		case 'p':
			ctx->opt_poison = optarg[0];
			count++;
			break;

		case 's':
			ctx->opt_shared = 1;
			break;

		case 'S':
			ctx->opt_static = 1;
			break;

		case 'v':
			ctx->opt_verbose = 1;
			break;

		case 'V':
			print_version();
			exit(EXIT_SUCCESS);
			break;

		case 'x':
			ctx->opt_strip = 1;
			break;

		case 'X':
			ctx->opt_sstrip = 1;
			break;

		case ':':
		case '?':
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			exit(-2);

		default:
			fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			exit(-2);
		};
	};

	// arguments sanity checks
	if (count >= argc - 1) {
		fprintf(stderr, "error: No source binary found in arguments.\n");
		fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
		exit(-2);
	}
	// verify target file exists
	if (stat(argv[count + 1], &sb)) {
		printf("error: Could not open file %s : %s\n", argv[count + 1], strerror(errno));
		exit(EXIT_FAILURE);
	}
	// store original size
	orig_sz = sb.st_size;

	if (ctx->opt_debug) {
		printf(" -- Analysing: %s\n", argv[count + 1]);
	}
	// copy target file name
	ctx->binname = strdup(argv[count + 1]);
	return 0;
}

/**
* Application Entry Point
*/
int main(int argc, char **argv)
{
	ctx_t *ctx = 0;

	ctx = ctx_init();
	ctx_getopt(ctx, argc, argv);
	libify(ctx);

	return 0;
}


int verify_critical_relocations(ctx_t *ctx)
{
    printf("\n=== Verifying Critical Relocations ===\n");
    
    // Check for critical sections that must have proper relocations
    msec_t *data_section = section_from_name(ctx, ".data");
    if (!data_section) {
        printf("ERROR: .data section not found\n");
        return -1;
    }
    
    // Look for patterns that commonly cause crashes
    printf("Checking .data section for critical pointers...\n");
    
    for (unsigned int i = 0; i < data_section->len; i += 8) {
        unsigned long *ptr = (unsigned long *)(data_section->data + i);
        unsigned long val = *ptr;
        
        // Check for values that might need relocation but were missed
        if (val > 0x1000 && val < 0x7fffffffffff) {
            // See if this value corresponds to any section
            msec_t *target = section_from_addr(ctx, val);
            if (target && !str_eq(target->name, ".data")) {
                // This looks like it should be relocated but might not be
                printf("WARNING: data[%04u] contains 0x%lx (-> %s) - might need relocation\n",
                       i/8, val, target->name);
                
                // Check if there's already a relocation for this offset
                int has_relocation = 0;
                for (unsigned int j = 0; j < globaldatareloclen / sizeof(Elf_Rela); j++) {
                    Elf_Rela *r = (Elf_Rela *)(globaldatareloc + j * sizeof(Elf_Rela));
                    if (r->r_offset == i) {
                        has_relocation = 1;
                        break;
                    }
                }
                
                if (!has_relocation) {
                    printf("  --> NO RELOCATION FOUND! This might cause crash.\n");
                }
            }
        }
    }
    
    // Verify that critical runtime structures are properly set up
    printf("\nChecking for critical runtime structures...\n");
    
    // Check GOT entries
    msec_t *got_section = section_from_name(ctx, ".got");
    if (got_section) {
        printf("Found .got section at VMA 0x%lx, size %lu\n", 
               got_section->s_bfd->vma, got_section->s_elf->sh_size);
    } else {
        printf("WARNING: No .got section found\n");
    }
    
    // Check PLT entries
    msec_t *plt_section = section_from_name(ctx, ".plt");
    if (plt_section) {
        printf("Found .plt section at VMA 0x%lx, size %lu\n", 
               plt_section->s_bfd->vma, plt_section->s_elf->sh_size);
    } else {
        printf("WARNING: No .plt section found\n");
    }
    
    return 0;
}

// Also add this debugging function to find exactly which data entry is causing the crash
void debug_crash_analysis(ctx_t *ctx)
{
    printf("\n=== Crash Analysis ===\n");
    
    // The crash happens when loading from 0x5555555803a8
    // We need to find which .data entry this corresponds to
    
    msec_t *data_section = section_from_name(ctx, ".data");
    if (!data_section) {
        printf("ERROR: .data section not found\n");
        return;
    }


    // In a typical PIE binary loaded at 0x555555554000:
    // - .text starts at file offset 0x1000, loads at 0x555555555000
    // - .data is at VMA 0x23000, loads at 0x555555577000
    
    // But let's calculate the actual offset
    unsigned long crash_loaded_addr = 0x5555555803a8;
    unsigned long likely_base = 0x555555554000;  // Common PIE base
    unsigned long data_vma = data_section->s_bfd->vma;  // Should be around 0x23000
    unsigned long text_base = 0x1000;  // Typical .text VMA in object file
    
    // Calculate where .data would be loaded
    unsigned long data_loaded_at = likely_base + (data_vma - text_base);
    
    if (crash_loaded_addr >= data_loaded_at && 
        crash_loaded_addr < data_loaded_at + data_section->len) {
        
        unsigned long offset = crash_loaded_addr - data_loaded_at;
        unsigned int entry_index = offset / 8;
        
        printf("Crash address 0x%lx maps to data[%04u]\n", crash_loaded_addr, entry_index);
        
        // Show what's at that location
        if (entry_index * 8 < data_section->len) {
            unsigned long *value_ptr = (unsigned long *)(data_section->data + entry_index * 8);
            unsigned long current_value = *value_ptr;
            
            printf("Current value at data[%04u]: 0x%lx\n", entry_index, current_value);
            
            if (current_value == 0) {
                printf("*** This is NULL! This explains the crash! ***\n");
                printf("This entry should have been relocated but wasn't.\n");
            }
        }
    } else {
        printf("Crash address 0x%lx is outside calculated .data range\n", crash_loaded_addr);
        printf("Data section VMA: 0x%lx, calculated load address: 0x%lx\n", 
               data_vma, data_loaded_at);
    }
}
    
// Function to analyze problematic values in more detail
void analyze_problematic_values(ctx_t *ctx)
{
    printf("\n=== Analyzing Problematic Values ===\n");
    
    // Value 1: ff01003fffffffff at data[0004]
    unsigned long val1 = 0xff01003ffffffff;
    printf("\n1. Value: 0x%lx (at data[0004])\n", val1);
    
    // This could be a negative offset when interpreted as signed
    long signed_val1 = (long)val1;
    printf("   As signed: %ld\n", signed_val1);
    
    // Check if it could be a relative address from .data
    msec_t *data_sec = section_from_name(ctx, ".data");
    if (data_sec && data_sec->s_bfd) {
        long relative_addr = data_sec->s_bfd->vma + signed_val1;
        printf("   .data base (0x%lx) + signed_val = 0x%lx\n", 
               data_sec->s_bfd->vma, relative_addr);
        
        if (relative_addr > 0 && relative_addr < 0x100000) {
            // Check if this points to any section
            msec_t *target = section_from_addr(ctx, relative_addr);
            if (target) {
                printf("   *** This would point to section: %s ***\n", target->name);
                printf("   *** This looks like a relative offset that needs relocation! ***\n");
            }
        }
    }
    
    // Check if it might be two 32-bit values
    unsigned int high32_1 = (val1 >> 32) & 0xFFFFFFFF;
    unsigned int low32_1 = val1 & 0xFFFFFFFF;
    printf("   High 32-bits: 0x%x (%u)\n", high32_1, high32_1);
    printf("   Low 32-bits: 0x%x (%u)\n", low32_1, low32_1);
    
    // Value 2: 0000010100000001 at data[0017]
    unsigned long val2 = 0x010100000001;
    printf("\n2. Value: 0x%lx (at data[0017])\n", val2);
    
    unsigned int high32_2 = (val2 >> 32) & 0xFFFFFFFF;
    unsigned int low32_2 = val2 & 0xFFFFFFFF;
    printf("   High 32-bits: 0x%x (%u)\n", high32_2, high32_2);
    printf("   Low 32-bits: 0x%x (%u)\n", low32_2, low32_2);
    
    // This pattern suggests it might be structured data
    printf("   This might be:\n");
    printf("   - Count/size in high part: %u\n", high32_2);
    printf("   - Flags/index in low part: %u\n", low32_2);
    printf("   - Or two separate 32-bit values that were incorrectly combined\n");
    
    // Let's also check the values we skipped as "small values"
    printf("\n=== Values classified as 'small' that might need attention ===\n");
    printf("Note: Some of these might be legitimate pointers in a low address space\n");
    printf("or offsets that need special handling.\n");
}

// Let's also create a function to scan all data sections, not just .data
void scan_all_data_sections(ctx_t *ctx)
{
    printf("\n=== Scanning All Data-Like Sections ===\n");
    
    msec_t *sec = 0;
    DL_FOREACH(ctx->mshdrs, sec) {
        // Check sections that might contain pointers
        if (str_eq(sec->name, ".data") || 
            str_eq(sec->name, ".data.rel.ro") ||
            str_eq(sec->name, ".got") ||
            str_eq(sec->name, ".bss") ||
            str_eq(sec->name, ".rodata")) {
            
            printf("\nAnalyzing section: %s\n", sec->name);
            printf("VMA: 0x%lx, Size: %lu bytes\n", 
                   sec->s_bfd ? sec->s_bfd->vma : 0, 
                   sec->s_elf ? sec->s_elf->sh_size : 0);
            
            if (sec->data && sec->len > 0) {
                // Look for null pointers that should have been relocated
                for (unsigned int i = 0; i < sec->len; i += 8) {
                    unsigned long *ptr = (unsigned long *)(sec->data + i);
                    if (*ptr == 0 && i < sec->len - 8) {
                        printf("  Null pointer at offset 0x%x (entry %u)\n", i, i/8);
                    }
                }
            }
        }
    }
}
