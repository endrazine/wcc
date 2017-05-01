#/*
#    Copyright (C) 2013 Dovado Europe AB <markus@dovado.com>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 3 of the
#    License, or (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>. 
#*/

## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign 1.11 check-news dist-bzip2 std-options

ACLOCAL_AMFLAGS = -I$(top_builddir)/m4
AM_CPPFLAGS = -I$(top_builddir)/include -Iinclude -Ilua/src
AM_CFLAGS = $(INTELCFLAGS) -W -Wall -Wextra -O0 -ggdb -g3 -Wno-unused-but-set-variable -Wno-unused-parameter -rdynamic -Wl,-E -Wl,-z,now

if TARGET_IS_X64
AM_LDFLAGS = -T ./script.lds -lopenlibm
else
AM_LDFLAGS = $(LIBM)
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

