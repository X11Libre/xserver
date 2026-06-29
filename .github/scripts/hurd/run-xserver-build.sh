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

echo "==> apt update"
sudo apt-get update

# Toolchain MUST succeed (apt is transactional: one unlocatable package aborts
# the whole install, so keep the essentials separate from the maybe-renamed X
# libs — otherwise a missing lib takes git/meson down with it, as it did).
echo "==> install toolchain (required)"
sudo apt-get install -y --no-install-recommends \
    git build-essential meson ninja-build pkg-config ca-certificates

# X libraries + helpers: best-effort, one at a time, so a package the Hurd port
# lacks or renames only skips itself (named in the log); meson then reports what
# is genuinely required and absent.
echo "==> install X libs (best-effort)"
for p in \
    libpixman-1-dev libxfont2-dev libxfont-dev libxkbfile-dev xtrans-dev \
    x11proto-dev xorg-sgml-doctools libxcvt-dev libxau-dev libxdmcp-dev \
    libxcb1-dev libx11-dev libxext-dev libxfixes-dev libxrender-dev \
    libxi-dev libxtst-dev libxres-dev libxshmfence-dev libfontenc-dev \
    libtirpc-dev nettle-dev libbsd-dev libgcrypt20-dev libepoxy-dev
do
    sudo apt-get install -y --no-install-recommends "$p" || echo "WARN: package $p not available on hurd"
done

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
