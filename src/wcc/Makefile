#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2025 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

# Note:
# You will need to provide your own 32b static library for libbfd if you
# want to cross compile to 32b intel from amd64. Simply installing
# the 32b library on top of a 64b system breaks ubuntu.
#
# Note that the 64bits version of wcc generates 64b ELF files,
# while the 32bits version generates 32b ELF files.
# So there is a point in having both. Cross compiling to
# other architectures (eg ARM) seems pointless : the 32b intel
# version can process 32b ARM, and the 64b version process AARCH64.
#
# Compiling natively on ARM or other architectures should work provided
# capstone and libbfd are installed on the system.
#

CFLAGS := -W -Wall -Wno-discarded-qualifiers -Wno-int-conversion -Wno-unused-parameter -Wno-unused-function -Wno-unused-result -fpie -pie -fPIC -g3 -ggdb -I../../include  -I./include/sflib/ -I./include -I../../include/  -Wno-incompatible-pointer-types  -fstack-protector-all -Wl,-z,relro,-z,now -DPACKAGE -DPACKAGE_VERSION -rdynamic  -O0 -ggdb -g3 -lbfd
#-D_FORTIFY_SOURCE=2 -O2

WCC :=  ./wcc
FILE := file

all::
	$(CC) $(CFLAGS) wcc.c -o wcc -lbfd -lelf -lcapstone
#	$(CC) $(CFLAGS) -m32 -Wl,-rpath /home/jonathan/solution-exp/unlinking/awareness/self/wcc/src/wcc/lib32/  wcc.c -o wcc32 -lelf ./lib32/libbfd-2.24-system.so ./lib32/libcapstone.so.3

	cp wcc ../../bin/  || :
#	cp wcc32 ../../bin/

test::
	$(WCC) /bin/ls -o ./ls.o -c
	$(CC) ./ls.o -o ./ls.so -shared -g3 -ggdb -O0 -lselinux -lc
	$(FILE) ls.so
clean:
	rm -f wcc a.out wcc.o core wcc32 ls.o ls.so

install:
	cp wcc $(DESTDIR)/usr/bin/wcc || :
#	cp wcc32 $(DESTDIR)/usr/bin/wcc32 || :

uninstall:
	rm $(DESTDIR)/usr/bin/wcc -f
#	rm $(DESTDIR)/usr/bin/wcc32 -f
