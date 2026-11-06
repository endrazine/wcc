#!/bin/sh
#
# Regression test for wsh raw system calls (lsyscall).
#
# Verifies the syscall call path end to end: a successful no-arg syscall
# returns a sane value with errno 0 and is stable across calls; getuid
# succeeds; an invalid syscall number returns -1 with errno ENOSYS (38);
# and a pointer-argument syscall (uname) succeeds (ret 0), proving the
# kernel accepted a userland pointer passed from Lua.
#
# Syscall numbers in syscall.wsh are x86_64; skip on other arches.
#
# Copyright 2016-2026 Jonathan Brossard. MIT License.
#

set -e
cd "$(dirname "$0")"

arch=$(uname -m)
if [ "$arch" != "x86_64" ]; then
	echo "SKIP: syscall test uses x86_64 syscall numbers (arch=$arch)"
	exit 0
fi

WSH=../../wsh
if [ ! -x "$WSH" ]; then
	echo "FAIL: $WSH not built (run 'make' in src/wsh first)"
	exit 1
fi

OUT=$("$WSH" syscall.wsh 2>/dev/null | grep -E '^(getpid|getuid|enosys|uname) ')

fail=0
check() {
	if printf '%s\n' "$OUT" | grep -Eq "$2"; then
		echo "  ok   - $1"
	else
		echo "  FAIL - $1"
		fail=1
	fi
}

check "getpid succeeds, errno 0, stable and positive" '^getpid [0-9]+ 0 true$'
check "getuid succeeds, errno 0"                       '^getuid [0-9]+ 0$'
check "invalid syscall -> -1 / ENOSYS(38)"             '^enosys -1 38$'
check "uname (pointer arg) succeeds, ret 0"            '^uname 0 0$'

if [ "$fail" -eq 0 ]; then
	echo "PASS: syscall (lsyscall) regression test"
	exit 0
else
	echo "FAIL: syscall (lsyscall) regression test"
	echo "----- wsh output -----"
	printf '%s\n' "$OUT"
	exit 1
fi
