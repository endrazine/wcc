/**
* Test target for wsh globals tracing (globals_trace).
* Known, deterministic global accesses across .data, .bss and .rodata.
*
* Copyright 2016-2026 Jonathan Brossard. MIT License.
*/
#include <string.h>

int  g_initialized = 7;              /* .data  (read by bump)        */
int  g_counter     = 100;            /* .data  (read+written by bump)*/
char g_name[32]    = "default";      /* .data  (written by copy_msg) */
int  g_zero_bss[64];                 /* .bss   (written by copy_msg) */
const char g_rodata_msg[] = "hello"; /* .rodata(read by copy_msg)    */

/* Reads g_initialized (.data), reads+writes g_counter (.data). */
int bump(void)
{
	g_counter += g_initialized;
	return g_counter;
}

/* Reads g_rodata_msg (.rodata), writes g_name (.data) and g_zero_bss (.bss). */
int copy_msg(void)
{
	strcpy(g_name, g_rodata_msg);
	g_zero_bss[0] = 0x2a;
	return (int)g_name[0] + g_zero_bss[0];
}
