#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2022 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

CC := gcc 
CFLAGS := -W -Wall -Wextra -Wno-int-conversion -Wno-incompatible-pointer-types -std=gnu17 -Wl,-e__main 

all::
	wunstrip -o ./sshd.tmp /usr/sbin/sshd -f -d
	wunstrip -o ./sshd ./sshd.tmp -r
	wcc -c -o sshd.o sshd
	$(CC) $(CFLAGS) sshd.o sshd2.c -o sshd2

clean:
	rm -rf sshd2 sshd.o sshd sshd.tmp
