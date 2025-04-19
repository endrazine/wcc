/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2025 Jonathan Brossard
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
