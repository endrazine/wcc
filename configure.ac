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
AC_INIT([Witchcraft Compiler Collection (WCC)],[0.0.1],[Jonathan Brossard <endrazine@gmail.com>],[wcc])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_SRCDIR([src/wld/wld.c])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_AUX_DIR([libltdl/config])
AC_REQUIRE_AUX_FILE([install-sh])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET


dnl macros for automake
AM_INIT_AUTOMAKE([foreign 1.11 check-news dist-bzip2 std-options])
AM_MAINTAINER_MODE
AC_PROG_MAKE_SET
AC_CONFIG_MACRO_DIR([m4 libltdl/m4])

dnl Set it in C99-mode
AC_SYS_INTERPRETER
AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC_C99

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


LT_INIT
LTDL_INIT
LT_CONFIG_LTDL_DIR([libltdl])

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


AC_ARG_WITH(libelf,
            [AS_HELP_STRING([--with-libelf],[LIBELF directory (must contain ./lib and ./include)])],
            [LIBELF_INC_DIR="${withval}/include";
            LIBELF_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(libelf-incdir,
  	   [AS_HELP_STRING([--with-libelf-incdir],[LIBELF include directory])],
	   [LIBELF_INC_DIR="${withval}";],)
AC_ARG_WITH(libelf-libdir,
           [AS_HELP_STRING([--with-libelf-libdir],[LIBELF library directory])],
           [LIBELF_LIB_DIR="${withval}";],)

if test "x$LIBELF_INC_DIR" != "x" ; then
   CPPFLAGS="-I${LIBELF_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([libelf.h gelf.h])

if test "x$LIBELF_LIB_DIR" != "x" ; then
   LDFLAGS="-L${LIBELF_LIB_DIR} ${LDFLAGS}"
fi

dnl The dlopen() function is in the C library for *BSD and in
dnl libdl on GLIBC-based systems
AC_CHECK_LIB([elf], [elf_begin], [], [
  AC_MSG_ERROR([Unable to find the elf_begin() function!])
])

AC_ARG_WITH(capstone,
            [AS_HELP_STRING([--with-capstone],[CAPSTONE directory (must contain ./lib and ./include)])],
            [CAPSTONE_INC_DIR="${withval}/include";
            CAPSTONE_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(capstone-incdir,
  	   [AS_HELP_STRING([--with-capstone-incdir],[CAPSTONE include directory])],
	   [CAPSTONE_INC_DIR="${withval}";],)
AC_ARG_WITH(capstone-libdir,
           [AS_HELP_STRING([--with-capstone-libdir],[CAPSTONE library directory])],
           [CAPSTONE_LIB_DIR="${withval}";],)

if test "x$CAPSTONE_INC_DIR" != "x" ; then
   CPPFLAGS="-I${CAPSTONE_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([capstone.h])

if test "x$CAPSTONE_LIB_DIR" != "x" ; then
   LDFLAGS="-L${CAPSTONE_LIB_DIR} ${LDFLAGS}"
fi

dnl The dlopen() function is in the C library for *BSD and in
dnl libdl on GLIBC-based systems
AC_CHECK_LIB([capstone], [cs_version], [], [
  AC_MSG_ERROR([Unable to find the cs_version() function!])
])

AC_ARG_WITH(libbfd,
            [AS_HELP_STRING([--with-libbfd],[LIBBFD directory (must contain ./lib and ./include)])],
            [LIBBFD_INC_DIR="${withval}/include";
            LIBBFD_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(libbfd-incdir,
  	   [AS_HELP_STRING([--with-libbfd-incdir],[LIBBFD include directory])],
	   [LIBBFD_INC_DIR="${withval}";],)
AC_ARG_WITH(libbfd-libdir,
           [AS_HELP_STRING([--with-libbfd-libdir],[LIBBFD library directory])],
           [LIBBFD_LIB_DIR="${withval}";],)

if test "x$LIBBFD_INC_DIR" != "x" ; then
   CPPFLAGS="-I${LIBBFD_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([bfd.h])

if test "x$LIBBFD_LIB_DIR" != "x" ; then
   LDFLAGS="-L$LIBBFD_LIB_DIR} ${LDFLAGS}"
fi

dnl The dlopen() function is in the C library for *BSD and in
dnl libdl on GLIBC-based systems
AC_CHECK_LIB([bfd], [bfd_init], [], [
  AC_MSG_ERROR([Unable to find the bfd_init() function!])
])


AC_ARG_WITH(libopenlibm,
            [AS_HELP_STRING([--with-libopenlibm],[LIBOPENLIBM directory (must contain ./lib and ./include)])],
            [LIBOPENLIBM_INC_DIR="${withval}/include";
            LIBOPENLIBM_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(libopenlibm-incdir,
  	   [AS_HELP_STRING([--with-libopenlibm-incdir],[LIBOPENLIBM include directory])],
	   [LIBOPENLIBM_INC_DIR="${withval}";],)
AC_ARG_WITH(libopenlibm-libdir,
           [AS_HELP_STRING([--with-libopenlibm-libdir],[LIBOPENLIBM library directory])],
           [LIBOPENLIBM_LIB_DIR="${withval}";],)

if test "x$LIBOPENLIBM_INC_DIR" != "x" ; then
   CPPFLAGS="-I${LIBOPENLIBM_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([openlibm.h])

if test "x$LIBOPENLIBM_LIB_DIR" != "x" ; then
   LDFLAGS="-L$LIBOPENLIBM_LIB_DIR} ${LDFLAGS}"
fi

dnl The dlopen() function is in the C library for *BSD and in
dnl libdl on GLIBC-based systems
AC_CHECK_LIB([openlibm], [isopenlibm], [], [
  AC_MSG_ERROR([Unable to find the isopenlibm() function!])
])

AC_ARG_WITH(libreadline,
            [AS_HELP_STRING([--with-libreadline],[LIBREADLINE directory (must contain ./lib and ./include)])],
            [LIBREADLINE_INC_DIR="${withval}/include";
            LIBREADLINE_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(libreadline-incdir,
  	   [AS_HELP_STRING([--with-libreadline-incdir],[LIBREADLINE include directory])],
	   [LIBREADLINE_INC_DIR="${withval}";],)
AC_ARG_WITH(libopenlibm-libdir,
           [AS_HELP_STRING([--with-libreadline-libdir],[LIBREADLINE library directory])],
           [LIBREADLINE_LIB_DIR="${withval}";],)

if test "x$LIBREADLINE_INC_DIR" != "x" ; then
   CPPFLAGS="-I${LIBREADLINE_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([readline.h])

if test "x$LIBREADLINE_LIB_DIR" != "x" ; then
   LDFLAGS="-L$LIBREADLINE_LIB_DIR} ${LDFLAGS}"
fi

dnl The dlopen() function is in the C library for *BSD and in
dnl libdl on GLIBC-based systems
AC_CHECK_LIB([readline], [readline], [], [
  AC_MSG_ERROR([Unable to find the readline() function!])
])

AC_ARG_WITH(lua51,
            [AS_HELP_STRING([--with-lua51],[LUA 5.1 directory (must contain ./lib and ./include)])],
            [LUA51_INC_DIR="${withval}/include";
            LUA51_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(lua51-incdir,
  	   [AS_HELP_STRING([--with-lua51-incdir],[LUA 5.1 include directory])],
	   [LUA51_INC_DIR="${withval}";],)
AC_ARG_WITH(lua51-libdir,
           [AS_HELP_STRING([--with-lua51-libdir],[LUA 5.1 library directory])],
           [LUA51_LIB_DIR="${withval}";],)

if test "x$LUA51_INC_DIR" != "x" ; then
   CPPFLAGS="-I${LUA51_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([lua.h luaconf.h lauxlib.h lualib.h])

if test "x$LUA51_LIB_DIR" != "x" ; then
   LDFLAGS="-L$LUA51_LIB_DIR} ${LDFLAGS}"
fi

dnl The dlopen() function is in the C library for *BSD and in
dnl libdl on GLIBC-based systems
AC_CHECK_LIB([lua5.1], [lua_settable], [], [
  AC_MSG_ERROR([Unable to find the lua_settable() function!])
])

AC_CHECK_TOOL([LOCAL_LDD], [ldd])

dnl generate output
AC_CONFIG_FILES([Makefile src/wld/Makefile src/wcc/Makefile src/wsh/Makefile src/tools/Makefile])
AC_CONFIG_FILES([src/tools/wldd], [chmod +x src/tools/wldd])
AC_CONFIG_FILES([src/tools/wcch], [chmod +x src/tools/wcch])

AC_OUTPUT
