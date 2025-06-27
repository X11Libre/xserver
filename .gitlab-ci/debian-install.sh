#!/bin/bash

set -e
set -o xtrace

# Packages which are needed by this script, but not for the xserver build
EPHEMERAL="
	libcairo2-dev
	libexpat-dev
	libgles2-mesa-dev
	libxkbcommon-dev
	x11-utils
	x11-xserver-utils
	xauth
	xvfb
	"

# Add bullseye-backports for the newer linux-libc-dev & meson packages
echo 'deb http://deb.debian.org/debian bullseye-backports main' >> /etc/apt/sources.list
apt update

apt-get install -y \
	$EPHEMERAL \
	autoconf \
	automake \
	bison \
	build-essential \
	ca-certificates \
	ccache \
	dpkg-dev \
	flex \
	gcc-mingw-w64-i686 \
	git \
	libaudit-dev \
	libbsd-dev \
	libcairo2 \
	libcairo2-dev \
	libdbus-1-dev \
	libdrm-dev \
	libegl1-mesa-dev \
	libepoxy-dev \
	libevdev2 \
	libexpat1 \
	libffi-dev \
	libgbm-dev \
	libgcrypt-dev \
	libgl1-mesa-dev \
	libgles2 \
	libglx-mesa0 \
	libinput10 \
	libinput-dev \
	libpango1.0-0 \
	libpango1.0-dev \
	libpciaccess-dev \
	libpixman-1-dev \
	libselinux1-dev \
	libspice-protocol-dev \
	libsystemd-dev \
	libtool \
	libudev-dev \
	libunwind-dev \
	libx11-dev \
	libx11-xcb-dev \
	libxau-dev \
	libxaw7-dev \
	libxcb-damage0-dev \
	libxcb-dri2-0-dev \
	libxcb-dri3-dev \
	libxcb-glx0-dev \
	libxcb-icccm4-dev \
	libxcb-image0-dev \
	libxcb-keysyms1-dev \
	libxcb-randr0-dev \
	libxcb-render-util0-dev \
	libxcb-render0-dev \
	libxcb-shape0-dev \
	libxcb-shm0-dev \
	libxcb-sync-dev \
	libxcb-util0-dev \
	libxcb-xf86dri0-dev \
	libxcb-xinput-dev \
	libxcb-xkb-dev \
	libxcb-xv0-dev \
	libxcb1-dev \
	libxcursor-dev \
	libxdamage-dev \
	libxdmcp-dev \
	libxext-dev \
	libxfixes-dev \
	libxfont-dev \
	libxi-dev \
	libxinerama-dev \
	libxkbcommon0 \
	libxkbfile-dev \
	libxmu-dev \
	libxmuu-dev \
	libxpm-dev \
	libxrandr-dev \
	libxrender-dev \
	libxres-dev \
	libxshmfence-dev \
	libxss-dev \
	libxt-dev \
	libxtst-dev \
	libxv-dev \
	libxvmc-dev \
	libxxf86vm-dev \
	libz-mingw-w64-dev \
	linux-libc-dev/bullseye-backports \
	mesa-common-dev \
	meson/bullseye-backports \
	mingw-w64-tools \
	nettle-dev \
	pkg-config \
	python3-attr \
	python3-jinja2 \
	python3-mako \
	python3-numpy \
	python3-six \
	valgrind \
	weston \
	x11-xkb-utils \
	xfonts-utils \
	xkb-data \
	xutils-dev

.gitlab-ci/cross-prereqs-build.sh i686-w64-mingw32

cd /root

# drm 2.4.116 for drmSyncobjEventfd
git clone https://gitlab.freedesktop.org/mesa/drm --depth 1 --branch=libdrm-2.4.121
cd drm
meson _build
ninja -C _build -j${FDO_CI_CONCURRENT:-4} install
cd ..
rm -rf drm

# xserver requires libxcvt
git clone https://gitlab.freedesktop.org/xorg/lib/libxcvt.git --depth 1 --branch=libxcvt-0.1.0
cd libxcvt
meson _build
ninja -C _build -j${FDO_CI_CONCURRENT:-4} install
cd ..
rm -rf libxcvt

# xserver requires xorgproto >= 2024.1
git clone https://gitlab.freedesktop.org/xorg/proto/xorgproto.git --depth 1 --branch=xorgproto-2024.1
pushd xorgproto
./autogen.sh
make -j${FDO_CI_CONCURRENT:-4} install
popd
rm -rf xorgproto

git clone https://gitlab.freedesktop.org/mesa/piglit.git
cd piglit
git checkout 265896c86f90cb72e8f218ba6a3617fca8b9a1e3
cd ..

git clone https://gitlab.freedesktop.org/xorg/test/xts
cd xts
git checkout 12a887c2c72c4258962b56ced7b0aec782f1ffed
./autogen.sh
xvfb-run make -j${FDO_CI_CONCURRENT:-4}
cd ..

git clone https://gitlab.freedesktop.org/xorg/test/rendercheck
cd rendercheck
git checkout 67a820621b1475ebfcf3d4f9d7f03a5fc3b9769a
meson build
ninja -j${FDO_CI_CONCURRENT:-4} -C build install
cd ..

rm -rf piglit/.git xts/.git piglit/tests/spec/ rendercheck/

echo '[xts]' > piglit/piglit.conf
echo 'path=/root/xts' >> piglit/piglit.conf

find -name \*.a -o -name \*.o -o -name \*.c -o -name \*.h -o -name \*.la\* | xargs rm
strip xts/xts5/*/.libs/*

apt-get purge -y \
	$EPHEMERAL

apt-get autoremove -y --purge
