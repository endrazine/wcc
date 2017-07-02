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
AM_CPPFLAGS = -I$(top_builddir)/include -Iinclude -I$(top_builddir)/src/wsh/include
AM_CFLAGS = -DLUA_USE_LINUX=1

if TARGET_IS_LINUX
if TARGET_IS_SUPPORTED
lib_LTLIBRARIES = liblua.la
liblua_la_LIBADD = -lopenlibm
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
