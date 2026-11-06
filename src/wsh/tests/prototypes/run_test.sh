#!/bin/sh
#
# Regression test for prototypes_c() SAL-annotated header generation.
#
# Feeds a crafted learnwitch.log describing argument behaviour for functions
# in a (not-loaded) library and checks that prototypes_c() renders them as C
# declarations with the right Microsoft SAL annotations:
#   _input_ptr          -> _In_ const void *
#   _output_ptr         -> _Out_ void *
#   input + output      -> _Inout_ void *
#   _exec_ptr           -> _Exec_ void (*)()
#
# Copyright 2016-2026 Jonathan Brossard. MIT License.
#

set -e
cd "$(dirname "$0")"

WSH=../../wsh
if [ ! -x "$WSH" ]; then
	echo "FAIL: $WSH not built (run 'make' in src/wsh first)"
	exit 1
fi

# Generate the learn fixture here rather than shipping it: the repo's
# .gitignore excludes learnwitch.log, so a committed fixture would be
# dropped from the tree. prototypes_c() reads ./learnwitch.log from the
# current directory.
cat > learnwitch.log <<'FIXTURE'
TAG testlibproto.so tfoo argument1 _input_ptr 0
TAG testlibproto.so tfoo argument2 _output_ptr 8
TAG testlibproto.so tfoo argument2 _input_ptr 0
TAG testlibproto.so tcb argument1 _exec_ptr 0
TAG testlibproto.so tscalar argument1 _output_ptr 0
FIXTURE

OUT=$("$WSH" prototypes.wsh 2>/dev/null | grep -E '^void \*')

rm -f learnwitch.log

fail=0
check() {
	if printf '%s\n' "$OUT" | grep -Fq "$2"; then
		echo "  ok   - $1"
	else
		echo "  FAIL - $1"
		fail=1
	fi
}

check "input ptr -> _In_, in+out ptr -> _Inout_" "void *tfoo(_In_ const void *argument1, _Inout_ void *argument2);"
check "exec ptr -> _Exec_ function pointer"       "void *tcb(_Exec_ void (*argument1)());"
check "output-only ptr -> _Out_"                  "void *tscalar(_Out_ void *argument1);"

if [ "$fail" -eq 0 ]; then
	echo "PASS: prototypes_c SAL header regression test"
	exit 0
else
	echo "FAIL: prototypes_c SAL header regression test"
	echo "----- wsh output -----"
	printf '%s\n' "$OUT"
	exit 1
fi
