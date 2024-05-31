

#     The Witchcraft Compiler Collection

Welcome to the Witchcraft Compiler Collection !

## Purpose
WCC is a collection of compilation tools to perform binary black magic on the GNU/Linux and other POSIX platforms.

## User manual

The WCC user manual is available online at : https://github.com/endrazine/wcc/wiki

## Installation

### Installation Requirements
The Witchcraft Compiler Collection requires the following software to be installed:

    capstone, glibc, libbfd, libdl, zlib, libelf, libreadline, libgsl, make

### Installation Requirements on Ubuntu/Debian
Under Ubuntu/Debian those dependencies can be installed with the following commands (tested on Ubuntu 22.04):
    
    sudo apt-get install -y clang libbfd-dev uthash-dev libelf-dev libcapstone-dev  libreadline-dev libiberty-dev libgsl-dev build-essential git debootstrap file

## Building and Installing:

### Fetching the code over git
This will download the code of wcc from the internet to a directory named wcc in the current working directory:

    git clone https://github.com/endrazine/wcc.git

You can then enter this directory with:

    cd wcc

### Initializing git submodules
From your root wcc directory, type:

    git submodule init
    git submodule update

#### Building WCC
From your root wcc directory, type:

    make
#### Installing WCC
Then to install wcc, type:

    sudo make install

#### Building the WCC documentation (Optional)
WCC makes use of doxygen to generate its documentation. From the root wcc directory, type

    make documentation


## Core commands
The following commands constitute the core of the Witchcraft Compiler Collection.

### wld : The Witchcraft Linker.
wld takes an ELF executable as an input and modifies it to create a shared library.
#### wld command line options
	jonathan@blackbox:~$ wld
	Witchcraft Compiler Collection (WCC) version:0.0.6    (18:10:51 May 10 2024)

	Usage: wld -libify [-noinit] file

	Options:
	    -libify          Transform executable into shared library.
	    -noinit          Ignore constructors and desctructors in output library.
	jonathan@blackbox:~$ 
#### Example usage of wld
The following example libifies the executable /bin/ls into a shared library named /tmp/ls.so.

	jonathan@blackbox:~$ cp /bin/ls /tmp/ls.so
	jonathan@blackbox:~$ wld -libify /tmp/ls.so
	jonathan@blackbox:~$ 

#### Limits of wld
wld currently only works on ELF binaries. However wld can process ELF executables irrelevant of their architecture or operating system. wld could for instance process Intel, ARM or SPARC executables from Android, Linux, BSD or UNIX operating systems and transform them into "non relocatable shared libraries". Feel free to refer to the documentation under the /doc directory for more ample details.

### wcc : The Witchcraft Compiler.
The wcc compiler takes binaries (ELF, PE, ...) as an input and creates valid ELF binaries as an output. It can be used to create relocatable object files from executables or shared libraries. 

#### wcc command line options
	jonathan@blackbox:~$ wcc
	Witchcraft Compiler Collection (WCC) version:0.0.6    (18:10:50 May 10 2024)

	Usage: wcc [options] file

	options:

	    -o, --output           <output file>
	    -m, --march            <architecture>
	    -e, --entrypoint       <0xaddress>
	    -i, --interpreter      <interpreter>
	    -p, --poison           <poison>
	    -s, --shared
	    -c, --compile
	    -S, --static
	    -x, --strip
	    -X, --sstrip
	    -E, --exec
	    -C, --core
	    -O, --original
	    -D, --disasm
	    -d, --debug
	    -h, --help
	    -v, --verbose
	    -V, --version


	jonathan@blackbox:~$ 

#### Example usage of wcc
The primary use of wcc is to "unlink" (undo the work of a linker) ELF binaries, either executables or shared libraries, back into relocatable shared objects.
The following command line attempts to unlink the binary /bin/ls (from GNU binutils) into a relocatable file named /tmp/ls.o

    jonathan@blackbox:~$ wcc -c /bin/ls -o /tmp/ls.o
    jonathan@blackbox:~$ 

This relocatable file can then be used as if it had been directly produced by a compiler. The following command would use the gcc compiler to link /tmp/ls.o into a shared library /tmp/ls.so

	jonathan@blackbox:~$ gcc /tmp/ls.o -o /tmp/ls.so -shared
	jonathan@blackbox:~$ 

