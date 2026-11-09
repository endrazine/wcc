#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2026 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

OBJLIB := ./lua/src/liblua.a ./openlibm/libopenlibm.a ./librustdemangle/target/release/librust_demangle.a

CFLAGS := -rdynamic -W -Wall -Wextra -O0 -ggdb -g3 -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-incompatible-pointer-types -Wno-int-conversion -I./include -rdynamic -I../../include/ -I./luajit-2.0/src/ -Wl,-E -Wl,-z,now
#OBJLIB := ./luajit-2.0/src/libluajit.a ./openlibm/libopenlibm.a

unamem := $(shell uname -m)

ifeq ($(unamem), x86_64)
CFLAGS += -masm=intel
endif

ifeq ($(unamem), x86_64)
WLINK	= -Wl,-T -Wl,script.lds
else
WLINK	=
endif

# musl sysroot for header symlinks
MUSL_SYSROOT := /usr/include/x86_64-linux-musl

all::
	cd librustdemangle && cargo build --release
	cd openlibm && make CFLAGS="-fpie -fPIC"
	cd lua && make linux CFLAGS="-fpie -fPIC"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) wsh.c -o wsh.o -c -fpie -fPIC -ldl -lreadline
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) wshmain.c -o wshmain.o -c -fpie -fPIC -ldl -lreadline
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) helper.c -o helper.o -c -fpie -fPIC
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) linenoise/linenoise.c -o linenoise.o -c -fpie -fPIC
	$(CC) $(CFLAGS) wsh.o helper.o linenoise.o -shared -fPIC -o libwitch.so
	ar cr libwitch.a wsh.o helper.o linenoise.o
	$(CC) $(CFLAGS) elfloader64.c -o elfloader64.o -c
	$(CC) $(CFLAGS) disasm.c -o disasm.o -c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) ../wld/wld.o disasm.o elfloader64.o wsh.o helper.o linenoise.o wshmain.o -o wsh $(WLINK) -liberty $(OBJLIB) -ldl -lcapstone
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) ../wld/wld.o disasm.o elfloader64.o wsh.o helper.o linenoise.o wshmain.o -o wsh-static-`uname -m` $(WLINK) -liberty $(OBJLIB) -static  -lcapstone
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) ../wld/wld.o disasm.o elfloader64.o wsh.o helper.o linenoise.o wshmain.o -o wsh-`uname -m` $(WLINK) -liberty -lcapstone $(OBJLIB) -ldl -Wl,-rpath=/tmp/wsh/`uname -m`/,-rpath=/tmp/wsh/,-rpath=.
	cp wsh libwsh.so
	../wld/wld -l libwsh.so
	cp wsh ../../bin/

# ---------------------------------------------------------------
# static-musl: fully static binary linked against musl libc
#
# Requires: musl-tools (apt install musl-tools)
# Run 'make musl-sysroot' once to set up header symlinks.
# ---------------------------------------------------------------

MUSL_OBJLIB := ./lua/src/liblua.a ./openlibm/libopenlibm.a ./librustdemangle/target/release/librust_demangle.a

musl-sysroot::
	@echo "=== Creating musl sysroot symlinks (requires sudo) ==="
	@# Kernel UAPI headers (libc-agnostic)
	sudo ln -sfn /usr/include/linux $(MUSL_SYSROOT)/linux
	sudo ln -sfn /usr/include/x86_64-linux-gnu/asm $(MUSL_SYSROOT)/asm
	sudo ln -sfn /usr/include/asm-generic $(MUSL_SYSROOT)/asm-generic
	@# Library headers (standalone, no glibc dependency)
	sudo ln -sfn /usr/include/libelf.h $(MUSL_SYSROOT)/libelf.h
	sudo ln -sfn /usr/include/gelf.h $(MUSL_SYSROOT)/gelf.h
	sudo ln -sfn /usr/include/elfutils $(MUSL_SYSROOT)/elfutils
	sudo ln -sfn /usr/include/capstone $(MUSL_SYSROOT)/capstone
	@echo "=== musl sysroot ready ==="

static-musl:: musl-sysroot
	@# --- dependencies ---
	cd librustdemangle && cargo build --release
	cd openlibm && make CFLAGS="-fpie -fPIC"
	cd lua && make clean && make -C src all CC=musl-gcc MYCFLAGS="-fpie -fPIC" MYLIBS=""
	@# --- compile all .o with musl-gcc ---
	musl-gcc $(CFLAGS) wsh.c -o wsh.o -c -fpie -fPIC
	musl-gcc $(CFLAGS) wshmain.c -o wshmain.o -c -fpie -fPIC
	musl-gcc $(CFLAGS) helper.c -o helper.o -c -fpie -fPIC
	musl-gcc $(CFLAGS) linenoise/linenoise.c -o linenoise.o -c -fpie -fPIC
	musl-gcc $(CFLAGS) elfloader64.c -o elfloader64.o -c
	musl-gcc $(CFLAGS) disasm.c -o disasm.o -c
	musl-gcc $(CFLAGS) ../wld/wld.c -o wld_musl.o -c -fpie -fPIC
	musl-gcc musl_shims.c -o musl_shims.o -c
	@# --- link ---
	musl-gcc -no-pie $(CFLAGS) \
		wld_musl.o disasm.o elfloader64.o wsh.o helper.o linenoise.o wshmain.o musl_shims.o \
		-o wsh-static-musl-`uname -m` \
		$(WLINK) \
		/usr/lib/x86_64-linux-gnu/libiberty.a \
		$(MUSL_OBJLIB) \
		-static \
		/usr/lib/x86_64-linux-gnu/libcapstone.a
	@echo "=== wsh-static-musl-`uname -m` built ==="
	@file wsh-static-musl-`uname -m`

test:
	cd tests && make

clean::
	rm wsh elfloader64.o helper.o wsh.o wshmain.o libwitch.so libwitch.a linenoise.o learnwitch.log libwsh.so wsh-* disasm.o wld_musl.o musl_shims.o -f
	cd openlibm && make clean
	cd lua && make clean
	cd tests && make clean
	cd librustdemangle && rm -rf target && rm -f Cargo.lock
deepclean:
	cd openlibm && make clean
	cd lua && make clean
	make clean

install::
	mkdir -p $(DESTDIR)/usr/share/wcc/
	cp -r ./scripts $(DESTDIR)/usr/share/wcc/
	cp wsh $(DESTDIR)/usr/bin/wsh || :
	cp libwsh.so $(DESTDIR)/lib/x86_64-linux-gnu/  || :
uninstall::
	rm -rf $(DESTDIR)/usr/share/wcc/
	rm -f $(DESTDIR)/usr/bin/wsh
	rm -f $(DESTDIR)/lib/x86_64-linux-gnu/libwsh.so

binfmt:
	sudo update-binfmts --install wsh /usr/bin/wsh --extension wsh

