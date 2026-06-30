#!/bin/sh
#
# Runs INSIDE the Debian GNU/Hurd VM (over ssh) — see the xserver-build-hurd job
# in .github/workflows/build-xserver.yml. Builds, as the job's FATAL pass/fail
# gate, every X server proven to build on GNU/Hurd: Xvfb, Xnest, the physical
# xfree86 Xorg server, Xephyr, and the GLX extension. The DRM-coupled bits
# (dri*, glamor) and Xfbdev stay off — they need a DRM kernel interface / Linux
# VTs that Hurd lacks (see the build section). $REPO and $SHA come from the
# workflow.
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
    libtirpc-dev nettle-dev libbsd-dev libgcrypt20-dev libepoxy-dev \
    libxcb-util-dev libxcb-icccm4-dev libxcb-shape0-dev libxcb-xkb-dev \
    libxcb-keysyms1-dev libxcb-image0-dev libxcb-render-util0-dev \
    libxcb-randr0-dev libxcb-shm0-dev libxcb-render0-dev \
    libxcb-xv0-dev libxcb-glx0-dev \
    libpciaccess-dev libdrm-dev
do
    sudo apt-get install -y --no-install-recommends "$p" || echo "WARN: package $p not available on hurd"
done

echo "==> clone xserver @ $SHA"
rm -rf xserver
git clone --depth 1 "https://github.com/$REPO" xserver
cd xserver
git fetch --depth 1 origin "$SHA"
git checkout FETCH_HEAD

# FATAL build: every server proven to build on GNU/Hurd, in one meson build so a
# regression breaking any of them fails the lane:
#   - Xvfb, Xnest    virtual DDXes
#   - Xorg           the physical xfree86 server (libpciaccess has a Hurd backend,
#                    so the PCI layer configures/links)
#   - Xephyr         kdrive server that runs as an X client over XCB (no linux
#                    fbdev/VT path), so unlike Xfbdev it builds fine on Hurd
#   - glx            the GLX extension builds without libdrm (software/indirect)
# udev + logind stay off (absent on Hurd → static config / input discovery).
# The DRM-coupled subsystems stay off because Hurd has no DRM kernel interface:
#   - dri1/dri2/dri3 — libdrm's <drm.h> pulls a nonexistent mach/x86_64/ioccom.h
#   - glamor         — glamor_egl.c needs DRM_FORMAT_MOD_INVALID + GBM (no GBM
#                      without DRM); confirmed non-buildable on Hurd
# Xfbdev (kdrive fbdev) also stays off: it builds hw/kdrive/linux, which needs
# Linux VTs (<linux/vt.h>) and would require a dedicated Hurd kdrive backend.
echo "==> meson setup (Xvfb + Xnest + Xorg + Xephyr + glx — the servers that build on Hurd)"
meson setup _build \
    -Dwerror=false \
    -Dxvfb=true -Dxnest=true -Dxorg=true -Dxephyr=true \
    -Dglx=true \
    -Dxfbdev=false -Dglamor=false -Ddri1=false -Ddri2=false -Ddri3=false \
    -Dudev=false -Dsystemd_logind=false

echo "==> meson compile (Xvfb + Xnest + Xorg + Xephyr + glx)"
meson compile -C _build
