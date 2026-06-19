#!/bin/sh

set -ex

. .github/scripts/util.sh

export PATH="$PATH:/usr/sbin:/sbin:/usr/local/sbin"

NETBSD_RELEASE="10.1"
NETBSD_ARCH="amd64"
PKGSRC_ARCH="x86_64"

# Install pkgin if not present
if ! command -v pkgin >/dev/null 2>&1; then
    echo "Installing pkgin..."
    MIRRORS="
    https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    https://ftp.fr.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    https://mirrorservice.org/sites/ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    https://mirror.planetunix.net/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
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

{
cat <<EOF
https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
https://ftp.fr.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
https://mirrorservice.org/sites/ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
https://mirror.planetunix.net/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
EOF
    } > /usr/pkg/etc/pkgin/repositories.conf
cp /usr/pkg/etc/pkgin/repositories.conf /etc/pkgin/repositories.conf

# Unset PKG_REPOS so pkgin reads repositories.conf (one URL per line)
unset PKG_REPOS

# Update package database
# pkgin exits non-zero if ANY mirror fails (e.g. "Connection refused"),
# even if at least one mirror succeeds. We use || true to tolerate this.
echo "Updating pkgin..."
if ! pkgin update; then
    echo "pkgin update had partial mirror failures, falling back to NetBSD 10.0 repositories..."
    {
    cat <<EOF
https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/All
https://ftp.fr.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/All
https://mirrorservice.org/sites/ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/All
https://mirror.planetunix.net/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/All
https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/All
EOF
    } > /usr/pkg/etc/pkgin/repositories.conf
    cp /usr/pkg/etc/pkgin/repositories.conf /etc/pkgin/repositories.conf
    pkgin update || true
fi

# Install curl for downloading sets
echo "Installing curl..."
pkgin -y install curl

# X11 binary sets
SETS_MIRRORS="
https://ftp.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
https://ftp.fr.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
https://ftp.us.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
https://cdn.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
"

echo "Downloading and installing X11 sets..."
for i in xbase xetc xfont xcomp xserver; do
    ok=0
    for urlbase in $SETS_MIRRORS; do
        url="$urlbase/$i.tar.xz"
        echo "Fetching $url"
        if curl -L --retry 3 --connect-timeout 20 -f -o "/$i.tar.xz" "$url"; then
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
    mkfontscale python311 gmake curl

mkdir -p "$X11_BUILD_DIR"
cd "$X11_BUILD_DIR"

export X11_INSTALL_PREFIX=/usr/pkg

build_ac xorg-macros $(fdo_mirror xorg-macros) refs/tags/util-macros-1.20.2
build_ac libxcb-wm   $(fdo_mirror libxcb-wm)   refs/tags/xcb-util-wm-0.4.2
