/**
* Test target for wsh on-disk .symtab name resolution.
*
* These globals are file-local (static), so they live only in .symtab -- not
* in the runtime .dynsym. Building this UNstripped lets wsh's load_symtab()
* path resolve them by name; a stripped build would fall back to
* <section+offset>.
*
* Copyright 2016-2026 Jonathan Brossard. MIT License.
*/

static int s_counter = 50;   /* .data, file-local */
static int s_bss[16];        /* .bss,  file-local */

/* Exported so wsh can call it; reads+writes the two static globals. */
int touch(void)
{
	s_counter += 1;
	s_bss[0] = s_counter;
	return s_bss[0];
}
