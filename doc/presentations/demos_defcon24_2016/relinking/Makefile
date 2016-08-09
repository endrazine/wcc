CC := gcc-4.8
WCC = wcc

CFLAGS :=  -W -Wall -Wextra

CF :=  -ggdb -g3 -rdynamic  #-shared

all::
	$(CC) $(CFLAGS) small.c -o small
	$(WCC) small -o small_wcc.o -c
	$(CC) small_wcc.o -o small2

