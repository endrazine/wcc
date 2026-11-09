#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2026 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

CC := gcc
CFLAGS := -W -Wall -Wextra -Wno-int-conversion -std=gnu17
LDFLAGS := -lc -lcrypto -lssl -Wl,-rpath=`pwd`

all::
	wcc -c -o sshd.o /usr/sbin/sshd
	$(CC) $(CFLAGS) sshd.o ssh.c -o ssh $(LDFLAGS)

test::
	./ssh
	
clean:
	rm -rf ssh sshd.o
