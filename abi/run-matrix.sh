#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Driver ABI check across server releases: build the driver against each
# server in ABI_SERVERS and check that every server symbol the driver needs
# exists in every one of them (a driver built against one release must load
# into the others).
#
#   ABI_WORK=<dir> ABI_MODULE=<path> [ABI_DRIVER_SRC=<dir>] [ABI_SERVERS=...]
#   run-matrix.sh
#
# Layout changes are checked per change by gate-driver.sh.

set -eu
. "$(dirname "$0")/common.sh"
: "${ABI_MODULE:?set ABI_MODULE to the module in a driver build, e.g. src/amdgpu_drv.so}"

fail=0

for s in $ABI_SERVERS; do
	wd=$(abi_server "$s")
	echo "run-matrix: $s -> $wd"
	if ! abi_build_driver "$wd" "$wd/driver" "$ABI_DRIVER_SRC"; then
		echo "run-matrix: FAIL $s (driver build failed)"
		exit 1
	fi
done

for built in $ABI_SERVERS; do
	for target in $ABI_SERVERS; do
		bwd=$(abi_workdir "$built")
		twd=$(abi_workdir "$target")
		out=$bwd/symbols-$(echo "$target" | tr '/' '-').txt
		if "$abi_dir/check-symbols.sh" "$bwd/driver/$ABI_MODULE" "$twd" >"$out"; then
			echo "run-matrix: symbols PASS built for $built, loaded into $target"
		else
			echo "run-matrix: symbols FAIL built for $built, loaded into $target"
			sed 's/^/  /' "$out"
			fail=1
		fi
	done
done
exit $fail
