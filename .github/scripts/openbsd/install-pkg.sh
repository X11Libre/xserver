#!/bin/sh

set -e

# OpenBSD ships X.Org (xenocara) in the base system under /usr/X11R6, so the X
# libraries, the protocol headers and their pkg-config files are already
# present — only the build tooling (and a few libs not in base) come from
# packages via pkg_add. The vmactions OpenBSD image pre-configures the package
# mirror (installurl / PKG_PATH), so a plain pkg_add resolves.

# Retry a command a few times with linear backoff; package mirrors flake
# transiently and a single network hiccup must not abort the whole CI run.
retry() {
    n=0
    max=3
    while true; do
        n=$((n + 1))
        if "$@"; then
            return 0
        fi
        if [ "$n" -ge "$max" ]; then
            echo "--> '$*' failed after $max attempts" >&2
            return 1
        fi
        echo "--> '$*' failed (attempt $n/$max), retrying in $((n * 10))s ..." >&2
        sleep $((n * 10))
    done
}

echo "--> install build tooling + deps not in base"
# -I: non-interactive (don't prompt on ambiguous matches). meson pulls python3.
# epoll-shim: the server hard-requires it on the BSDs (meson.build) to emulate
# the Linux epoll API; it is not part of the base system.
retry pkg_add -I meson ninja pkgconf git epoll-shim
