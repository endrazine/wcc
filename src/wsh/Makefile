#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2025 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

OBJLIB := ./lua/src/liblua.a ./openlibm/libopenlibm.a

CFLAGS := -rdynamic -W -Wall -Wextra -O0 -ggdb -g3 -Wno-unused-but-set-variable -Wno-unused-parameter -I./include -rdynamic -I../../include/ -I./luajit-2.0/src/ -Wl,-E -Wl,-z,now
#OBJLIB := ./luajit-2.0/src/libluajit.a ./openlibm/libopenlibm.a 

unamep := $(shell uname -p)

ifeq ($(unamep), x86_64)
CFLAGS += -masm=intel
endif

ifeq ($(unamep), x86_64)
WLINK	:= -Wl,-T -Wl,script.lds
else
WLINK	:=
endif

all::
	cd openlibm && make CFLAGS="-fpie -fPIC"
	cd lua && make linux CFLAGS="-fpie -fPIC"
	$(CC) $(CFLAGS) wsh.c -o wsh.o -c -fpie -fPIC -ldl -lreadline
	$(CC) $(CFLAGS) wshmain.c -o wshmain.o -c -fpie -fPIC -ldl -lreadline
	$(CC) $(CFLAGS) helper.c -o helper.o -c -fpie -fPIC
	$(CC) $(CFLAGS) linenoise/linenoise.c -o linenoise.o -c -fpie -fPIC
	$(CC) $(CFLAGS) wsh.o helper.o linenoise.o -shared -fPIC -o libwitch.so
	ar cr libwitch.a wsh.o helper.o linenoise.o
	$(CC) $(CFLAGS) elfloader64.c -o elfloader64.o -c
	$(CC) $(CFLAGS) elfloader64.o wsh.o helper.o linenoise.o wshmain.o -o wsh $(WLINK) -liberty $(OBJLIB) -ldl
	cp wsh libwsh.so
	../wld/wld -l libwsh.so
	cp wsh ../../bin/

test:
	cd tests && make

clean::
	rm wsh elfloader64.o helper.o wsh.o wshmain.o libwitch.so libwitch.a linenoise.o learnwitch.log libwsh.so -f
	cd openlibm && make clean
	cd lua && make clean
	cd tests && make clean

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


