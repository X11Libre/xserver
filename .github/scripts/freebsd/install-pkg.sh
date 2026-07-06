#!/bin/sh

set -e

# The CI VM's release is pinned (build-xserver.yml: release: "14.3"), but
# FreeBSD's official package repo tracks the "14" ABI branch, not the exact
# point release — when it rolls forward to packages built against a newer
# point release than the pinned VM, pkg refuses to proceed non-interactively
# ("pkg: repository FreeBSD contains packages for wrong OS version" /
# "Ignore the mismatch and continue? [y/N]:", no TTY in CI to answer it).
# IGNORE_OSVERSION=yes is pkg's own documented escape hatch for exactly this
# case; a one-point-release ABI skew is expected to be compatible in
# practice. This does NOT mask a genuine build/test failure — it only lets
# `pkg update`/`pkg install` proceed past the version check.
export IGNORE_OSVERSION=yes

# Retry a command a few times with linear backoff. pkg mirrors flake
# transiently (catalogue refresh / package fetch), and a single network
# hiccup must not abort the whole CI run.
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

echo "--> refresh package catalogue"
retry pkg update -f

echo "--> install extra dependencies"
retry pkg install -y \
    bzip2 \
    curl \
    git \
    libdrm \
    libepoll-shim \
    libX11 \
    libxkbfile \
    libxshmfence \
    libXfont2 \
    libxcvt \
    libglvnd \
    libepoxy \
    libudev-devd \
    mesa-dri \
    mesa-libs \
    meson \
    pixman \
    pkgconf \
    xcb-util-image \
    xcb-util-keysyms \
    xcb-util-renderutil \
    xcb-util-wm \
    xkbcomp \
    xorgproto

# The FreeBSD `bzip2` package installs libbz2 + headers but, at least on
# this CI image, no bzip2.pc — freetype2's own .pc lists bzip2 in
# Requires.private, so pkgconf's dependency resolution for xfont2 (which
# needs freetype2) fails outright even though the actual library is
# present and perfectly usable. This is a missing-metadata gap in the
# package, not a missing library — synthesize the .pc pkgconf itself
# never shipped, pointing at wherever `pkg` actually placed the files
# (found via `pkg info -l`, not hardcoded, since the exact split between
# base-system and /usr/local varies).
if ! pkg-config --exists bzip2 2>/dev/null; then
    echo "--> bzip2.pc missing from the bzip2 package; synthesizing one"
    bz_hdr=$(pkg info -l bzip2 | grep -m1 '/bzlib\.h$') || true
    bz_lib=$(pkg info -l bzip2 | grep -m1 '/libbz2\.so$') || true
    bz_incdir=$(dirname "${bz_hdr:-/usr/local/include/bzlib.h}")
    bz_libdir=$(dirname "${bz_lib:-/usr/local/lib/libbz2.so}")
    bz_ver=$(pkg info bzip2 2>/dev/null | awk -F'[-:]' '/^Version/ {print $2; exit}')
    pcdir=/usr/local/libdata/pkgconfig
    mkdir -p "$pcdir"
    cat > "$pcdir/bzip2.pc" <<EOF
prefix=/usr/local
libdir=$bz_libdir
includedir=$bz_incdir

Name: bzip2
Description: bzip2 compression library
Version: ${bz_ver:-1.0.8}
Libs: -L\${libdir} -lbz2
Cflags: -I\${includedir}
EOF
    echo "--> wrote $pcdir/bzip2.pc:"
    cat "$pcdir/bzip2.pc"
fi
