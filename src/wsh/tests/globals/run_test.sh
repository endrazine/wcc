#!/bin/sh
#
# Regression test for wsh globals tracing (globals_trace).
#
# Builds the target library, runs the globals.wsh driver under wsh, and
# asserts that the expected global-variable accesses were detected: reads
# vs writes, across .data / .bss / .rodata, with variables resolved to
# their symbol names and attributed to the accessing function.
#
# Exit 0 on success, 1 on failure. Addresses vary with ASLR, so the checks
# match on (mode, variable, section, function), never on absolute addresses.
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

make --no-print-directory >/dev/null 2>&1 || { echo "FAIL: could not build target library"; exit 1; }

OUT=$("$WSH" globals.wsh 2>/dev/null)

fail=0
check() {
	# check <description> <extended-regex matched against full output>
	if printf '%s\n' "$OUT" | grep -Eq "$2"; then
		echo "  ok   - $1"
	else
		echo "  FAIL - $1"
		fail=1
	fi
}

# bump(): reads g_initialized, reads+writes g_counter (all .data)
check "bump reads g_initialized (.data)"        'R[[:space:]]+0x[0-9a-f]+ g_initialized[[:space:]]+\.data\+.*bump\(\)'
check "bump writes g_counter (.data)"           'W[[:space:]]+0x[0-9a-f]+ g_counter[[:space:]]+\.data\+.*bump\(\)'

# copy_msg(): reads .rodata, writes .data and .bss
check "copy_msg reads g_rodata_msg (.rodata)"   'R[[:space:]]+0x[0-9a-f]+ g_rodata_msg.*\.rodata\+.*copy_msg\(\)'
check "copy_msg writes g_name (.data)"          'W[[:space:]]+0x[0-9a-f]+ g_name.*\.data\+.*copy_msg\(\)'
check "copy_msg writes g_zero_bss (.bss)"       'W[[:space:]]+0x[0-9a-f]+ g_zero_bss[[:space:]]+\.bss\+.*copy_msg\(\)'

# ---- globals_reify(): live read/write of globals by name ----
ROUT=$("$WSH" reify.wsh 2>/dev/null | tr -d '\000' | grep -E '^[RW] ')

rcheck() {
	if printf '%s\n' "$ROUT" | grep -Eq "$2"; then
		echo "  ok   - $1"
	else
		echo "  FAIL - $1"
		fail=1
	fi
}

rcheck "reify reads g_counter initial (100)"     '^R g_counter 100$'
rcheck "reify reads g_initialized (7)"           '^R g_initialized 7$'
rcheck "reify writes g_counter live (777)"       '^W g_counter 777$'
rcheck "reify returns nil for unknown global"    '^R missing nil$'

# ---- on-disk .symtab: name file-local (static) globals ----
SOUT=$("$WSH" symtab.wsh 2>/dev/null | tr -d '\000' | grep -E '^  \[0')

scheck() {
	if printf '%s\n' "$SOUT" | grep -Eq "$2"; then
		echo "  ok   - $1"
	else
		echo "  FAIL - $1"
		fail=1
	fi
}

# Names must resolve from .symtab (not the <section+offset> fallback)
scheck "symtab names static .data global s_counter" '[[:space:]]s_counter[[:space:]]+\.data\+'
scheck "symtab names static .bss global s_bss"       '[[:space:]]s_bss[[:space:]]+\.bss\+'

# And the <section+offset> fallback must NOT appear (names fully resolved)
if printf '%s\n' "$SOUT" | grep -q '<\.'; then
	echo "  FAIL - symtab resolution still shows <section+offset> fallback"
	fail=1
else
	echo "  ok   - symtab resolution leaves no <section+offset> fallback"
fi

if [ "$fail" -eq 0 ]; then
	echo "PASS: globals tracing + reification regression test"
	exit 0
else
	echo "FAIL: globals tracing + reification regression test"
	echo "----- trace output -----"
	printf '%s\n' "$OUT"
	echo "----- reify output -----"
	printf '%s\n' "$ROUT"
	exit 1
fi
