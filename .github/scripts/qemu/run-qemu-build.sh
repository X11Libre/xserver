#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net>
#
# Native (non-cross) xserver build for a foreign CPU architecture via
# QEMU user-mode emulation.
#
# Instead of a cross toolchain, we debootstrap a Debian rootfs for the target
# architecture and run the build *inside* it with qemu-user-static + binfmt_misc.
# The compiler is the real target-arch gcc, so arch-specific inline asm (e.g.
# include/compiler.h, include/xlibre_membarrier.h) is genuinely assembled —
# which the x86_64-host lanes never exercise. This is what the
# __asm__ "missing trailing ;" bugs in PR #3484 / #3485 needed.
#
# Usage:
#   run-qemu-build.sh <arch> <qemu-static> <suite> <mirror> [--run-test]
#
#   arch         Debian architecture name (e.g. ppc64el, armhf, mipsel, sparc64, alpha)
#   qemu-static  qemu user-mode static binary (e.g. qemu-ppc64le-static)
#   suite        Debian suite (e.g. bookworm, sid)
#   mirror       Debian mirror base URL (debootstrap --foreign source)
#   --run-test   also run `meson test` inside the rootfs (best-effort)
#
# Environment:
#   MESON_ARGS, MESON_TEST_ARGS, FDO_CI_CONCURRENT  (passed into the rootfs)
#
# Run as root (workflow uses `sudo`). The runner's own OS stays untouched:
# only qemu-user-static/binfmt-support/debootstrap get installed on it.

set -e
set -x  # Debug: print every command

ARCH="$1"
QEMU="$2"
SUITE="$3"
MIRROR="$4"
shift 4

RUN_TEST=0
if [ "$1" = "--run-test" ]; then
    RUN_TEST=1
    shift
fi

if [ -z "$ARCH" ] || [ -z "$QEMU" ] || [ -z "$SUITE" ] || [ -z "$MIRROR" ]; then
    echo "usage: $0 <arch> <qemu-static> <suite> <mirror> [--run-test]" >&2
    exit 1
fi

ROOTFS=/tmp/qemu-rootfs-$ARCH

echo "=== [$(date)] Starting QEMU build for $ARCH ($QEMU, $SUITE, $MIRROR) ==="

# --- host side: prepare the emulator + debootstrap -------------------------
echo "=== [$(date)] Installing host packages ==="
apt-get update
apt-get install -y \
    qemu-user-static \
    binfmt-support \
    debootstrap \
    wget

# debian-ports (sparc64/alpha) needs the ports archive keyring. The one
# shipped by the runner's distro (Ubuntu) is routinely out of date and misses
# the current ports signing key, so fetch the keyring from the ports archive
# itself. The official Debian ports (ppc64el/armhf/mipsel) use the regular
# archive keyring, which debootstrap resolves automatically from the mirror URL.
case "$MIRROR" in
    *debian-ports*)
        echo "=== [$(date)] Fetching debian-ports keyring ==="
        PORTS_KEYRING_DEB=$(wget -qO- "http://deb.debian.org/debian-ports/pool/main/d/debian-ports-archive-keyring/" \
            | grep -o 'debian-ports-archive-keyring_[0-9.]*_all.deb' | sort -V | tail -1)
        if [ -z "$PORTS_KEYRING_DEB" ]; then
            echo "ERROR: could not resolve debian-ports-archive-keyring version" >&2
            exit 1
        fi
        echo "=== [$(date)] Downloading $PORTS_KEYRING_DEB ==="
        wget -qO "/tmp/$PORTS_KEYRING_DEB" "http://deb.debian.org/debian-ports/pool/main/d/debian-ports-archive-keyring/$PORTS_KEYRING_DEB"
        rm -rf /tmp/ports-keyring
        dpkg-deb -x "/tmp/$PORTS_KEYRING_DEB" /tmp/ports-keyring
        DEBOOTSTRAP_KEYRING="--keyring=/tmp/ports-keyring/usr/share/keyrings/debian-ports-archive-keyring.gpg"
        echo "=== [$(date)] debian-ports keyring installed ==="
    ;;
esac

# --- create the foreign-arch rootfs (--foreign: can't run target tools yet) --
echo "=== [$(date)] Creating rootfs for $ARCH ==="
rm -rf "$ROOTFS"
mkdir -p "$ROOTFS"

echo "=== [$(date)] Running debootstrap --foreign ==="
debootstrap \
    --foreign \
    --arch="$ARCH" \
    ${DEBOOTSTRAP_KEYRING:-} \
    --variant=minbase \
    "$SUITE" \
    "$ROOTFS" \
    "$MIRROR"

