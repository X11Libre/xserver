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
