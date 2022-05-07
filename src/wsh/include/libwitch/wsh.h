/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2022 Jonathan Brossard
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

#define _GNU_SOURCE
#include <sys/prctl.h>
#include <setjmp.h>
#include <link.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <malloc.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <ctype.h>
#include <execinfo.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/ptrace.h>

#define USE_LUA 1
// Use either lua or luajit
#ifdef USE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#else
#include "mylaux.h"
#include "luajit.h"
#endif

#include <linenoise.h>
#include "helper.h"
#include <colors.h>
#include <config.h>
#include <uthash.h>


#define DEFAULT_SCRIPT		"/usr/share/wcc/scripts/debug"
#define DEFAULT_SCRIPT_INDEX	"/usr/share/wcc/scripts/INDEX"
#define DEFAULT_WSHRC		".wshrc"
#define DEFAULT_WSH_PROFILE	".wsh_profile"
#define PROC_ASLR_PATH		"/proc/sys/kernel/randomize_va_space"

#define DEFAULT_LEARN_FILE "./learnwitch.log"

#define MAX_SIGNALS 2000000

#define MY_CPU 1		// Which CPU to set affinity to

#define BIND_FLAGS             RTLD_NOW

/*
#define save_context(c){ \
	memset(c, 0x00, sizeof(ucontext_t)); \
	kill(getpid(),42); \
} \
*/

#define         DMGL_PARAMS   (1 << 0)
#define         DMGL_ANSI   (1 << 1)
#define         DMGL_ARM   (1 << 11)

#ifdef __LP64__ // Generic 64b
#define Elf_Dyn  Elf64_Dyn
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym  Elf64_Sym
#else
#define Elf_Dyn  Elf32_Dyn
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym  Elf32_Sym
#endif

#define HPERMSMAX 5

#define ELF32_ST_BIND(val)              (((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)              ((val) & 0xf)
#define ELF32_ST_INFO(bind, type)       (((bind) << 4) + ((type) & 0xf))

#define ELF64_ST_BIND(val)              ELF32_ST_BIND (val)
#define ELF64_ST_TYPE(val)              ELF32_ST_TYPE (val)
#define ELF64_ST_INFO(bind, type)       ELF32_ST_INFO ((bind), (type))

#define STB_LOCAL          0
#define STB_GLOBAL         1
#define STB_WEAK           2
#define STB_GNU_UNIQUE     10
#define STB_GNU_SECONDARY  11

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_COMMON  5
#define STT_TLS     6


#define LINES_MAX 50


/**
* Read arg1
*/
#define read_arg1(arg1){ \
	if (lua_isnil(L, 1)) { \
		arg1 = 0; \
	} else if (lua_isnumber(L, 1)) { \
		arg1 = (unsigned long) lua_tonumber(L, 1); \
	} else if (lua_isstring(L, 1)) { \
		arg1 = luaL_checkstring(L, 1); \
	} else if (lua_istable(L, 1)) { \
	} else if (lua_isfunction(L, 1)) { \
		arg1 = lua_tocfunction(L, 1); \
	} else if (lua_iscfunction(L, 1)) { \
		arg1 = lua_touserdata(L, 1); \
	} else if (lua_isuserdata(L, 1)) { \
		arg1 = lua_touserdata(L, 1); \
	} else { \
		arg1 = 0; \
	} \
}

/**
* Read arg2
*/
#define read_arg2(arg2){ \
	if (lua_isnil(L, 2)) { \
		arg2 = 0; \
	} else if (lua_isnumber(L, 2)) { \
		arg2 = (unsigned long) lua_tonumber(L, 2); \
	} else if (lua_isstring(L, 2)) { \
		arg2 = luaL_checkstring(L, 2); \
	} else if (lua_istable(L, 2)) { \
	} else if (lua_isfunction(L, 2)) { \
		arg2 = lua_tocfunction(L, 2); \
	} else if (lua_iscfunction(L, 2)) { \
		arg2 = lua_touserdata(L, 2); \
	} else if (lua_isuserdata(L, 2)) { \
		arg2 = lua_touserdata(L, 2); \
	} else { \
		arg2 = 0; \
	} \
}

