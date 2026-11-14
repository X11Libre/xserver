#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Release branch ABI check: build the branch's ABI base release and HEAD,
# compare Xorg and the library modules with libabigail, and check the
# differences against the branch's baseline.
#
#   abi-check.sh <work-dir> [<branch>]
#
# <branch> defaults to the pull request's base branch or the pushed branch.
# Major 25: base xlibre-xserver-25.0.0.0, accepted differences in
# .github/abi/baseline-release-25.X.txt. Major 26 and later: base
# xlibre-xserver-<major>.0.0 and no baseline, so any difference fails.
# In-tree drivers (modules/drivers, modules/input) are not checked.

set -eu

[ $# -ge 1 ] && [ $# -le 2 ] || {
	echo "usage: $0 <work-dir> [<branch>]" >&2
	exit 2
}
work=$1
branch=${2:-${GITHUB_BASE_REF:-${GITHUB_REF_NAME:-}}}
top=$(git rev-parse --show-toplevel)

case $branch in
release/[0-9]*.[0-9]*) ;;
*) echo "abi-check: '$branch' is not a release branch" >&2; exit 2 ;;
esac
major=${branch#release/}
major=${major%%.*}
if [ "$major" -eq 25 ]; then
	base=xlibre-xserver-25.0.0.0
	baseline=$top/.github/abi/baseline-$(echo "$branch" | tr '/' '-').txt
	[ -f "$baseline" ] || { echo "abi-check: no $baseline" >&2; exit 2; }
elif [ "$major" -ge 26 ]; then
	base=xlibre-xserver-$major.0.0
	baseline=-
else
	echo "abi-check: no ABI base for major $major" >&2
	exit 2
fi
git rev-parse --verify --quiet "refs/tags/$base^{commit}" >/dev/null || {
	echo "abi-check: tag $base not found (are tags fetched?)" >&2
	exit 2
}
echo "abi-check: $branch at $(git rev-parse --short HEAD) against $base"

export XSERVER_SRC=$top
mkdir -p "$work"
"$top/abi/build-server.sh" "$(git rev-parse "$base^{commit}")" "$work/base"
"$top/abi/build-server.sh" "$(git rev-parse HEAD)" "$work/head"

# bin/Xorg, or xorg/modules/<path> without the libdir, which differs by distro
key() {
	echo "${1#*/root/usr/}" | sed -E 's#^.*/(xorg/modules/)(xlibre-[0-9.]+/)?#\1#'
}
binaries() {
	find "$1/root/usr" \( -name '*.so' -o -path '*/bin/Xorg' \) -type f |
		grep -vE '/xorg/modules/[^/]+/(drivers|input)/' | sort
}

reports=$work/reports
rm -rf "$reports"
mkdir -p "$reports"
fail=0
for old in $(binaries "$work/base"); do
	k=$(key "$old")
	new=
	for h in $(binaries "$work/head"); do
		[ "$(key "$h")" = "$k" ] && { new=$h; break; }
	done
	if [ -z "$new" ]; then
		echo "abi-check: FAIL: $k is missing in HEAD"
		fail=1
		continue
	fi
	rc=0
	ABI_REPORT=$reports/$(echo "$k" | tr '/' '_').txt \
		"$top/abi/abigail-compare.sh" "$old" "$work/base/root/usr/include/xorg" \
		"$new" "$work/head/root/usr/include/xorg" >/dev/null || rc=$?
	if [ $rc -ge 2 ]; then
		echo "abi-check: abidiff failed on $k (exit $rc)" >&2
		exit 2
	fi
done

python3 "$top/.github/scripts/abi-baseline.py" check "$reports" "$baseline" || fail=1
exit $fail
