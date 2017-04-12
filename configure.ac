dnl/*
dnl    Copyright (C) 2013 Dovado Europe AB <markus@dovado.com>
dnl
dnl    This program is free software: you can redistribute it and/or modify
dnl    it under the terms of the GNU General Public License as
dnl    published by the Free Software Foundation, either version 3 of the
dnl    License, or (at your option) any later version.
dnl
dnl    This program is distributed in the hope that it will be useful,
dnl    but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl    GNU General Public License for more details.
dnl
dnl    You should have received a copy of the GNU General Public License
dnl    along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl */

dnl Process this file with autoconf to produce a configure script.

dnl init autoconf
AC_REQ(2.63)
AC_INIT([wcc],[0.0.1],[Jonathan Brossard <jonathan@blackbox>])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_SRCDIR([src/wld/wld.c])
AC_CONFIG_AUX_DIR([.])
AC_REQUIRE_AUX_FILE([install-sh])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET


dnl macros for automake
AM_INIT_AUTOMAKE([foreign 1.11 check-news dist-bzip2 std-options])
AM_MAINTAINER_MODE
AC_PROG_MAKE_SET
AC_CONFIG_MACRO_DIR([m4])

dnl Set it in C99-mode
AC_SYS_INTERPRETER
AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC_C99
dnl GNUlib
gl_EARLY
gl_INIT

dnl AC_PROG_CPP
AC_REQUIRE_CPP
AC_PROG_CC_C_O
AC_PROG_CXX
AC_C_FLEXIBLE_ARRAY_MEMBER
AC_C_VARARRAYS

dnl /bin/ln -s
AC_PROG_LN_S
dnl /usr/bin/make
AC_PROG_MAKE_SET
dnl /usr/bin/install
AC_PROG_INSTALL
dnl /bin/mkdir -p
AC_PROG_MKDIR_P
dnl /usr/bin/ranlib
AC_PROG_RANLIB

dnl Standard socklen_t test

dnl C checks
AC_C_CONST
AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE
AC_C_TYPEOF
AC_C_CHAR_UNSIGNED
AC_C_STRINGIZE
AC_C_BIGENDIAN
AC_FUNC_SELECT_ARGTYPES


dnl generate output
AC_CONFIG_FILES([Makefile src/wld/Makefile src/wcc/Makefile src/wsh/Makefile])
AC_OUTPUT
