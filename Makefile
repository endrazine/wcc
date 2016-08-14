#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

all:
	mkdir -p bin
	cd src && make

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