/**
* Read arg3
*/
#define read_arg3(arg3){ \
	if (lua_isnil(L, 3)) { \
		arg3 = 0; \
	} else if (lua_isnumber(L, 3)) { \
		arg3 = (unsigned long) lua_tonumber(L, 3); \
	} else if (lua_isstring(L, 3)) { \
		arg3 = luaL_checkstring(L, 3); \
	} else if (lua_istable(L, 3)) { \
	} else if (lua_isfunction(L, 3)) { \
		arg3 = lua_tocfunction(L, 3); \
	} else if (lua_iscfunction(L, 3)) { \
		arg3 = lua_touserdata(L, 3); \
	} else if (lua_isuserdata(L, 3)) { \
		arg3 = lua_touserdata(L, 3); \
	} else { \
		arg3 = 0; \
	} \
}

/**
* Read arg4
*/
#define read_arg4(arg4){ \
	if (lua_isnil(L, 4)) { \
		arg4 = 0; \
	} else if (lua_isnumber(L, 4)) { \
		arg4 = (unsigned long) lua_tonumber(L, 4); \
	} else if (lua_isstring(L, 4)) { \
		arg4 = luaL_checkstring(L, 4); \
	} else if (lua_istable(L, 4)) { \
	} else if (lua_isfunction(L, 4)) { \
		arg4 = lua_tocfunction(L, 4); \
	} else if (lua_iscfunction(L, 4)) { \
		arg4 = lua_touserdata(L, 4); \
	} else if (lua_isuserdata(L, 4)) { \
		arg4 = lua_touserdata(L, 4); \
	} else { \
		arg4 = 0; \
	} \
}

/**
* Read argument number j
*/
#define read_arg(arg, j){ \
	if (lua_isnil(L, j)) { \
		arg = 0; \
	} else if (lua_isnumber(L, j)) { \
		arg = (unsigned long) lua_tonumber(L, j); \
	} else if (lua_isstring(L, j)) { \
		arg = luaL_checkstring(L, j); \
	} else if (lua_istable(L, j)) { \
	} else if (lua_isfunction(L, j)) { \
		arg = lua_tocfunction(L, j); \
	} else if (lua_iscfunction(L, j)) { \
		arg = lua_touserdata(L, j); \
	} else if (lua_isuserdata(L, j)) { \
		arg = lua_touserdata(L, j); \
	} else { \
		arg = 0; \
	} \
}

#define SHELL_HISTORY_NAME ".wsh_history"
#define luaL_reg      luaL_Reg

#define MIN_BIN_SIZE 10

#define FAULT_READ 	1
#define FAULT_WRITE	2
#define FAULT_EXEC	4

#define default_poison 0x61

/**
* Backtrace parameters
*/
#ifdef DEBUG
#define SKIP_INIT        0	// for developpment
#define SKIP_BOTTOM      0
#else
#define SKIP_INIT        3
#define SKIP_BOTTOM      13
#endif

/**
* Imported declarations prototypes
*/
char *cplus_demangle(const char *mangled, int options);

/**
* Imported globals
*/
extern char *__progname_full;

/**
* Forward prototypes declarations
*/
static struct link_map *do_loadlib(char *libname);
static int empty_phdrs(void);
static int wsh_appear(lua_State * L);
static int wsh_hide(lua_State * L);
static int empty_shdrs(void);
//int getarray(lua_State * L);
static int getsize(lua_State * L);
static int newarray(lua_State * L);
static int print_functions(lua_State * L);
static int print_libs(lua_State * L);
static int print_objects(lua_State * L);
static int print_phdrs(void);
static int print_shdrs(void);
static int entrypoints(lua_State * L);
static int print_symbols(lua_State * L);
static int print_version(void);
static int setarray(lua_State * L);
static int usage(char *name);
static void set_align_flag(void);
static void set_branch_flag(void);
static void set_trace_flag(void);
static void singlebranch(lua_State * L);
static void singlestep(lua_State * L);
static void traceunaligned(lua_State * L);
static void unset_align_flag(void);
static void unset_branch_flag(void);
static void unset_trace_flag(void);
static void unsinglebranch(lua_State * L);
static void unsinglestep(lua_State * L);
static void untraceunaligned(lua_State * L);
static void unverbosetrace(lua_State * L);
static void verbosetrace(lua_State * L);
static void xfree(lua_State * L);

