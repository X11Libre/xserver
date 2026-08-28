#!/bin/sh
# typegen wrapper for the meson build (and manual use).
# Generates the X11 reply-struct C header from the per-extension YAML specs.
#
# Usage: gen.sh [-o <outfile>] <input.yaml...>
#
# SPDX-License-Identifier: AGPL-3.0-or-later

set -eu

DIR="$(cd "$(dirname "$0")" && pwd)"
CWD="$(pwd)"

# normalize relative paths against the caller's cwd: the go run below
# switches into the module dir, where relative args would mis-resolve
norm() {
    case "$1" in
        -* | /*) printf '%s\n' "$1" ;;
        *) printf '%s/%s\n' "$CWD" "$1" ;;
    esac
}

# shellcheck disable=SC2046
set -- $(for a in "$@"; do norm "$a"; done)

cd "$DIR"
exec go run -mod=vendor . "$@"