# --- register the emulator inside the rootfs and finish the bootstrap -------
echo "=== [$(date)] Copying qemu binary and running --second-stage ==="
cp "/usr/bin/$QEMU" "$ROOTFS/usr/bin/"
chroot "$ROOTFS" /debootstrap/debootstrap --second-stage

# Copy debian-ports keyring into chroot for apt verification. apt only trusts
# keyrings it finds in /etc/apt/trusted.gpg.d/ (or referenced via signed-by= in
# sources.list); placing it only under /usr/share/keyrings/ leaves apt without
# the ports signing key, so `apt-get update` fails with NO_PUBKEY and the index
# stays stale (e.g. m4 unresolved).
case "$MIRROR" in
    *debian-ports*)
        mkdir -p "$ROOTFS/usr/share/keyrings" "$ROOTFS/etc/apt/trusted.gpg.d"
        cp /tmp/ports-keyring/usr/share/keyrings/debian-ports-archive-keyring.gpg "$ROOTFS/usr/share/keyrings/"
        cp /tmp/ports-keyring/usr/share/keyrings/debian-ports-archive-keyring.gpg "$ROOTFS/etc/apt/trusted.gpg.d/"
    ;;
esac

echo "=== [$(date)] Bootstrap complete ==="

# --- copy the source tree into the rootfs -----------------------------------
echo "=== [$(date)] Copying source tree ==="
mkdir -p "$ROOTFS/src"
cp -a . "$ROOTFS/src/"

# --- chroot-side scripts ------------------------------------------------------
echo "=== [$(date)] Creating install-prereq.sh ==="
cat > "$ROOTFS/install-prereq.sh" <<'INSTALL_PREREQ'
#!/bin/bash
set -e
set -x
export DEBIAN_FRONTEND=noninteractive

# libunwind-dev not available in debian-ports/unstable for alpha/sparc64
# ARCH is passed via env from outer script
case "$ARCH" in
    alpha|sparc64)
        UNWIND_PKG=""
        ;;
    *)
        UNWIND_PKG="libunwind-dev"
        ;;
esac

echo "=== [$(date)] Updating apt in chroot ==="
apt-get update

# --no-install-recommends: dpkg-dev 1.23.7+ Recommends an OpenPGP tool
# ("sq | sqop | rsop | gosop | pgpainless-cli | gpg-sq | gnupg") for package
# signature verification. On debian-ports/alpha apt picks pgpainless-cli, a
# Java (BouncyCastle) implementation, dragging in default-jre-headless ->
# openjdk-25-jre-headless, whose postinst fails in an unmounted chroot.
# None of these are needed to build the xserver, so skip Recommends entirely.
echo "=== [$(date)] Installing build dependencies ==="
apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    build-essential \
    bison \
    ca-certificates \
    flex \
    wget \
    libaudit-dev \
    libbsd-dev \
    libcairo2-dev \
    libdbus-1-dev \
    libdrm-dev \
    libegl1-mesa-dev \
    libepoxy-dev \
    libevdev-dev \
    libffi-dev \
    libgbm-dev \
    libgcrypt-dev \
    libgl1-mesa-dev \
    libinput-dev \
    libpciaccess-dev \
    libpixman-1-dev \
    libspice-protocol-dev \
    libsystemd-dev \
    libtool \
    libudev-dev \
    ${UNWIND_PKG} \
    libx11-dev \
    libx11-xcb-dev \
    libxau-dev \
    libxaw7-dev \
    libxcb-glx0-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxcb-render0-dev \
    libxcb-shape0-dev \
    libxcb-shm0-dev \
    libxcb-util0-dev \
    libxcb-xf86dri0-dev \
    libxcb-xkb-dev \
    libxcb-xv0-dev \
    libxcb1-dev \
    libxcvt-dev \
    libxdmcp-dev \
    libxext-dev \
    libxfixes-dev \
    libxfont-dev \
    libxi-dev \
    libxinerama-dev \
    libxkbfile-dev \
    libxmu-dev \
    libxmuu-dev \
    libxpm-dev \
    libxrandr-dev \
    libxrender-dev \
    libxres-dev \
    libxshmfence-dev \
    libxt-dev \
    libxtst-dev \
    libxv-dev \
    libpango1.0-dev \
    mesa-common-dev \
    meson \
    ninja-build \
    pkg-config \
    python3-mako \
    x11-xkb-utils \
    x11proto-dev \
    xfonts-utils \
    xkb-data \
    xutils-dev \
    xauth \
    xvfb

