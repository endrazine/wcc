#!/bin/sh

set -e

case `uname` in Darwin*) glibtoolize --install --force --copy ;;
  *) libtoolize --install --force --copy ;; esac
  
aclocal -Im4
autoheader
automake --gnu --add-missing --copy
autoconf
