#!/bin/sh
#
# Runs INSIDE the Debian GNU/Hurd VM (over ssh) — see the xserver-build-hurd job
# in .github/workflows/build-xserver.yml. Builds the X servers that work on
# GNU/Hurd today as the job's FATAL pass/fail gate: the virtual DDXes Xvfb +
# Xnest AND the physical xfree86 Xorg server. It ADDITIONALLY makes a non-fatal
# attempt at the kdrive fbdev server (not yet portable — see below). $REPO and
# $SHA come from the workflow.
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

# FATAL build: the servers proven to build on GNU/Hurd — Xvfb + Xnest (virtual
# DDXes) and the physical xfree86 Xorg server, all in one meson build so a
# regression breaking any of them fails the lane. libpciaccess has a Hurd backend
# (Hurd ships a PCI arbiter), so the xfree86 PCI layer configures/links. udev +
# logind stay off (absent on Hurd → static config / input discovery). The
# GPU/DRM-coupled subsystems are all forced off because Hurd has no DRM kernel
# interface: DRI needs libdrm's <drm.h>, which on Hurd pulls a Mach ioctl header
# (mach/x86_64/ioccom.h) that doesn't exist; and glamor (glamor_egl.c) needs
# DRM_FORMAT_MOD_INVALID / GBM / EGL-on-DRM. So dri1/dri2/dri3, glx and glamor
# stay off.
echo "==> meson setup (Xvfb + Xnest + xfree86 Xorg — the servers that build on Hurd)"
meson setup _build \
    -Dwerror=false \
    -Dxvfb=true -Dxnest=true -Dxorg=true \
    -Dxephyr=false -Dxfbdev=false \
    -Dglx=false -Dglamor=false -Ddri1=false -Ddri2=false -Ddri3=false \
    -Dudev=false -Dsystemd_logind=false

echo "==> meson compile (Xvfb + Xnest + Xorg)"
meson compile -C _build

# NON-FATAL: the kdrive fbdev server. It builds hw/kdrive/linux, which needs
# Linux VTs (<linux/vt.h>) — Hurd has none and would need its own kdrive backend.
# Attempt it so the gap stays visible and the lane lights up green the day a Hurd
# backend lands, without failing CI meanwhile.
echo "==> meson setup (kdrive xfbdev — EXPERIMENTAL, non-fatal)"
kdrive_ok=0
if meson setup _build_kdrive \
    -Dwerror=false \
    -Dxvfb=false -Dxnest=false -Dxorg=false -Dxephyr=false \
    -Dxfbdev=true \
    -Dglx=false -Dglamor=false -Ddri1=false -Ddri2=false -Ddri3=false \
    -Dudev=false -Dsystemd_logind=false
then
    echo "==> meson compile (kdrive xfbdev)"
    if meson compile -C _build_kdrive; then kdrive_ok=1; fi
fi
if [ "$kdrive_ok" = 1 ]; then
    echo "==> RESULT: kdrive xfbdev also BUILDS on GNU/Hurd 🎉"
else
    echo "::warning::kdrive xfbdev does not yet build on GNU/Hurd (needs a Hurd VT/kdrive backend) — non-fatal"
fi
