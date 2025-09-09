#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2025 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

OBJLIB := ./lua/src/liblua.a ./openlibm/libopenlibm.a ./librustdemangle/target/release/librust_demangle.a

CFLAGS := -rdynamic -W -Wall -Wextra -O0 -ggdb -g3 -Wno-unused-but-set-variable -Wno-unused-parameter -I./include -rdynamic -I../../include/ -I./luajit-2.0/src/ -Wl,-E -Wl,-z,now
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

# Detect Alpine Linux
ifneq (,$(wildcard /etc/alpine-release))
    LIBS += -lunwind
endif

all::
	cd librustdemangle && cargo build --release
	cd openlibm && make CFLAGS="-fpie -fPIC"
	cd lua && make linux CFLAGS="-fpie -fPIC"
	$(CC) $(CFLAGS) wsh.c -o wsh.o -c -fpie -fPIC -ldl -lreadline
	$(CC) $(CFLAGS) wshmain.c -o wshmain.o -c -fpie -fPIC -ldl -lreadline
	$(CC) $(CFLAGS) helper.c -o helper.o -c -fpie -fPIC
	$(CC) $(CFLAGS) linenoise/linenoise.c -o linenoise.o -c -fpie -fPIC
	$(CC) $(CFLAGS) wsh.o helper.o linenoise.o -shared -fPIC -o libwitch.so
	ar cr libwitch.a wsh.o helper.o linenoise.o
	$(CC) $(CFLAGS) elfloader64.c -o elfloader64.o -c
	$(CC) $(CFLAGS) ../wld/wld.o elfloader64.o wsh.o helper.o linenoise.o wshmain.o -o wsh $(LIB) $(WLINK) -liberty $(OBJLIB) -ldl
	$(CC) $(CFLAGS) ../wld/wld.o elfloader64.o wsh.o helper.o linenoise.o wshmain.o -o wsh-static-`uname -m` $(LIB) $(WLINK) -liberty $(OBJLIB) -static
	$(CC) $(CFLAGS) ../wld/wld.o elfloader64.o wsh.o helper.o linenoise.o wshmain.o -o wsh-`uname -m` $(WLINK) $(LIB) -liberty $(OBJLIB) -ldl -Wl,-rpath=/tmp/wsh/`uname -m`/,-rpath=/tmp/wsh/,-rpath=.
	cp wsh libwsh.so
	../wld/wld -l libwsh.so
	cp wsh ../../bin/

test:
	cd tests && make

clean::
	rm wsh elfloader64.o helper.o wsh.o wshmain.o libwitch.so libwitch.a linenoise.o learnwitch.log libwsh.so wsh-* -f
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


