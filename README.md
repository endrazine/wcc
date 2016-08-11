

#     The Witchcraft Compiler Collection

Welcome to the Witchcraft Compiler Collection !

## Purpose

WCC is a collection of compilation tools to perform witchcraft on the GNU/Linux and other POSIX plateforms.


## Commands

### wld : The Witchcraft Linker.
wld takes an ELF executable as an input and modifies it to create a shared library.
#### wld command line options
	jonathan@blackbox:~$ wld
	Witchcraft Compiler Collection (WCC) version:0.0.1    (23:11:13 Jul 21 2016)

	Usage: wld [options] file

	options:

	    -libify          Set Class to ET_DYN in input ELF file.

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
	Witchcraft Compiler Collection (WCC) version:0.0.1    (01:47:53 Jul 29 2016)

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
The witchcraft shell accepts ELF shared libraries, ELF ET_DYN executables and Witchcraft Shell Scripts written in Punk-C as an input. It loads all the executables in its own address space and make their API available for programming in its embedded interpreter. This provides for binaries functionalities similar to those provided via reflection on languages like Java.

#### wsh command line options

	jonathan@blackbox:~$ wsh -h
	Usage: wsh [script] [options] [binary1] [binary2] ... [-x] [script_arg1] [script_arg2] ...

	Options:

	    -x, --args                Optional script argument separator.
	    -v, --verbose
	    -V, --version

	Script:

	    If the first argument is an existing file which is not a known binary file format,
	    it is assumed to be a lua script and gets executed.

	Binaries:

	    Any binary file name before the -x tag gets loaded before running the script.
	    The last binary loaded is the main binary analyzed.

	jonathan@blackbox:~$ 

#### Example usage of wsh
The following command loads the /usr/sbin/apache2 executable within wsh, calls the ap_get_server_banner() function within
apache to retreive its banner and displays it within the wsh intterpreter.

	jonathan@blackbox:~$ wsh /usr/sbin/apache2
	> a = ap_get_server_banner()
	> print(a)
	Apache/2.4.7
	> 
#### Limits of wsh
wsh can only load sharde libraries and ET_DYN dynamically linked ELF executables directly. This means ET_EXEC executables may need to be libified using wld before use in wsh.

## Downloading the source code
The official codebase of the Witchcraft Compiler Collection is hosted on github at https://github.com/endrazine/wcc/ . It uses git modules, so some extra steps are needed to fetch all the code including depedencies. To download the source code of wcc, in a terminal, type:

    git clone https://github.com/endrazine/wcc.git
    cd wcc
    git submodule init
    git submodule update

This will create a directory named wcc and fetch all required source code in it.

## Pre-requisits

### Installing requirements
The Witchcraft Compiler Collection requires the following software to be installed:

        Glibc, libbfd, libdl, zlib, libelf, libreadline, libgsl.

### Installing requirements on Ubuntu/Debian
Under ubuntu/debian those dependancies can be installed with the following command:

    sudo apt-get install clang libbfd-dev uthash-dev libelf-dev libcapstone-dev libreadline6 libreadline6-dev libiberty-dev libgsl-dev

## Building and Installing:
#### Building WCC
From your root wcc directory, type:

    make
#### Installing WCC
Then to install wcc, type:

    sudo make install

#### Building the WCC documentation
WCC makes use of doxygen to generate its documentation. From the root wcc directory, type

    make documentation

## Licence
The Witchcraft Compiler Collection is published under the MIT License.
Please refer to the file named LICENSE for more information.

