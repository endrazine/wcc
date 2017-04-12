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

/**
*
* Generated with:
*   find /bin/ /sbin/ /usr/sbin/|while read mama; do readelf -S $mama 2>/dev/null|grep "\["|sed s#"\[.*\]"##gi|awk '{print "{\"" $1 "\", SHT_" $2 ", \"SHT_" $2 "\"}," }'; done|sort -u
*
*/

#define SHT_VERSYM 0x6fffffff
#define SHT_VERNEED 0x6ffffffe
#define SHT_GNU_HASH	  0x6ffffff6	/* GNU-style hash table.  */

typedef struct assoc_nametotype_t{
	char *name;
	unsigned int type;
	char *htype;
}assoc_nametotype_t;

assoc_nametotype_t nametotype[] = {
{".bss", SHT_NOBITS, "SHT_NOBITS"},
{".comment", SHT_PROGBITS, "SHT_PROGBITS"},
{".ctors", SHT_PROGBITS, "SHT_PROGBITS"},
{".data", SHT_PROGBITS, "SHT_PROGBITS"},
{".data.rel.ro", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_abbrev", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_aranges", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_info", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_line", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_loc", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_macinfo", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_macro", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_pubnames", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_pubtypes", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_ranges", SHT_PROGBITS, "SHT_PROGBITS"},
{".debug_str", SHT_PROGBITS, "SHT_PROGBITS"},
{".dtors", SHT_PROGBITS, "SHT_PROGBITS"},
{".dynamic", SHT_DYNAMIC, "SHT_DYNAMIC"},
{".dynstr", SHT_STRTAB, "SHT_STRTAB"},
{".dynsym", SHT_DYNSYM, "SHT_DYNSYM"},
{".eh_frame", SHT_PROGBITS, "SHT_PROGBITS"},
{".eh_frame_hdr", SHT_PROGBITS, "SHT_PROGBITS"},
{".fini", SHT_PROGBITS, "SHT_PROGBITS"},
{".fini_array", SHT_FINI_ARRAY, "SHT_FINI_ARRAY"},
{".gcc_except_table", SHT_PROGBITS, "SHT_PROGBITS"},
{".gnu.hash", SHT_GNU_HASH, "SHT_GNU_HASH"},
{".gnu.version", SHT_VERSYM, "SHT_VERSYM"},
{".gnu.version_r", SHT_VERNEED, "SHT_VERNEED"},
{".gnu_debuglink", SHT_PROGBITS, "SHT_PROGBITS"},
{".got", SHT_PROGBITS, "SHT_PROGBITS"},
{".got.plt", SHT_PROGBITS, "SHT_PROGBITS"},
{".hash", SHT_HASH, "SHT_HASH"},
{".init", SHT_PROGBITS, "SHT_PROGBITS"},
{".init_array", SHT_INIT_ARRAY, "SHT_INIT_ARRAY"},
{".interp", SHT_PROGBITS, "SHT_PROGBITS"},
{".jcr", SHT_PROGBITS, "SHT_PROGBITS"},
{".modinfo", SHT_PROGBITS, "SHT_PROGBITS"},
{".module_license", SHT_PROGBITS, "SHT_PROGBITS"},
{".note.ABI-tag", SHT_NOTE, "SHT_NOTE"},
{".note.gnu.build-id", SHT_NOTE, "SHT_NOTE"},
{".plt", SHT_PROGBITS, "SHT_PROGBITS"},
{".plt.got", SHT_PROGBITS, "SHT_PROGBITS"},
{".rel.dyn", SHT_REL, "SHT_REL"},
{".rel.plt", SHT_REL, "SHT_REL"},
{".rela.dyn", SHT_RELA, "SHT_RELA"},
{".rela.plt", SHT_RELA, "SHT_RELA"},
{".rodata", SHT_PROGBITS, "SHT_PROGBITS"},
{".shstrtab", SHT_STRTAB, "SHT_STRTAB"},
{".strtab", SHT_STRTAB, "SHT_STRTAB"},
{".symtab", SHT_SYMTAB, "SHT_SYMTAB"},
{".tbss", SHT_NOBITS, "SHT_NOBITS"},
{".tdata", SHT_PROGBITS, "SHT_PROGBITS"},
{".text", SHT_PROGBITS, "SHT_PROGBITS"},
{"__cmd", SHT_PROGBITS, "SHT_PROGBITS"},
{"__debug", SHT_PROGBITS, "SHT_PROGBITS"},
{"__libc_atexit", SHT_PROGBITS, "SHT_PROGBITS"},
{"__libc_freeres_fn", SHT_PROGBITS, "SHT_PROGBITS"},
{"__libc_freeres_pt", SHT_NOBITS, "SHT_NOBITS"},
{"__libc_subfreeres", SHT_PROGBITS, "SHT_PROGBITS"},
{"__libc_thread_fre", SHT_PROGBITS, "SHT_PROGBITS"},
{"__libc_thread_sub", SHT_PROGBITS, "SHT_PROGBITS"},
{"pl_arch", SHT_PROGBITS, "SHT_PROGBITS"}
};

