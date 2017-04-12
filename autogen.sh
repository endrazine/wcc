#!/bin/sh

# You need autoconf 2.5x, preferably 2.57 or later
# You need automake 1.7 or later. 1.6 might work.

set -e

libtoolize --install --force || glibtoolize --install --force
aclocal -Im4
autoheader
automake --gnu --add-missing --copy
autoconf
