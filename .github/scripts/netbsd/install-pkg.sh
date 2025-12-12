#!/bin/sh

set -e
set -v

# Set PKG_PATH for binary packages (adjust arch if not amd64; check with uname -m)
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

#wget $FILESET_URL/base.tar.xz
#wget $FILESET_URL/comp.tar.xz
#wget $FILESET_URL/debug.tar.xz
#wget $FILESET_URL/etc.tar.xz
#wget $FILESET_URL/misc.tar.xz
#wget $FILESET_URL/tests.tar.xz
#wget $FILESET_URL/text.tar.xz
#wget $FILESET_URL/xbase.tar.xz
#wget $FILESET_URL/xcomp.tar.xz
#wget $FILESET_URL/xdebug.tar.xz
#wget $FILESET_URL/xetc.tar.xz
#wget $FILESET_URL/xfont.tar.xz
#wget $FILESET_URL/xserver.tar.xz

for i in xbase xetc xfont xcomp xserver ; do
    echo "downloading $FILESET_URL/$i.tar.xz --> /$i.tar.xz"
    curl $FILESET_URL/$i.tar.xz -o /$i.tar.xz
    tar --unlink -xJf /$i.tar.xz -C /
done

#echo "CHECK_OS_VERSION=no" >> /etc/pkg_install.conf
#echo "CHECK_OSABI=no" >> /etc/pkg_install.conf

#pkg_add -v pkgin
#echo "http://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/amd64/10.1/All/" > /usr/pkg/etc/pkgin/repositories.conf
#pkgin update
#pkgin install x11/libX11
#pkgin install xorgproto x11-links libX11 libXau libXdmcp xcb-proto libxcb

# Use PKG_PATH for direct binary fetch (bypasses repo index)
#export PKG_PATH="http://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/amd64/10.1/All/"
#pkg_add "xorgproto" "x11-links" "x11/libX11" "x11/libXau" "x11/libXdmcp" "x11/xcb-proto" "x11/libxcb"
#pkg_add "pkgin"

#pkgin clean
#rm -f /usr/pkg/db/pkgin.db.*  # Nuke local cache if corrupted
#pkgin update -f  # Force full refresh from mirror

#pkgin -y install python312
#ln -s /usr/pkg/bin/python3.11 /usr/pkg/bin/python3

