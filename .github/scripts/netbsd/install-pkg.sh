#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net>

set -ex

. .github/scripts/util.sh

export PATH="$PATH:/usr/sbin:/sbin:/usr/local/sbin"

NETBSD_RELEASE="11.0"
NETBSD_ARCH="amd64"
PKGSRC_ARCH="x86_64"

# Install pkgin if not present
if ! command -v pkgin >/dev/null 2>&1; then
    echo "Installing pkgin..."
    MIRRORS="
    https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    "
    for mirror in $MIRRORS; do
        echo "Trying pkgin from $mirror"
        export PKG_PATH="$mirror"
        if pkg_add -v pkgin; then
            echo "pkgin installed from $mirror"
            break
        fi
    done
    if ! command -v pkgin >/dev/null 2>&1; then
        echo "Failed to install pkgin"
        exit 1
    fi
fi

# Configure pkgin repositories (use .conf extension)
rm -f /usr/pkg/etc/pkgin/repositories.conf
rm -f /etc/pkgin/repositories.conf
mkdir -p /usr/pkg/etc/pkgin
mkdir -p /etc/pkgin

# pkgin fails as a whole when one repository's summary cannot be fetched,
# so list only mirrors that serve it. Base ftp(1) speaks https.
reachable_repos() {
    for base in https://ftp.netbsd.org https://cdn.netbsd.org; do
        url="$base/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/$1/All"
        if ftp -o /dev/null -q 30 "$url/pkg_summary.gz" >/dev/null 2>&1; then
            echo "$url"
        fi
    done
}

repos=$(reachable_repos "$NETBSD_RELEASE")
if [ -z "$repos" ]; then
    echo "No NetBSD ${NETBSD_RELEASE} package mirror answered, falling back to NetBSD 10.0 repositories..."
    repos=$(reachable_repos 10.0)
fi
if [ -z "$repos" ]; then
    echo "ERROR: no NetBSD package mirror answered"
    exit 1
fi
echo "$repos" > /usr/pkg/etc/pkgin/repositories.conf
cp /usr/pkg/etc/pkgin/repositories.conf /etc/pkgin/repositories.conf

# Unset PKG_REPOS so pkgin reads repositories.conf (one URL per line)
unset PKG_REPOS

# Update package database
echo "Updating pkgin from:"
cat /usr/pkg/etc/pkgin/repositories.conf
pkgin update

# X11 binary sets
SETS_MIRRORS="
https://ftp.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
https://ftp.us.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
https://cdn.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
"

echo "Downloading and installing X11 sets..."
for i in xbase xetc xfont xcomp xserver; do
    ok=0
    for urlbase in $SETS_MIRRORS; do
        url="$urlbase/$i.tar.xz"
        echo "Fetching $url"
        if ftp -o "/$i.tar.xz" -q 60 "$url"; then
            ok=1
            break
        fi
    done
    if [ $ok -ne 1 ]; then
        echo "ERROR: Failed to download $i.tar.xz"
        exit 1
    fi
    tar --unlink -xJf "/$i.tar.xz" -C /
    rm -f "/$i.tar.xz"
done

# Install build dependencies
echo "Installing build dependencies..."
pkgin -y install \
    bash git pkgconf autoconf automake libtool xorgproto meson pixman xtrans \
    libxkbfile libxcvt libpciaccess font-util libepoll-shim libepoxy nettle \
    xkbcomp xcb-util libXcursor libXScrnSaver spice-protocol fontconfig \
    mkfontscale python311 gmake curl || true

mkdir -p "$X11_BUILD_DIR"
cd "$X11_BUILD_DIR"

export X11_INSTALL_PREFIX=/usr/pkg

build_ac xorg-macros $(fdo_mirror xorg-macros) refs/tags/util-macros-1.20.2
build_ac libxcb-wm   $(fdo_mirror libxcb-wm)   refs/tags/xcb-util-wm-0.4.2
