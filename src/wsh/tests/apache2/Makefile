#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2026 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

CC	:=	gcc
CFLAGS	:=	-W -Wall
LDFLAGS	:=	-lc ./apache2.so -Wl,-rpath=`pwd`

all::
	cp /usr/sbin/apache2 ./apache2.so
	wld -ls ./apache2.so
	$(CC)  $(CFLAGS) ap2v.c -o ap2v $(LDFLAGS)

test::
	./ap2v

clean::
	rm ap2v apache2.so -f


