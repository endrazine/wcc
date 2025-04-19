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

/**
* Internal help datastructure
*/
typedef struct help_t{
	char *name;
	char *proto;
	char *descr;
	char *protoprefix;
	char *retval;
}help_t;

/**
* Internal help : wsh commands
*/
help_t cmdhelp[] ={
//	{"help", "[topic]","Display general help."},
	{"quit", "", "Exit wsh.", "", "Does not return : exit wsh\n"},
	{"exit", "", "Exit wsh.", "", "Does not return : exit wsh\n"},
	{"shell", "[command]", "Run a /bin/sh shell.", "", "None. Returns uppon shell termination."},
	{"exec", "<command>", "Run <command> via the system() library call.", "", "None. Returns uppon <command> termination."},
	{"clear", "", "Clear terminal.", "", "None."},
};

/**
* Internal help : wsh functions
*/
help_t fcnhelp[] ={
	{"help", "[topic]","Display help on [topic]. If [topic] is ommitted, display general help.", "", "None"},
	{"man", "[page]", "Display system manual page for [page].", "", "None"},
	{"hexdump", "<address>, <num>", "Display <num> bytes from memory <address> in enhanced hexadecimal form.", "", "None"},
	{"hex", "<object>", "Display lua <object> in enhanced hexadecimal form.", "", "None"},
	{"phdrs", "", "Display ELF program headers from all binaries loaded in address space.", "", "None"},
	{"shdrs", "", "Display ELF section headers from all binaries loaded in address space.", "", "None"},
	{"map", "", "Display a table of all the memory ranges mapped in memory in the address space.", "", "None"},
	{"procmap", "", "Display a table of all the memory ranges mapped in memory in the address space as displayed in /proc/<pid>/maps.", "", "None"},
	{"bfmap", "", "Bruteforce valid mapped memory ranges in address space.", "", "None"},
	{"symbols", "[sympattern], [libpattern], [mode]", "Display all the symbols in memory matching [sympattern], from library [libpattern]. If [mode] is set to 1 or 2, do not wait user input between pagers. [mode] = 2 provides a shorter output.", "", "None"},
	{"functions","[sympattern], [libpattern], [mode]", "Display all the functions in memory matching [sympattern], from library [libpattern]. If [mode] is set to 1 or 2, do not wait user input between pagers. [mode] = 2 provides a shorter output.", "table func = ", "Return 1 lua table _func_ whose keys are valid function names in address space, and values are pointers to them in memory."},
	{"objects","[pattern]", "Display all the functions in memory matching [sympattern]", "", "None"},
	{"info", "[address] | [name]", "Display various informations about the [address] or [name] provided : if it is mapped, and if so from which library and in which section if available.", "", "None"},
	{"search", "<pattern>", "Search all object names matching <pattern> in address space.", "", "None"},
	{"headers", "", "Display C headers suitable for linking against the API loaded in address space.", "", "None"},
	{"grep", "<pattern>, [patternlen], [dumplen], [before]","Search <pattern> in all ELF sections in memory. Match [patternlen] bytes, then display [dumplen] bytes, optionally including [before] bytes before the match. Results are displayed in enhanced decimal form", "table match = ", "Returns 1 lua table containing matching memory addresses."},
	{"grepptr", "<pattern>, [patternlen], [dumplen], [before]","Search pointer <pattern> in all ELF sections in memory. Match [patternlen] bytes, then display [dumplen] bytes, optionally including [before] bytes before the match. Results are displayed in enhanced decimal form", "table match = ", "Returns 1 lua table containing matching memory addresses."},
	{"loadbin","<pathname>","Load binary to memory from <pathname>.", "", "None"},
	{"libs", "", "Display all libraries loaded in address space.", "table libraries = ", "Returns 1 value: a lua table _libraries_ whose values contain valid binary names (executable/libraries) mapped in memory."},
	{"entrypoints", "", "Display entry points for each binary loaded in address space.", "", "None"},
	{"rescan", "", "Re-perform address space scan.", "", "None"},
	{"libcall", "<function>, [arg1], [arg2], ... arg[6]", "Call binary <function> with provided arguments.", "void *ret, table ctx = ", "Returns 2 return values: _ret_ is the return value of the binary function (nill if none), _ctx_ a lua table representing the execution context of the library call.\n"},
	{"enableaslr", "", "Enable Address Space Layout Randomization (requires root privileges).", "", "None"},
	{"disableaslr", "", "Disable Address Space Layout Randomization (requires root privileges).", "", "None"},
	{"verbose", "<verbosity>", "Change verbosity setting to <verbosity>.", "", "None"},
	{"breakpoint", "<address>, [weight]", "Set a breakpoint at memory <address>. Optionally add a <weight> to breakpoint score if hit.", "", "None"},
	{"bp", "<address>, [weight]", "Set a breakpoint at memory <address>. Optionally add a <weight> to breakpoint score if hit. Alias for breakpoint() function.", "", "None"},
	{"hollywood", "<level>", "Change hollywood (fun) display setting to <level>, impacting color display (enable/disable).", "", "None"},
};

