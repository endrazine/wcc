#!/bin/sh

set -e

libtoolize --install -c --force || glibtoolize --install -c --force
aclocal -Im4
autoheader
automake --gnu --add-missing --copy
autoconf
