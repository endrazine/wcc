#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2022 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

CC := gcc 
CFLAGS := -W -Wall -Wextra -Wl,-e__main
LDFLAGS := /usr/sbin/sshd

all::
	$(CC) $(CFLAGS) $(LDFLAGS) sshd2.c -o sshd2

clean:
	rm -rf sshd2
