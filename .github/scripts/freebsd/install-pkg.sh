#!/bin/sh

set -e

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
