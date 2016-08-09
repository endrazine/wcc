CC := gcc

CFLAGS := -masm=intel -rdynamic -W -Wall -Wextra -O0 -mpreferred-stack-boundary=12 -mstackrealign -ggdb -g3 -Wno-unused-but-set-variable -Wno-unused-parameter -I./include/sflib/ -I./include -I../../include/ -rdynamic -Wl,-E -Wl,-z,now #--sanitize=address
OBJLIB := ./lua/src/liblua.a ./openlibm/libopenlibm.a


#CFLAGS := -rdynamic -W -Wall -Wextra -O0 -mpreferred-stack-boundary=12 -mstackrealign -ggdb -g3 -Wno-unused-but-set-variable -Wno-unused-parameter -I./include -rdynamic -I../../include/ -I./luajit-2.0/src/ -Wl,-E -Wl,-z,now
#OBJLIB := ./luajit-2.0/src/libluajit.a ./openlibm/libopenlibm.a 

all::
	cd openlibm && make CFLAGS="-pie -fpie -fPIC"
	cd lua && make linux CFLAGS="-pie -fpie -fPIC"
	$(CC) $(CFLAGS) wsh.c -o wsh.o -c -ldl -lreadline -pie -fpie -fPIC
	$(CC) $(CFLAGS) wshmain.c -o wshmain.o -c -ldl -lreadline -pie -fpie -fPIC
	$(CC) $(CFLAGS) helper.c -o helper.o -c -pie -fpie -fPIC
	$(CC) $(CFLAGS) linenoise/linenoise.c -o linenoise.o -c -pie -fpie -fPIC
	$(CC) $(CFLAGS) wsh.o helper.o linenoise.o -shared -fPIC -o libwitch.so
	ar cr libwitch.a wsh.o helper.o linenoise.o
	$(CC) $(CFLAGS) wsh.o helper.o linenoise.o wshmain.o -o wsh -T script.lds -liberty $(OBJLIB) -ldl -lgsl -lgslcblas

	cp wsh ../../bin/

test:
	cd tests && make

clean::
	rm wsh helper.o wsh.o wshmain.o libwitch.so libwitch.a linenoise.o learnwitch.log -f
	cd openlibm && make clean
	cd lua && make clean
	cd tests && make clean

deepclean:
	cd openlibm && make clean
	cd lua && make clean
	make clean

install::
	mkdir -p /usr/share/wcc/
	cp -r ./scripts /usr/share/wcc/
	cp wsh /usr/bin/wsh

uninstall::
	rm -rf /usr/share/wcc/
	rm -f /usr/bin/wsh

binfmt:
	sudo update-binfmts --install wsh /usr/bin/wsh --extension wsh