static void systrace(lua_State * L);
static void rtrace(lua_State * L);
static void unsystrace(lua_State * L);
static void unrtrace(lua_State * L);


static int add_symbol(char *symbol, char *libname, char *htype, char *hbind, unsigned long value, unsigned int size, unsigned long int addr);
static void segment_add(unsigned long int addr, unsigned long int size, char *perms, char *fname, char *ptype, int flags);

static int alloccharbuf(lua_State * L);
static int bfmap(lua_State * L);
static int teletype(lua_State * L);
static int breakpoint(lua_State * L);
static int execlib(lua_State * L);
static int getcharbuf(lua_State * L);
static int grep(lua_State * L);
static int grepptr(lua_State * L);
static int help(lua_State * L);
static int hollywood(lua_State * L);
static int info(lua_State * L);
static int libcall(lua_State * L);
static int loadbin(lua_State * L);
static int man(lua_State * L);
static int map(lua_State * L);
static int phdrs(lua_State * L);
static int priv_memcpy(lua_State * L);
static int priv_strcat(lua_State * L);
static int priv_strcpy(lua_State * L);
static int rdnum(lua_State * L);
static int rdstr(lua_State * L);
static int setcharbuf(lua_State * L);
static int shdrs(lua_State * L);
static int verbose(lua_State * L);
static int xalloc(lua_State * L);
static int ralloc(lua_State * L);

static int headers(lua_State * L);
static int prototypes(lua_State * L);
static int bsspolute(lua_State * L);

static unsigned int ltrace(void);
static int procmap_lua(void);
static void rescan(void);
static void hexdump(uint8_t * data, size_t size, size_t colorstart, size_t color_len);
static int disable_aslr(void);
static int enable_aslr(void);
static int run_script(char *name);

static int enable_core(lua_State * L);
static int disable_core(lua_State * L);
static int gencore(lua_State * L);

static char *signaltoname(int signal);
static char *sicode_strerror(int signal, siginfo_t * s);

/*
int memmap (lua_State *L);
int newmemmap(lua_State * L);
int getmemmap(lua_State * L);
int setmemmap(lua_State * L);
int memmapsize(lua_State * L);
*/

static int rawmemread  (lua_State *L);
static int rawmemwrite (lua_State *L);
static int rawmemstr   (lua_State *L);
static int rawmemusage (lua_State *L);
static int rawmemaddr  (lua_State *L);
static int rawmemstrlen(lua_State *L);

static char *lua_strerror(int err);

/**
* Internal representation of an ELF
*/
typedef struct {
	bool et_dyn;
	Elf_Dyn *dyns;
	Elf_Ehdr *ehdr;
	Elf_Phdr *phdrs;
	uint32_t dyn_index;
	uintptr_t base, limit;
	uintptr_t *p_pltgot;
	struct r_debug *r_debug;
	struct link_map *link_map;

} elfdata_t;

/**
* Memory ranges
*/
typedef struct range_t {
	unsigned long long int min;
	unsigned long long int max;
} range_t;


/**
* Breakpoint structure
*/
typedef struct breakpoint_t {
	char *ptr;		// Pointer to location in memory
	char backup;		// Backup bytes
	unsigned int weight;	// Weight (optional)
} breakpoint_t;


/**
* Libraries to be preloaded
* (before shell/script execution)
*/
typedef struct preload_t {
	char *name;

	struct preload_t *prev;	// utlist.h
	struct preload_t *next;	// utlist.h

} preload_t;

/**
* Scripts to be executed
*/
typedef struct script_t {
	char *name;

	struct preload_t *prev;	// utlist.h
	struct preload_t *next;	// utlist.h
} script_t;

