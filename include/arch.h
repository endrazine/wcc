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

#include "elf-em.h"

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

