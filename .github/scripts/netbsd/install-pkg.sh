#!/bin/sh

set -e

. .github/scripts/util.sh

export PATH="$PATH:/usr/sbin:/sbin:/usr/local/sbin"

pkg_add -v pkgin
pkgin update

echo "--> install extra dependencies"
pkgin -y install \
    bash git pkgconf autoconf automake libtool xorgproto meson pixman xtrans \
    libxkbfile libxcvt libpciaccess font-util libepoll-shim libepoxy nettle \
    xkbcomp xcb-util libXcursor libXScrnSaver spice-protocol fontconfig \
    mkfontscale python311 gmake curl

FILESET_URL=https://cdn.netbsd.org/pub/NetBSD/NetBSD-10.0/amd64/binary/sets

for i in xbase xetc xfont xcomp xserver ; do
    echo "downloading $FILESET_URL/$i.tar.xz --> /$i.tar.xz"
    curl $FILESET_URL/$i.tar.xz -o /$i.tar.xz
    echo "unpacking /$i.tar.xz"
    tar --unlink -xJf /$i.tar.xz -C /
done

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

export X11_INSTALL_PREFIX=/usr/pkg

build_ac xorg-macros $(fdo_mirror xorg-macros) refs/tags/util-macros-1.20.2
build_ac libxcb-wm   $(fdo_mirror libxcb-wm)   refs/tags/xcb-util-wm-0.4.2
