all::

#we get:
#jonathan@blackbox:/tmp/bsd2linux$ ls fmt.c.O0_amd64
#fmt.c.O0_amd64
#jonathan@blackbox:/tmp/bsd2linux$ ./fmt.c.O0_amd64
#bash: ./fmt.c.O0_amd64: No such file or directory
#jonathan@blackbox:/tmp/bsd2linux$ 

	echo " [*] Fixing dynamic linker"
	sudo cp /lib64/ld-linux-x86-64.so.2 /usr/libexec/ld.so

#we get:
#./fmt.c.O0_amd64: error while loading shared libraries: libc.so.62.0: cannot open shared object file: No such file or directory

	echo " [*] Fixing libc"
	sed s#"libc.so.62.0"#"libc.so.6\x00.0"#gi ./fmt.c.O0_amd64 > out
	mv out ./fmt.c.O0_amd64

	echo " [*] Changing permissions"
	chmod +x ./fmt.c.O0_amd64

#we get:
#./fmt.c.O0_amd64: symbol lookup error: ./fmt.c.O0_amd64: undefined symbol: atexit

	echo " [*] Compiling missing symbols"
	cd missing && make

	echo " [*] Running with preloaded dependencies"
	LD_PRELOAD=./missing/missing.so ./fmt.c.O0_amd64 foo bar baz ; echo ""

clean::
	sudo rm /usr/libexec/ld.so

