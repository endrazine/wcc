#/*
#    Copyright (C) 2017 Markus Gothe <nietzsche@lysator.liu.se>
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
AM_CPPFLAGS = -I$(top_builddir)/include -Iinclude -I$(top_builddir)/src/wsh/include
AM_CFLAGS = -DLUA_USE_LINUX=1

if TARGET_IS_LINUX
if TARGET_IS_SUPPORTED
lib_LTLIBRARIES = liblua.la
liblua_la_LIBADD = $(LIBM)
liblua_la_SOURCES = \
	lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c \
	lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c  \
	lundump.c lvm.c lzio.c lcorolib.c \
	lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c \
	lstrlib.c loadlib.c linit.c lutf8lib.c \
	lapi.h ldebug.h lgc.h lmem.h lparser.h ltable.h lzio.h ldo.h llex.h \
	lobject.h lstate.h ltm.h lundump.h lcode.h lfunc.h llimits.h lopcodes.h \
	lstring.h lvm.h

endif
endif
