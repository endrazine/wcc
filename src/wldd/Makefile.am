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
AM_CFLAGS =
AM_CPPFLAGS = -I$(top_builddir)/include
AM_LDFLAGS =

bin_PROGRAMS = wldd$(EXEEXT)
EXTRA_PROGRAMS = 

wldd_SOURCES = wldd.c
wldd_LDADD = 
man_MANS = $(top_builddir)/doc/manpages/wldd.1
noinst_HEADERS =
EXTRA_DIST = $(man_MANS)

