# SPDX-License-Identifier: MIT
#
# Shared helpers for the ABI scripts (sourced, not run).
#
# Environment:
#   ABI_WORK        work directory for staged servers and builds (required)
#   ABI_SERVERS     servers to check against (default: the release branch
#                   tips and master)
#   ABI_DRIVER_SRC  driver source tree (default: the git top level of the
#                   current directory)
#   ABI_MODULE      the module inside a driver build dir, e.g.
#                   src/amdgpu_drv.so (meson) or src/.libs/xgi_drv.so
#                   (autotools); needed by gate-driver.sh and run-matrix.sh
#   XSERVER_SRC     local xserver clone to take sources from (optional)
#   XSERVER_REMOTE  remote name in XSERVER_SRC for branches (default upstream)

# The scripts that source this live next to it, so $0 locates the
# directory; ABI_DIR overrides it for callers elsewhere.
abi_dir=${ABI_DIR:-$(cd "$(dirname "$0")" && pwd)}
: "${ABI_WORK:?set ABI_WORK to a work directory}"
: "${ABI_SERVERS:=release/25.0 release/25.1 release/25.2 master}"
: "${XSERVER_REMOTE:=upstream}"
mkdir -p "$ABI_WORK"
ABI_WORK=$(cd "$ABI_WORK" && pwd)
ABI_DRIVER_SRC=$(cd "${ABI_DRIVER_SRC:-$(git rev-parse --show-toplevel)}" && pwd)

# Print the commit a tag or branch name currently points to.
abi_resolve() {
	if [ -n "${XSERVER_SRC:-}" ]; then
		git -C "$XSERVER_SRC" rev-parse --verify --quiet "refs/tags/$1^{commit}" ||
			git -C "$XSERVER_SRC" rev-parse --verify "$XSERVER_REMOTE/$1^{commit}"
	else
		git ls-remote https://github.com/X11Libre/xserver \
			"refs/tags/$1^{}" "refs/tags/$1" "refs/heads/$1" |
			head -n 1 | cut -f1
	fi
}

# Print the work directory for a tag or branch at its current commit.
abi_workdir() {
	commit=$(abi_resolve "$1")
	[ -n "$commit" ] || { echo "abi: cannot resolve $1" >&2; return 1; }
	echo "$ABI_WORK/$(echo "$1" | tr '/' '-')-$(echo "$commit" | cut -c1-12)"
}

# Build the driver into <builddir> (build-driver.sh arguments), logging to
# <builddir>.log. On failure print the log's tail and return 1, so a failed
# build is reported instead of silently ending the calling script.
abi_build_driver() {
	log=$2.log
	if "$abi_dir/build-driver.sh" "$@" >"$log" 2>&1; then
		return 0
	fi
	echo "abi: driver build failed ($2), last lines of $log:"
	tail -n 20 "$log" | sed 's/^/  /'
	return 1
}

# Build (or reuse) the staged server for a tag or branch; print its workdir.
abi_server() {
	wd=$(abi_workdir "$1") || return 1
	if [ -n "${XSERVER_SRC:-}" ]; then
		"$abi_dir/build-server.sh" "$(abi_resolve "$1")" "$wd" >&2 || return 1
	else
		"$abi_dir/build-server.sh" "$1" "$wd" >&2 || return 1
	fi
	echo "$wd"
}
