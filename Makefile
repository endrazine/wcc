#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2024 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

CFLAGS := -W -Wall -Wno-discarded-qualifiers -Wno-int-conversion -Wno-unused-parameter -Wno-unused-function -Wno-unused-result -fpie -pie -fPIC -g3 -ggdb -I../../include  -I./include/sflib/ -I./include -I../../include/  -Wno-incompatible-pointer-types  -fstack-protector-all -Wl,-z,relro,-z,now -DPACKAGE -DPACKAGE_VERSION -masm=intel -rdynamic -D_FORTIFY_SOURCE=2 -O2

all:
	mkdir -p bin
	cd src && make CFLAGS=" $(CFLAGS)"

documentation:
	cd src && doxygen ./tex/project.cfg
	cd doc/latex && make && cp refman.pdf ../WCC_internal_documentation.pdf
clean:
	cd src && make clean
	rm -f ./bin/*

clean-documentation:
#	rm -rf ./doc/html
	rm -rf ./doc/latex

install:
	cd src && make install

uninstall:
	cd src && make uninstall