/**
* Representation of ELF Sections
*/
typedef struct sections_t {
	unsigned long int addr;
	unsigned long int size;
	char *libname;
	char *name;
	char *perms;
	int flags;

	struct sections_t *prev;	// utlist.h
	struct sections_t *next;	// utlist.h

} sections_t;

/**
* Representation of ELF Segments
*/
typedef struct segments_t {
	unsigned long int addr;
	unsigned long int size;
	char *libname;
	char *type;
	char *perms;
	int flags;

	struct segments_t *prev;	// utlist.h
	struct segments_t *next;	// utlist.h

} segments_t;

/**
* Representation of ELF Symbols
*/
typedef struct symbols_t {
	unsigned long int addr;
	unsigned long int size;
	char *symbol;
	char *libname;
	char *htype;
	char *hbind;
	unsigned long int value;

	struct symbols_t *prev;	// utlist.h
	struct symbols_t *next;	// utlist.h

} symbols_t;

typedef struct eps_t {
	unsigned long long int addr;
	char *name;

	struct eps_t *prev;	// utlist.h
	struct eps_t *next;	// utlist.h

} eps_t;

/**
* wsh context
*/
typedef struct wsh_t {

	// State
	lua_State *L;
	char *luabuff;
	unsigned int luabuffsz;

	char *selflib;
	char *learnlog;
	FILE *learnfile;

	unsigned long long int mainhandle;	// This is really a struct link_map *

	unsigned int opt_verbose;
	unsigned int opt_quiet;
	unsigned int opt_hollywood;	// Default = 1;

	unsigned int opt_rescan;
	unsigned int opt_verbosetrace;	// Display verbose trace
	unsigned int opt_appear;	// Display ourselves or hide ourselves ?

	unsigned int firsterrno;
	unsigned int firstsicode;
	unsigned int firstsignal;
	unsigned int totsignals;	// Per libcall
	unsigned int globalsignals;	// Never reset
	unsigned long int faultaddr;

	void *firstcontext;
	unsigned int reason;
	//unsigned int lastret;
	unsigned int is_stdinscript;
	unsigned int bp_points;

	void *pltgot;
	unsigned int pltsz;

	ucontext_t *errcontext;
//	ucontext_t *initcontext;

	unsigned long int btcaller;

	unsigned int libified;

	breakpoint_t *bp_array;
	unsigned int bp_num;

	unsigned int opt_argc;
	char *opt_argv;

	char **script_args;
	unsigned int script_argnum;

	unsigned int trace_unaligned;
	unsigned int trace_singlestep;
	unsigned int trace_singlebranch;

	unsigned int trace_rtrace;
	unsigned int trace_strace;

	unsigned int singlestep_count;
	unsigned int singlebranch_count;
	unsigned int sigbus_count;

	unsigned long long int singlestep_hash;
	unsigned long long int singlebranch_hash;
	unsigned long long int sigbus_hash;

	jmp_buf longjmp_ptr_high;
	jmp_buf longjmp_ptr;

	unsigned int interrupted;
	unsigned int longjmp_ptr_high_cnt;

	struct sections_t *shdrs;
	struct segments_t *phdrs;
	struct symbols_t *symbols;
	struct eps_t *eps;

	struct preload_t *preload;	// Libraries/binaries to preload
	struct script_t *scripts;	// Queue of scripts to execute

} wsh_t;

/**
* The next structure define
* how prototypes are learned
* by analysing runtime experiences
*/
typedef struct tuple_t{
	void *addr;
	char *name;
} tuple_t;

typedef struct learn_key_t{

	char ttype[10];
	char tlib[200];
	char tfunction[200];
	char targ[20];
	char tvalue[200];
}learn_key_t;

typedef struct learn_t{
	learn_key_t key;
	char toffset[20];
	UT_hash_handle hh;
} learn_t;

int wsh_init(void);
int wsh_getopt(int argc, char **argv);
int wsh_loadlibs(void);
int reload_elfs(void);
int wsh_run(void);
int wsh_usage(char *name);
int wsh_print_version(void);
void _exit(int status);
void exit(int status);

/*
int newarray(lua_State * L);
int setarray(lua_State * L);
int getarray(lua_State * L);
int getsize(lua_State * L);
*/

