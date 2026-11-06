#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2026 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

CC      := gcc
CFLAGS  := -W -Wall -Wextra -O0 -g -fPIC

all:: libglobals_target.so libstatic.so

libglobals_target.so: globals_target.c
	$(CC) $(CFLAGS) -shared -o $@ $<

# Built UNstripped on purpose: its static globals live only in .symtab.
libstatic.so: globals_static.c
	$(CC) $(CFLAGS) -shared -o $@ $<

test: all
	sh run_test.sh

clean:
	rm -f libglobals_target.so libstatic.so
