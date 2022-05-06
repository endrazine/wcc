/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2022 Jonathan Brossard
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
*   find /usr/sbin/|while read mama; do readelf -S $mama 2>/dev/null|grep "\[" -A 1|sed s#"\[.*\]"#"\nKOALA\n"#gi|tr -d "\n"|sed s#"KOALA"#"\n"#gi|sed s#"[WAX]\{1,3\}"##g|awk '{print "{\"" $1 "\", " $6 "}," }'|grep "[0-9]" ; done|sort -u
*
*/


typedef struct assoc_nametosz_t{
	char *name;
	unsigned int sz;
}assoc_nametosz_t;

assoc_nametosz_t nametosize[] = {
#ifdef __LP64__	// Generic 64b
{".bss", 0x00},
{".comment", 0x01},
{".debug_str", 0x01},
{".dynamic", 0x10},
{".dynsym", 0x18},
{".got", 0x08},
{".got.plt", 0x08},
{".hash", 0x04},
{".plt", 0x10},
{".rela.dyn", 0x18},
{".rela.plt", 0x18},
{".rel.dyn", 0x18},
{".rel.plt", 0x18},
{".symtab", 0x18}
#else		// Generic 32b
{".bss", 0x00},
{".comment", 0x01},
{".debug_str", 0x01},
{".dynamic", 0x8},
{".dynsym", 0x10},
{".got", 0x04},
{".got.plt", 0x04},
{".hash", 0x04},
{".plt", 0x10},
{".rela.dyn", 0xc},
{".rela.plt", 0xc},
//{".rel.dyn", 0x8},
//{".rel.plt", 0x8},
{".rel.dyn", 0x8},
{".rel.plt", 0x8},
{".symtab", 0x10}
#endif
};