# xorgproto: Debian's x11proto-dev (2022.1 in bookworm) ships presentproto 1.2,
# but the xserver requires >= 1.4. Build the current xorgproto into a private
# prefix (same approach as the ubuntu lane) and prepend it to PKG_CONFIG_PATH.
# Needs only meson/ninja/python3, all of which are already installed above.
# We fetch the source via wget tarball instead of git clone: git carries a
# hard dependency on git-man, which is currently unresolved in
# debian-ports/unstable (git:alpha=1:2.53.0-1 vs git-man <1:2.53.0-.) — the
# apt solver aborts on it and drags down every other build dep with it.
echo "=== [$(date)] Building xorgproto from source ==="
mkdir -p /opt/xorgproto
cd /opt/xorgproto
wget -qO xorgproto.tar.gz \
    "https://github.com/X11Libre/mirror.fdo.xorgproto/archive/refs/tags/xorgproto-2024.1.tar.gz"
mkdir xorgproto-src
tar -xzf xorgproto.tar.gz -C xorgproto-src --strip-components=1
cd xorgproto-src
meson setup build -Dprefix=/opt/xorgproto
meson compile -C build
meson install -C build
echo "=== [$(date)] xorgproto build complete ==="
INSTALL_PREREQ
chmod +x "$ROOTFS/install-prereq.sh"

echo "=== [$(date)] Creating run-build.sh ==="
cat > "$ROOTFS/run-build.sh" <<RUN_BUILD
#!/bin/bash
set -e
set -x

cd /src

export MESON_BUILDDIR=_build
export FDO_CI_CONCURRENT=\${FDO_CI_CONCURRENT:-2}
export PKG_CONFIG_PATH="/opt/xorgproto/lib/pkgconfig:/opt/xorgproto/share/pkgconfig:\${PKG_CONFIG_PATH:-}"

if [ -z "\$MESON_ARGS" ]; then
    MESON_ARGS="-Dprefix=/usr -Dwerror=true -Dxorg=true -Dxvfb=true -Dxnest=true -Dxephyr=true -Dxfbdev=false"
fi

echo "=== [$(date)] meson setup ==="
rm -rf "\$MESON_BUILDDIR"
meson setup "\$MESON_BUILDDIR" \$MESON_ARGS

echo "=== [$(date)] meson configure ==="
meson configure "\$MESON_BUILDDIR"

echo "=== [$(date)] meson compile ==="
meson compile -v -C "\$MESON_BUILDDIR" -j\$FDO_CI_CONCURRENT

RUN_BUILD
if [ "$RUN_TEST" = "1" ]; then
cat >> "$ROOTFS/run-build.sh" <<'RUN_TEST_SH'

echo "=== [$(date)] meson test (best-effort, emulation can be slow/flaky) ==="
if ! meson test -C "\$MESON_BUILDDIR" --print-errorlogs \${MESON_TEST_ARGS} --no-rebuild; then
    echo "WARNING: meson test failed (QEMU user-mode flakiness); build itself succeeded" >&2
fi
RUN_TEST_SH
fi
cat >> "$ROOTFS/run-build.sh" <<'RUN_BUILD_END'
echo "=== [$(date)] build done ==="
RUN_BUILD_END
chmod +x "$ROOTFS/run-build.sh"

# --- run the build inside the rootfs -----------------------------------------
# Bind-mount the host's /proc, /sys and /dev into the rootfs. Several package
# postinst maintainer scripts (e.g. openjdk-25-jre-headless on alpha, which
# errors out with "the java command requires a mounted proc fs (/proc)" when
# it's missing) expect a mounted procfs/sysfs inside a chroot. debootstrap
# --second-stage only populates the skeleton, it doesn't mount anything.
echo "=== [$(date)] Mounting /proc, /sys and /dev into rootfs ==="
for m in /proc /sys /dev; do
    mkdir -p "$ROOTFS$m"
    mount --bind "$m" "$ROOTFS$m"
done

echo "=== [$(date)] Running install-prereq.sh in chroot ==="
ARCH="$ARCH" chroot "$ROOTFS" /install-prereq.sh
echo "=== [$(date)] Running run-build.sh in chroot ==="
chroot "$ROOTFS" /run-build.sh

echo "=== [$(date)] Unmounting rootfs ==="
for m in /proc /sys /dev; do
    umount "$ROOTFS$m"
done

echo "=== [$(date)] QEMU build for '$ARCH' succeeded ==="