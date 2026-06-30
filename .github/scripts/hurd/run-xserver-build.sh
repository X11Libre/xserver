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

# --- non-fatal probes for two review points (stefan11111 on PR #3193) -------
# PROBE 1: glx "should build without libdrm" — probe glx ALONE on top of Xorg
# (glamor off, dri off). DRI is the part that hard-needs libdrm's <drm.h> → the
# nonexistent mach/x86_64/ioccom.h; glamor is probed separately below.
echo "==> PROBE 1: meson setup (Xorg + glx, no glamor, dri off — non-fatal)"
glx_ok=0
if meson setup _build_glx \
    -Dwerror=false \
    -Dxvfb=false -Dxnest=false -Dxephyr=false -Dxfbdev=false \
    -Dxorg=true -Dglx=true -Dglamor=false \
    -Ddri1=false -Ddri2=false -Ddri3=false -Dudev=false -Dsystemd_logind=false
then
    echo "==> meson compile (Xorg + glx)"
    if meson compile -C _build_glx; then glx_ok=1; fi
fi
[ "$glx_ok" = 1 ] \
    && echo "==> PROBE 1 RESULT: glx BUILDS on GNU/Hurd without DRI 🎉" \
    || echo "::warning::PROBE 1: glx does not build on GNU/Hurd (see errors above) — non-fatal"

# PROBE 1b: glamor. We expect this to fail without DRM/GBM (glamor_egl.c needs
# DRM_FORMAT_MOD_INVALID + gbm, and gbm is unavailable on Hurd), but probe it to
# confirm/refute.
echo "==> PROBE 1b: meson setup (Xorg + glamor, dri off — non-fatal)"
glamor_ok=0
if meson setup _build_glamor \
    -Dwerror=false \
    -Dxvfb=false -Dxnest=false -Dxephyr=false -Dxfbdev=false \
    -Dxorg=true -Dglx=false -Dglamor=true \
    -Ddri1=false -Ddri2=false -Ddri3=false -Dudev=false -Dsystemd_logind=false
then
    echo "==> meson compile (Xorg + glamor)"
    if meson compile -C _build_glamor; then glamor_ok=1; fi
fi
[ "$glamor_ok" = 1 ] \
    && echo "==> PROBE 1b RESULT: glamor BUILDS on GNU/Hurd without DRI 🎉" \
    || echo "::warning::PROBE 1b: glamor does not build on GNU/Hurd (needs DRM/GBM) — non-fatal"

# PROBE 2: Xephyr "should work, it doesn't use the kdrive linux input drivers."
# Xephyr is a kdrive server that runs as an X client over XCB, not the linux
# fbdev/VT path that blocks xfbdev — so unlike xfbdev it has a real chance.
echo "==> PROBE 2: meson setup (Xephyr — non-fatal)"
xephyr_ok=0
if meson setup _build_xephyr \
    -Dwerror=false \
    -Dxvfb=false -Dxnest=false -Dxorg=false -Dxfbdev=false \
    -Dxephyr=true \
    -Dglx=false -Dglamor=false -Ddri1=false -Ddri2=false -Ddri3=false \
    -Dudev=false -Dsystemd_logind=false
then
    echo "==> meson compile (Xephyr)"
    if meson compile -C _build_xephyr; then xephyr_ok=1; fi
fi
[ "$xephyr_ok" = 1 ] \
    && echo "==> PROBE 2 RESULT: Xephyr BUILDS on GNU/Hurd 🎉" \
    || echo "::warning::PROBE 2: Xephyr does not build on GNU/Hurd (see errors above) — non-fatal"
