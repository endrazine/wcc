#/*
#*******************************************************************************
#* The MIT License (MIT)
#* Copyright (c) 2017 Markus Gothe
#*
#* Permission is hereby granted, free of charge, to any person obtaining a copy
#* of this software and associated documentation files (the "Software"), to deal
#* in the Software without restriction, including without limitation the rights
#* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#* copies of the Software, and to permit persons to whom the Software is
#* furnished to do so, subject to the following conditions:
#*
#* The above copyright notice and this permission notice shall be included in
#* all copies or substantial portions of the Software.
#*
#* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#* SOFTWARE.
#*******************************************************************************
#*/

## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign 1.11 check-news dist-bzip2 std-options

ACLOCAL_AMFLAGS = -I$(top_builddir)/m4
AM_CPPFLAGS = -I$(top_builddir)/include -Iinclude -Ilua/src
AM_CFLAGS = $(INTELCFLAGS) $(SANITIZECFLAGS) $(PTHREAD_CFLAGS) -W -Wall -Wextra -O0 -ggdb -g3 -Wno-unused-but-set-variable -Wno-unused-parameter -rdynamic -Wl,-E -Wl,-z,now
AM_LDFLAGS = -lopenlibm $(PTHREAD_LIBS)

if TARGET_IS_X64
AM_LDFLAGS += -T ./script.lds
endif

if TARGET_IS_LINUX
if TARGET_IS_SUPPORTED
SUBDIRS = lua/src
bin_PROGRAMS = wsh$(EXEEXT)
wshdir = @DATADIR@
wsh_DATA = scripts/print_G scripts/INDEX scripts/annotate scripts/breakpoints scripts/callmain.wsh scripts/debug scripts/dumpbin.wsh scripts/dumptable scripts/hashing scripts/headers scripts/hexdump scripts/internals.lua scripts/libmain scripts/main.wsh scripts/map scripts/memory scripts/scope scripts/search scripts/searchobj scripts/store scripts/read.wsh scripts/md5.wsh
wsh_SOURCES = wsh.c wshmain.c helper.c linenoise.c
wsh_LDADD = lua/src/liblua.la
endif
endif
EXTRA_PROGRAMS = 
 
man_MANS = $(top_builddir)/doc/manpages/wsh.1
noinst_HEADERS = include/linenoise.h include/colors.h include/lua.h include/lualib.h include/luaconf.h

EXTRA_DIST = $(man_MANS)

