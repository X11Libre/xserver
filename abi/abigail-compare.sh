#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Compare two builds of the same binary with libabigail, looking at every
# type declared in the server SDK headers (including types not reachable
# from exported symbols, which is all of them for a driver built with
# -fvisibility=hidden).
#
#   abigail-compare.sh <old-binary> <old-incdir> <new-binary> <new-incdir> \
#       [--allow TYPE]...
#
# Exit 0: no ABI change, or only changes to --allow'ed types with nothing
# removed. Exit 1: ABI change that is not allowed. Exit 2: tool error.

set -u

usage() {
	echo "usage: $0 <old> <old-incdir> <new> <new-incdir> [--allow TYPE]..." >&2
	exit 2
}

[ $# -ge 4 ] || usage
old=$1 oldinc=$2 new=$3 newinc=$4
shift 4
# Type lists hold one name per line: names of anonymous types contain spaces.
nl='
'
allow=
while [ $# -gt 0 ]; do
	case $1 in
	--allow)
		[ $# -ge 2 ] || usage
		allow="$allow$2$nl"
		shift 2
		;;
	*) usage ;;
	esac
done

if ! command -v abidiff >/dev/null 2>&1; then
	echo "abigail-compare: abidiff not found (Gentoo: dev-util/libabigail," \
		"Ubuntu: abigail-tools)" >&2
	exit 2
fi

report=${ABI_REPORT:-$(dirname "$new")/abidiff-$(basename "$new").txt}
timeout 1800 abidiff --no-show-locs --no-added-syms --non-reachable-types \
	--leaf-changes-only --hd1 "$oldinc" --hd2 "$newinc" \
	"$old" "$new" >"$report" 2>&1
rc=$?
cat "$report"

if [ $rc -ge 128 ] || [ $((rc & 3)) -ne 0 ]; then
	echo "abigail-compare: abidiff failed (exit $rc)" >&2
	exit 2
fi
if [ $rc -eq 0 ]; then
	echo "abigail-compare: PASS (no ABI change)"
	exit 0
fi

# Added types (e.g. a newly used server struct) set the change bit too, as do
# types that only left the binary's debug info because it no longer uses
# them; with no function, variable or type changed or removed from the
# interface, that is not an ABI break.
if grep -q "^Leaf changes summary: 0 artifact" "$report" &&
	grep -q "functions summary: 0 Removed, 0 Changed" "$report" &&
	grep -q "variables summary: 0 Removed, 0 Changed" "$report" &&
	! grep "^Unreachable types summary:" "$report" |
		grep -Eqv "summary: [0-9]+ removed( \([0-9]+ filtered out\))?, 0 changed"; then
	if grep -Eq "^Unreachable types summary: [1-9]" "$report"; then
		echo "abigail-compare: PASS (only additions, and types this binary" \
			"no longer uses)"
	else
		echo "abigail-compare: PASS (only additions)"
	fi
	exit 0
fi

fail=0
if ! grep -q "functions summary: 0 Removed" "$report" ||
	! grep -q "variables summary: 0 Removed" "$report"; then
	echo "abigail-compare: FAIL: functions or variables were removed"
	fail=1
fi

# Leaf entries look like "'struct _damage' changed:" (types reachable from
# exported symbols) or "  [C] 'struct _damage' changed:" (unreachable
# types), followed by indented detail lines. A type counts as changed only
# when its own entry reports a layout change; entries that just repeat a
# change through a pointer ("underlying type ... changed") do not.
changed=
current=
while IFS= read -r line; do
	case $line in
	"'"*"' changed:" | "  [C] '"*"' changed:")
		current=${line#  \[C\] }
		current=${current#\'}
		current=${current%\' changed:}
		for prefix in "const " "struct " "union " "enum " "class " "typedef "; do
			current=${current#"$prefix"}
		done
		;;
	"  ["?"] "* | [!\ ]*)
		# another tagged entry, or a top-level line that is not a header
		current=
		;;
	*"type size changed"* | *"data member insertion"* | \
	*"data member deletion"* | *"offset changed"* | \
	*"was removed"* | *"enumerator"* | *"type name changed"*)
		if [ -n "$current" ]; then
			case "$nl$changed" in
			*"$nl$current$nl"*) ;;
			*) changed="$changed$current$nl" ;;
			esac
		fi
		;;
	esac
done <"$report"

if [ -z "$changed" ] && [ $fail -eq 0 ]; then
	echo "abigail-compare: FAIL: abidiff reported a change (exit $rc)" \
		"that this script cannot attribute to a type; read the report"
	exit 1
fi

while IFS= read -r t; do
	[ -n "$t" ] || continue
	case "$nl$allow" in
	*"$nl$t$nl"*) echo "abigail-compare: allowed change to $t" ;;
	*)
		echo "abigail-compare: FAIL: layout of $t changed"
		fail=1
		;;
	esac
done <<EOF
$changed
EOF

[ $fail -eq 0 ] && echo "abigail-compare: PASS (only allowed changes)"
exit $fail