#### Limits of wcc
wcc will process any file supported by libbfd and produce ELF files that will contain the same mapping when relinked and executed. This includes PE or OSX COFF files in 32 or 64 bits. However, rebuilding relocations is currently supported only for Intel ELF x86_64 binaries. Transforming a PE into an ELF and invoking pure functions is for instance supported.

### wsh : The Witchcraft shell
The witchcraft shell accepts ELF shared libraries, ELF ET_DYN executables and Witchcraft Shell Scripts written in Punk-C as an input. It loads all the executables in its own address space and makes their API available for programming in its embedded interpreter. This provides for binaries functionalities similar to those provided via reflection on languages like Java.

#### wsh command line options

	jonathan@blackbox:~$ wsh -h
	Usage: wsh [script] [-h|-q|-v|-V|-g] [binary1] [binary2] ... [-x [script_arg1] [script_arg2] ...]

	Options:

	    -x, --args                Optional script argument separator
	    -q, --quiet               Display less output
	    -v, --verbose             Display more output
	    -g, --global              Bind symbols globally
	    -V, --version             Display version and build, then exit

	Script:

	    If the first argument is an existing file which is not a known binary file format,
	    it is assumed to be a lua script and gets executed.

	Binaries:

	    Any binary file name before the -x tag gets loaded before running the script.
	    The last binary loaded is the main binary analyzed.

	jonathan@blackbox:~$ 


#### Example usage of wsh
The following command loads the /usr/sbin/apache2 executable within wsh, calls the ap_get_server_banner() function within
apache to retrieve its banner and displays it within the wsh interpreter.

	jonathan@blackbox:~$ wsh /usr/sbin/apache2
	> a = ap_get_server_banner()
	> print(a)
	Apache/2.4.7
	> 
	
To get help at any time from the wsh interpreter, simply type help. To get help on a particular topic, type help("topic").

The following example illustrates how to display the main wsh help from the interpreter and how to get detailed help on the grep command by calling help("grep") from the wsh interpreter.

	> help
	  [Shell commands]

		help, quit, exit, shell, exec, clear

	  [Functions]

	 + basic:
		help(), man()

	 + memory display:
		 hexdump(), hex_dump(), hex()

	 + memory maps:
		shdrs(), phdrs(), map(), procmap(), bfmap()

	 + symbols:
		symbols(), functions(), objects(), info(), search(), headers()

	 + memory search:
		grep(), grepptr()

	 + load libaries:
		loadbin(), libs(), entrypoints(), rescan()

	 + code execution:
		libcall()

	 + buffer manipulation:
		xalloc(), ralloc(), xfree(), balloc(), bset(), bget(), rdstr(), rdnum()

	 + control flow:
		 breakpoint(), bp()

	 + system settings:
		enableaslr(), disableaslr()

	 + settings:
		 verbose(), hollywood()

	 + advanced:
		ltrace()

	Try help("cmdname") for detailed usage on command cmdname.

	> help("grep")

		WSH HELP FOR FUNCTION grep


	NAME

		grep

	SYNOPSIS

		table match = grep(<pattern>, [patternlen], [dumplen], [before])

	DESCRIPTION

		Search <pattern> in all ELF sections in memory. Match [patternlen] bytes, then display [dumplen] bytes, optionally including [before] bytes before the match. Results are displayed in enhanced decimal form

	RETURN VALUES

		Returns 1 lua table containing matching memory addresses.


	> 

#### Extending wsh with Witchcraft Shell Scripts
The combination of a full lua interpreter in the same address space as the loaded executables and shared libraries in combination with the reflection like capabilities of wsh allow calling any function loaded in the address space from the wsh interpreter transparently. The resulting API, a powerful combination of lua and C API is called Punk-C. Wsh is fully scriptable in Punk-C, and executes Punk-C on the fly via its dynamic interpreter.
Scripts in Punk C can be invoked by specifying the full path to wsh in the magic bytes of a wsh shell. 
The following command displays the content of a Witchcraft shell script:

	jonathan@blackbox:/usr/share/wcc/scripts$ cat md5.wsh
	#!/usr/bin/wsh

	-- Computing a MD5 sum using cryptographic functions from foreign binaries (eg: sshd/OpenSSL)

	function str2md5(input)

		out = calloc(33, 1)
		ctx = calloc(1024, 1)

		MD5_Init(ctx)
		MD5_Update(ctx, input, strlen(input))
		MD5_Final(out, ctx)

		free(ctx)
		return out
	end

	input = "Message needing hashing\n"
	hash = str2md5(input)
	hexdump(hash,16)

	exit(0)
	jonathan@blackbox:/usr/share/wcc/scripts$ 


