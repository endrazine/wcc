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
AM_CFLAGS = -Xlinker --defsym -Xlinker WDATE=$$(date +'%Y%m%d') -Xlinker --defsym -Xlinker WTIME=$$(date +'%H%M') -masm=intel -W -Wall -Wextra -O0 -mpreferred-stack-boundary=12 -mstackrealign -ggdb -g3 -Wno-unused-but-set-variable -Wno-unused-parameter -rdynamic -Wl,-E -Wl,-z,now  #--sanitize=address
AM_CPPFLAGS = -I$(top_builddir)/include -Iinclude
AM_LDFLAGS =

bin_PROGRAMS = wsh$(EXEEXT)
EXTRA_PROGRAMS = 

wsh_SOURCES = wsh.c wshmain.c helper.c linenoise.c
wsh_LDADD = 
man_MANS =
noinst_HEADERS = 
EXTRA_DIST = $(man_MANS)

