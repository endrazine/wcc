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

