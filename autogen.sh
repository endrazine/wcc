#!/bin/sh

set -e

libtoolize --install --force || glibtoolize --install --force
aclocal -Im4
autoheader
automake --gnu --add-missing --copy
autoconf
