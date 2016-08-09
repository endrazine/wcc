

     The Witchcraft Compiler Collection



 -- Purpose:

WCC is a collection of reverse engineering tools
taking binary files as an input.

 -- Commands:

* wcc : The Witchcraft Compiler.
        Takes binaries (ELF, PE, ...) as an input
        and creates valid ELF binaries (eg: relocatable
        objects or shared libraries) that can be later
        compiled and linked with using regular compilers
        (eg: gcc/clang).

* wld : The Witchcraft Linker.
        Takes a valid ELF (32/64) binary as an input
        and changes its ELF class to ELF_DYN.

* wsh : The Witchcraft Loader and Dynamic Linker.
        Takes a valid ELF as an input, loads it in its own
        the address space, loads dependencies, solves relocations
        and execute an embedded (lua) shell or runs a script.

 -- Building and Installing:

	From the directory containing this file, type:
        $ make

	Then to install wcc, type:
	$ sudo make install

 -- External dependencies:

        Glibc, libbfd, libdl, zlib.

 -- Licences:

        See the file named LICENSE.

