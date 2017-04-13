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
AC_INIT([Witchcraft Compiler Collection (WCC)],[0.0.1],[Jonathan Brossard endrazine@gmail.com],[wcc],[https://github.com/endrazine/wcc])
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

LT_INIT([dlopen])
LT_PATH_LD

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

AC_CANONICAL_TARGET
AS_CASE([$target_cpu],
       [x86_64], [IS_X64="true"],
       [IS_X64="false"]
)
AS_CASE([$target_os],
       [linux*], [IS_LINUX="true"],
       [IS_LINUX="false"]
)
AM_CONDITIONAL(TARGET_IS_X64, [test "$IS_X64" = "true"])
AM_CONDITIONAL(TARGET_IS_LINUX, [test "$IS_LINUX" = "true"])


AC_CHECK_HEADERS([sys/procfs.h sys/prctl.h link.h stropts.h malloc.h])

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
AC_CHECK_HEADERS([libelf.h gelf.h], [], [
  AC_MSG_ERROR([Unable to find the libelf headers!])
])

if test "x$LIBELF_LIB_DIR" != "x" ; then
   LDFLAGS="-L${LIBELF_LIB_DIR} ${LDFLAGS}"
fi

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
AC_CHECK_HEADERS([capstone.h], [], [
  AC_MSG_ERROR([Unable to find the capstone.h header!])
])

if test "x$CAPSTONE_LIB_DIR" != "x" ; then
   LDFLAGS="-L${CAPSTONE_LIB_DIR} ${LDFLAGS}"
fi

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
AC_CHECK_HEADERS([bfd.h], [], [
  AC_MSG_ERROR([Unable to find the bfd.h header!])
])

if test "x$LIBBFD_LIB_DIR" != "x" ; then
   LDFLAGS="-L${LIBBFD_LIB_DIR} ${LDFLAGS}"
fi

AC_CHECK_LIB([bfd], [bfd_init], [], [
  AC_MSG_ERROR([Unable to find the bfd_init() function!])
])

AC_ARG_WITH(libiberty,
            [AS_HELP_STRING([--with-libiberty],[LIBIBERTY directory (must contain ./lib and ./include)])],
            [LIBIBERTY_INC_DIR="${withval}/include";
            LIBIBERTY_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(libiberty-incdir,
  	   [AS_HELP_STRING([--with-libiberty-incdir],[LIBIBERTY include directory])],
	   [LIBIBERTY_INC_DIR="${withval}";],)
AC_ARG_WITH(libiberty-libdir,
           [AS_HELP_STRING([--with-libiberty-libdir],[LIBIBERTY library directory])],
           [LIBIBERTY_LIB_DIR="${withval}";],)

if test "x$LIBIBERTY_INC_DIR" != "x" ; then
   CPPFLAGS="-I${LIBIBERTY_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([libiberty.h], [], [
  AC_MSG_ERROR([Unable to find the libiberty.h header!])
])

if test "x$LIBIBERTY_LIB_DIR" != "x" ; then
   LDFLAGS="-L${LIBIBERTY_LIB_DIR} ${LDFLAGS}"
fi

AC_CHECK_LIB([iberty], [get_run_time], [], [
  AC_MSG_ERROR([Unable to find the get_run_time() function!])
])

AC_ARG_WITH(openlibm,
            [AS_HELP_STRING([--with-openlibm],[OPENLIBM directory (must contain ./lib and ./include)])],
            [OPENLIBM_INC_DIR="${withval}/include";
            OPENLIBM_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(openlibm-incdir,
  	   [AS_HELP_STRING([--with-openlibm-incdir],[OPENLIBM include directory])],
	   [OPENLIBM_INC_DIR="${withval}";],)
AC_ARG_WITH(openlibm-libdir,
           [AS_HELP_STRING([--with-openlibm-libdir],[OPENLIBM library directory])],
           [OPENLIBM_LIB_DIR="${withval}";],)

if test "x$OPENLIBM_INC_DIR" != "x" ; then
   CPPFLAGS="-I${OPENLIBM_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([openlibm.h], [], [
  AC_MSG_ERROR([Unable to find the openlibm.h header!])
])

if test "x$OPENLIBM_LIB_DIR" != "x" ; then
   LDFLAGS="-L${OPENLIBM_LIB_DIR} ${LDFLAGS}"
fi

AC_CHECK_LIB([openlibm], [isopenlibm], [], [
  AC_MSG_ERROR([Unable to find the isopenlibm() function!])
])

AC_ARG_WITH(readline,
            [AS_HELP_STRING([--with-readline],[LIBREADLINE directory (must contain ./lib and ./include)])],
            [LIBREADLINE_INC_DIR="${withval}/include";
            LIBREADLINE_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(readline-incdir,
  	   [AS_HELP_STRING([--with-readline-incdir],[LIBREADLINE include directory])],
	   [LIBREADLINE_INC_DIR="${withval}";],)
AC_ARG_WITH(readline-libdir,
           [AS_HELP_STRING([--with-readline-libdir],[LIBREADLINE library directory])],
           [LIBREADLINE_LIB_DIR="${withval}";],)

if test "x$LIBREADLINE_INC_DIR" != "x" ; then
   CPPFLAGS="-I${LIBREADLINE_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([readline.h], [], [
  AC_MSG_ERROR([Unable to find the readline.h header!])
])

if test "x$LIBREADLINE_LIB_DIR" != "x" ; then
   LDFLAGS="-L${LIBREADLINE_LIB_DIR} ${LDFLAGS}"
fi

AC_CHECK_LIB([readline], [readline], [], [
  AC_MSG_ERROR([Unable to find the readline() function!])
])

AC_ARG_WITH(lua53,
            [AS_HELP_STRING([--with-lua53],[LUA 5.3 directory (must contain ./lib and ./include)])],
            [LUA53_INC_DIR="${withval}/include";
            LUA53_LIB_DIR="${withval}/lib";],)
AC_ARG_WITH(lua53-incdir,
  	   [AS_HELP_STRING([--with-lua53-incdir],[LUA 5.3 include directory])],
	   [LUA53_INC_DIR="${withval}";],)
AC_ARG_WITH(lua53-libdir,
           [AS_HELP_STRING([--with-lua53-libdir],[LUA 5.3 library directory])],
           [LUA53_LIB_DIR="${withval}";],)

if test "x$LUA53_INC_DIR" != "x" ; then
   CPPFLAGS="-I${LUA53_INC_DIR} ${CPPFLAGS}"
fi
AC_CHECK_HEADERS([lua.h luaconf.h lauxlib.h lualib.h])

if test "x$LUA53_LIB_DIR" != "x" ; then
   LDFLAGS="-L${LUA53_LIB_DIR} ${LDFLAGS}"
fi

AC_CHECK_LIB([lua5.3], [lua_settable])
AC_SEARCH_LIBS([dladdr1], [dl dld])

AC_PATH_TARGET_TOOL([LOCAL_LUA], [lua])
AM_CONDITIONAL(HAVE_LUA, [test -n "$LOCAL_LUA"])
AC_PATH_TARGET_TOOL([LOCAL_APACHE2], [apache2])
AM_CONDITIONAL(HAVE_APACHE2, [test -n "$LOCAL_APACHE2"])
AC_PATH_TARGET_TOOL([LOCAL_POSTGRES], [postgres])
AM_CONDITIONAL(HAVE_POSTGRES, [test -n "$LOCAL_POSTGRES"])
AC_PATH_TARGET_TOOL([LOCAL_SSHD], [sshd])
AM_CONDITIONAL(HAVE_SSHD, [test -n "$LOCAL_SSHD"])
AC_PATH_TARGET_TOOL([LOCAL_SSHD2], [sshd2])
AM_CONDITIONAL(HAVE_SSHD2, [test -n "$LOCAL_SSHD2"])
AC_PATH_TOOL([DOXYGEN], [doxygen])
AM_CONDITIONAL(HAVE_DOXYGEN, [test -n "$DOXYGEN"])
AX_CHECK_COMPILE_FLAG([-masm=intel], [INTELCFLAGS="$INTELCFLAGS -masm=intel"])
AX_CHECK_COMPILE_FLAG([-mpreferred-stack-boundary=12], [INTELCFLAGS="$INTELCFLAGS -mpreferred-stack-boundary=12"])
AX_CHECK_COMPILE_FLAG([-mstackrealign], [INTELCFLAGS="$INTELCFLAGS -mstackrealign"])
AX_CHECK_COMPILE_FLAG([--sanitize=address], [INTELCFLAGS="$INTELCFLAGS --sanitize=address"])
AC_SUBST(INTELCFLAGS)

AS_AC_EXPAND(BINDIR, $bindir)
AS_AC_EXPAND(DATADIR, ${datadir}/wsh)

dnl generate output
AC_CONFIG_FILES([Makefile src/wld/Makefile src/wcc/Makefile src/wsh/Makefile src/wsh/tests/apache2/Makefile src/wsh/tests/sshd/Makefile src/wsh/tests/sshd2/Makefile src/tools/Makefile src/wsh/scripts/INDEX src/wsh/scripts/libmain src/wsh/include/libwitch/wsh.h])
AC_CONFIG_FILES([src/tools/wldd], [chmod +x src/tools/wldd])
AC_CONFIG_FILES([src/tools/wcch], [chmod +x src/tools/wcch])
AC_CONFIG_FILES([src/wsh/scripts/print_G], [chmod +x src/wsh/scripts/print_G])
AC_CONFIG_FILES([src/wsh/scripts/md5.wsh], [chmod +x src/wsh/scripts/md5.wsh])
AC_CONFIG_FILES([src/wsh/scripts/read.wsh], [chmod +x src/wsh/scripts/read.wsh])

AC_OUTPUT
