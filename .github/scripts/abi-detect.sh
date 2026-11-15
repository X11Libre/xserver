#!/bin/bash
# SPDX-License-Identifier: MIT
#
# Master's driver ABI detector: compare what drivers see of two commits, the
# staged SDK headers and the Xorg binary and library modules.
#
#   abi-detect.sh <work-dir> <base-commit> <head-commit>
#
# Writes abi_changed=true|false and <side>_staged=true for each stage it has
# to $GITHUB_OUTPUT. A change is a warning annotation and keeps the exit
# status 0; a failed build or comparison is an error annotation, exit 1 and
# abi_changed=true. All error annotations come before all warnings. A change
# to abi/ or the driver build script also sets abi_changed=true.
# Stages already under <work-dir>/<side>/root are reused (build-server.sh).

set -u

[ $# -eq 3 ] || {
	echo "usage: $0 <work-dir> <base-commit> <head-commit>" >&2
	exit 2
}
work=$1 base=$2 head=$3
top=$(git rev-parse --show-toplevel)
reports=$work/reports
out=${GITHUB_OUTPUT:-/dev/null}
summary=${GITHUB_STEP_SUMMARY:-/dev/null}
changed=false
errors=()
warnings=()
notes=()

error() { errors+=("$1"); }
warn() { warnings+=("$1"); changed=true; }
note() { notes+=("$1"); changed=true; }

# an annotation message: % as %25, newlines as %0A
annotate() {
	local msg=${2//'%'/%25}
	echo "::$1::${msg//$'\n'/%0A}"
}

finish() {
	local m
	for m in "${errors[@]}"; do annotate error "$m"; done
	for m in "${warnings[@]}"; do annotate warning "$m"; done
	[ ${#errors[@]} -eq 0 ] || changed=true
	echo "abi_changed=$changed" >>"$out"
	{
		echo "### Driver ABI detector"
		echo
		echo "Base \`${base:0:10}\`, head \`${head:0:10}\`: abi_changed=$changed"
		echo
		for m in "${errors[@]}"; do echo "- error: ${m//$'\n'/; }"; done
		for m in "${warnings[@]}"; do echo "- ${m//$'\n'/; }"; done
		for m in "${notes[@]}"; do echo "- ${m//$'\n'/; }"; done
	} >>"$summary"
	[ ${#errors[@]} -eq 0 ]
	exit $?
}

if [ -z "$base" ] || [ -z "${base//0/}" ] ||
	! git cat-file -e "$base^{commit}" 2>/dev/null; then
	warn "no base commit to compare with ('$base'); the driver jobs run"
	finish
fi

# the driver jobs also test the check tools and the driver build script
tools=$(git diff --name-only "$base" "$head" -- abi \
	.github/scripts/ubuntu/compile-driver-single.sh)
[ -z "$tools" ] || note "check tools changed, the driver jobs run:"$'\n'"$tools"

mkdir -p "$reports"
export XSERVER_SRC=$top
for side in base head; do
	sha=$base
	[ $side = head ] && sha=$head
	echo "::group::build the $side commit ${sha:0:10}"
	"$top/abi/build-server.sh" "$sha" "$work/$side" 2>&1 |
		tee "$reports/build-$side.log"
	rc=${PIPESTATUS[0]}
	echo "::endgroup::"
	if [ "$rc" -ne 0 ]; then
		error "building the $side commit ${sha:0:10} failed (exit $rc); see build-$side.log in the abi-detect artifact"
		finish
	fi
	echo "${side}_staged=true" >>"$out"
done

binc=$work/base/root/usr/include/xorg
hinc=$work/head/root/usr/include/xorg
# one line per header: changed, added or removed, relative to include/xorg
header_changes() {
	local line f
	diff -rq "$binc" "$hinc" | while IFS= read -r line; do
		case $line in
		"Files $binc/"*" differ")
			f=${line#"Files $binc/"}
			echo "changed: ${f%% and *}" ;;
		"Only in $binc"*)
			f=${line#"Only in $binc"}
			f=${f#/}
			echo "removed: ${f/: //}" | sed 's#^removed: /#removed: #' ;;
		"Only in $hinc"*)
			f=${line#"Only in $hinc"}
			f=${f#/}
			echo "added: ${f/: //}" | sed 's#^added: /#added: #' ;;
		esac
	done
}

diff -ruN "$binc" "$hinc" >"$reports/headers.diff"
case $? in
0) ;;
1) warn "SDK headers changed:"$'\n'"$(header_changes)" ;;
*) error "comparing the SDK headers failed" ;;
esac

# bin/Xorg, or xorg/modules/<path> without the libdir and ABI directory
key() {
	echo "${1#*/root/usr/}" | sed -E 's#^.*/(xorg/modules/)(xlibre-[0-9.]+/)?#\1#'
}
binaries() {
	find "$1/root/usr" \( -name '*.so' -o -path '*/bin/Xorg' \) -type f |
		grep -vE '/xorg/modules/[^/]+/(drivers|input)/' | sort
}

# abigail-compare.sh writes each report; the reports are judged below with
# the release check's rules (abi-baseline.py), not by its exit status
declare -A name
for old in $(binaries "$work/base"); do
	k=$(key "$old")
	new=
	for h in $(binaries "$work/head"); do
		[ "$(key "$h")" = "$k" ] && { new=$h; break; }
	done
	if [ -z "$new" ]; then
		warn "$k is missing in the head build"
		continue
	fi
	n=$(echo "$k" | tr '/' '_')
	name[$n]=$k
	res=$(ABI_REPORT=$reports/$n.txt \
		"$top/abi/abigail-compare.sh" "$old" "$binc" "$new" "$hinc" 2>&1)
	rc=$?
	[ $rc -lt 2 ] || error "comparing $k failed:"$'\n'"$(echo "$res" | tail -3)"
done

# "abi-baseline: NEW: <report name> | <entry> | <details>", one per difference
res=$(python3 "$top/.github/scripts/abi-baseline.py" check "$reports" - 2>&1)
rc=$?
if [ $rc -ge 2 ]; then
	error "judging the reports failed:"$'\n'"$(echo "$res" | tail -3)"
	finish
fi
for n in $(echo "$res" | sed -n 's/^abi-baseline: NEW: \([^ ]*\) | .*/\1/p' | sort -u); do
	warn "ABI change in ${name[$n]:-$n}:"$'\n'"$(echo "$res" |
		sed -n "s/^abi-baseline: NEW: $n | //p" | sed 's/ | / /')"
done

finish
