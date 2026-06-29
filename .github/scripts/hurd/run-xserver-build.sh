#!/bin/sh
#
# Runs INSIDE the Debian GNU/Hurd VM (over ssh) — see
# .github/workflows/build-hurd.yml. EXPERIMENTAL: the X server is not (yet)
# ported to GNU/Hurd, so this is expected to surface missing pieces rather than
# go green. $REPO and $SHA are passed in by the workflow.
set -ex

export DEBIAN_FRONTEND=noninteractive

echo "==> uname / arch"
uname -a || true

echo "==> apt deps (best-effort; Hurd repo may lack some)"
sudo apt-get update
sudo apt-get install -y \
    build-essential meson ninja-build pkg-config git ca-certificates \
    libpixman-1-dev libxfont2-dev libxkbfile-dev xtrans-dev \
    x11proto-dev libxcvt-dev libxau-dev libxdmcp-dev libxcb1-dev \
    libx11-dev libxext-dev libxfixes-dev libxrender-dev libxi-dev \
    libxtst-dev libxres-dev libxshmfence-dev libxfont-dev libfontenc-dev \
    libtirpc-dev nettle-dev libbsd-dev || true

echo "==> clone xserver @ $SHA"
rm -rf xserver
git clone --depth 1 "https://github.com/$REPO" xserver
cd xserver
git fetch --depth 1 origin "$SHA"
git checkout FETCH_HEAD

echo "==> meson setup (minimal Xvfb/Xnest, no glx/dri/xorg — bring-up)"
meson setup _build \
    -Dwerror=false \
    -Dxvfb=true -Dxnest=true \
    -Dxorg=false -Dxephyr=false -Dxfbdev=false \
    -Dglx=false -Ddri2=false -Ddri3=false -Dudev=false -Dsystemd_logind=false

echo "==> meson compile"
meson compile -C _build