To run this script using the API made available inside the address space of sshd, simply run:

	jonathan@blackbox:/usr/share/wcc/scripts$ ./md5.wsh /usr/sbin/sshd 
	0x43e8b280    d6 fc 46 91 b0 6f ab 75 4d 9c a7 58 6d 9c 7e 36    V|F.0o+uM.'Xm.~6
	jonathan@blackbox:/usr/share/wcc/scripts$ 



#### Limits of wsh
wsh can only load shared libraries and ET_DYN dynamically linked ELF executables directly. This means ET_EXEC executables may need to be libified using wld before use in wsh. Binaries in other file formats might need to be turned into ELF files using wcc.

#### Note: Analysing and Executing ARM/SPARC/MIPS binaries "natively" on Intel x86_64 cpus via JIT binary translation
wsh can be cross compiled to ARM, SPARC, MIPS and other platforms and used in association with the qemu's user space emulation mode to provide JIT binary translation on the fly and analyse shared libraries and binaries from other cpus without requiring emulation of a full operating system in a virtual machine. The analyzed binaries are translated from one CPU to an other, and the analysed binaries, the wsh cross compiled analyser and the qemu binary translator share the address space of a single program. This significantly diminishes the complexity of analysing binaries across different hardware by seemingly allowing to run ARM or SPARC binaries on a linux x86_64 machine natively and transparently.

## Other commands

The following auxiliary commands are available with WCC. They are typically simple scripts built on top of WCC.

### wldd : print shared libraries compilation flags
When compiling C code, it is often required to pass extra arguments to the compiler to signify which shared libraries should be explicitly linked against the compile code. Figuring out those compilation parameters can be cumbersome. The wldd commands displays the shared libraries compilation flags given at compile time for any given ELF binary.

#### wldd command line options

	jonathan@blackbox:~$ wldd 
	Usage: /usr/bin/wldd </path/to/bin>

	  Returns libraries to be passed to gcc to relink this application.

	jonathan@blackbox:~$ 

#### Example usage of wldd
The following command displays shared libraries compilation flags as passed to gcc when compiling /bin/ls from GNU binutils:

	jonathan@blackbox:~$ wldd /bin/ls
	-lselinux -lacl -lc -lpcre -ldl -lattr 
	jonathan@blackbox:~$

### wcch : generate C headers from binaries
The wcch command takes an ELF binary path as a command line, and outputs a minimal C header file declaring all the exported global variables and functions from the input binary. This automates prototypes declaration when writing C code and linking with a binary for which C header files are not available.

#### Example usage of wcch

The following command instructs wcch to generate C headers from the apache2 executable and redirects the output from the standard output to a file named /tmp/apache2.h ready for use as a header in a C application.

	jonathan@blackbox:~$ wcch /usr/sbin/apache2 >/tmp/apache2.h
	jonathan@blackbox:~$ 

## Downloading the source code
The official codebase of the Witchcraft Compiler Collection is hosted on github at https://github.com/endrazine/wcc/ . It uses git modules, so some extra steps are needed to fetch all the code including dependencies. To download the source code of wcc, in a terminal, type:

    git clone https://github.com/endrazine/wcc.git
    cd wcc
    git submodule init
    git submodule update

This will create a directory named wcc and fetch all required source code in it.

## Greetings
The Witchcraft Compiler Collection uses the following amazing Open Source third party software:

  - Capstone, a lightweight multi-platform, multi-architecture disassembly framework http://www.capstone-engine.org/
  - Linenoise, A small self-contained alternative to readline and libedit https://github.com/antirez/linenoise
  - Openlibm, High quality system independent, portable, open source libm implementation https://github.com/JuliaMath/openlibm
  - Lua, The Programming Language Lua https://www.lua.org/
  - LuaJit, a Just-In-Time Compiler for Lua http://luajit.org/
  - Qemu, in particular its user space mode : https://qemu-project.gitlab.io/qemu/user/main.html
  - Uthash and Utlist, Hash tables and linked list implemented as C headers https://troydhanson.github.io/uthash/

## Testing

The following companion repository exsists to help test WCC: https://github.com/endrazine/wcc-tests

## Licence
The Witchcraft Compiler Collection is published under the MIT License.
Please refer to the file named [LICENSE](LICENSE) for more information.

