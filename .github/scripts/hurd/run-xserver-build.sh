#!/bin/sh
#
# Runs INSIDE the Debian GNU/Hurd VM (over ssh) — see the xserver-build-hurd job
# in .github/workflows/build-xserver.yml. The virtual DDXes (Xvfb/Xnest) build
# green on Hurd and are the job's pass/fail baseline; this script ADDITIONALLY
# makes a non-fatal attempt at the physical DDXes (xfree86 Xorg + kdrive fbdev)
# to surface what's still missing for a Hurd port. $REPO and $SHA come from the
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

echo "==> meson setup (baseline: virtual DDXes Xvfb/Xnest — must succeed)"
meson setup _build \
    -Dwerror=false \
    -Dxvfb=true -Dxnest=true \
    -Dxorg=false -Dxephyr=false -Dxfbdev=false \
    -Dglx=false -Ddri2=false -Ddri3=false -Dudev=false -Dsystemd_logind=false

echo "==> meson compile (baseline)"
meson compile -C _build

# Now ALSO attempt the PHYSICAL DDXes: the xfree86 server (-Dxorg) and the
# kdrive framebuffer server (-Dxfbdev). These are not (yet) ported to GNU/Hurd,
# so this stage is NON-FATAL — it surfaces the missing Hurd pieces without
# taking down the green Xvfb/Xnest baseline above. libpciaccess has a Hurd
# backend (Hurd ships a PCI arbiter), so the xfree86 PCI layer has a real chance
# to configure/link. udev + logind stay off (absent on Hurd → static config /
# input discovery). The GPU/DRM-coupled subsystems are all forced off because
# Hurd has no DRM kernel interface: DRI needs libdrm's <drm.h>, which on Hurd
# pulls a Mach ioctl header (mach/x86_64/ioccom.h) that doesn't exist; and glamor
# (glamor_egl.c) needs DRM_FORMAT_MOD_INVALID / GBM / EGL-on-DRM. So dri1/dri2/
# dri3, glx and glamor are off — leaving the xfree86 core (PCI, OS-support,
# input) and the kdrive fbdev server as the real Hurd-port frontier.
echo "==> meson setup (PHYSICAL DDXes: -Dxorg + -Dxfbdev — EXPERIMENTAL, non-fatal)"
phys_ok=0
if meson setup _build_phys \
    -Dwerror=false \
    -Dxvfb=false -Dxnest=false -Dxephyr=false \
    -Dxorg=true -Dxfbdev=true \
    -Dglx=false -Dglamor=false -Ddri1=false -Ddri2=false -Ddri3=false -Dudev=false -Dsystemd_logind=false
then
    echo "==> meson compile (physical DDXes)"
    if meson compile -C _build_phys; then phys_ok=1; fi
fi
if [ "$phys_ok" = 1 ]; then
    echo "==> RESULT: physical DDXes (Xorg + fbdev) BUILD on GNU/Hurd 🎉"
else
    echo "::warning::physical DDXes (Xorg/fbdev) do not yet build on GNU/Hurd — see the errors above (non-fatal; the Xvfb/Xnest baseline still passes)"
fi
