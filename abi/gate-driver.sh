#!/bin/sh
# SPDX-License-Identifier: MIT
#
# ABI gate for a driver change: build the committed tree (BEFORE_REF,
# default HEAD) and the working tree against each ABI server and require
# that the server types the driver uses are unchanged between the two.
#
#   ABI_WORK=<dir> ABI_MODULE=<path> [ABI_DRIVER_SRC=<dir>] [ABI_SERVERS=...]
#   [BEFORE_REF=<ref>] gate-driver.sh

set -eu
. "$(dirname "$0")/common.sh"
: "${ABI_MODULE:?set ABI_MODULE to the module in a driver build, e.g. src/amdgpu_drv.so}"

before_src=$ABI_WORK/driver-before
rm -rf "$before_src"
mkdir -p "$before_src"
git -C "$ABI_DRIVER_SRC" archive "${BEFORE_REF:-HEAD}" | tar -x -C "$before_src"

fail=0
for s in $ABI_SERVERS; do
	wd=$(abi_server "$s")
	inc=$wd/root/usr/include/xorg
	if ! abi_build_driver "$wd" "$wd/gate-before" "$before_src" ||
		! abi_build_driver "$wd" "$wd/gate-after" "$ABI_DRIVER_SRC"; then
		echo "gate-driver: FAIL $s (driver build failed)"
		fail=1
		continue
	fi
	if ABI_REPORT=$wd/gate-abidiff.txt "$abi_dir/abigail-compare.sh" \
		"$wd/gate-before/$ABI_MODULE" "$inc" \
		"$wd/gate-after/$ABI_MODULE" "$inc" >/dev/null; then
		echo "gate-driver: PASS $s"
	else
		echo "gate-driver: FAIL $s (report: $wd/gate-abidiff.txt)"
		fail=1
	fi
done
exit $fail
