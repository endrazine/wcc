#!/bin/bash

	echo " [*] Fixing dynamic linker"
	sudo cp /lib64/ld-linux-x86-64.so.2 /usr/libexec/ld.so

	echo " [*] Fixing libc"
	sed s#"libc.so.62.0"#"libc.so.6\x00.0"#gi ./fmt.c.O0_amd64 > out
	mv out ./fmt.c.O0_amd64

	echo " [*] Changing permissions"
	chmod +x ./fmt.c.O0_amd64

	echo " [*] Compiling missing symbols"
	cd missing && make && cd -

	echo " [*] Running with preloaded dependencies"
	LD_PRELOAD=./missing/missing.so ./fmt.c.O0_amd64 foo bar baz ; echo ""

