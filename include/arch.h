#include <linux/elf-em.h>

typedef struct archi_t{
	char *name;
	unsigned int value;
}archi_t;

archi_t wccarch[] = {
// nicknames
{"x86_64",	EM_X86_64},
{"x86-64",	EM_X86_64},
{"amd64",	EM_X86_64},
{"i386",	EM_386},
{"arm",		EM_ARM},
// official names
{"NONE", EM_NONE },
{"M32", EM_M32 },
{"SPARC", EM_SPARC },
{"386", EM_386 },
{"68K", EM_68K },
{"88K", EM_88K },
{"486", EM_486 },
{"860", EM_860 },
{"MIPS", EM_MIPS },
{"MIPS_RS3_LE", EM_MIPS_RS3_LE },
{"MIPS_RS4_BE", EM_MIPS_RS4_BE },
{"PARISC", EM_PARISC },
{"SPARC32PLUS", EM_SPARC32PLUS },
{"PPC", EM_PPC },
{"PPC64", EM_PPC64 },
{"SPU", EM_SPU },
{"ARM", EM_ARM },
{"SH", EM_SH },
{"SPARCV9", EM_SPARCV9 },
{"IA_64", EM_IA_64 },
{"X86_64", EM_X86_64 },
{"S390", EM_S390 },
{"CRIS", EM_CRIS },
{"V850", EM_V850 },
{"M32R", EM_M32R },
{"MN10300", EM_MN10300 },
{"BLACKFIN", EM_BLACKFIN },
{"TI_C6000", EM_TI_C6000 },
{"AARCH64", EM_AARCH64 },
{"FRV", EM_FRV }
};

