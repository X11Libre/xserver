#!/bin/sh

set -ex

. .github/scripts/util.sh

export PATH="$PATH:/usr/sbin:/sbin:/usr/local/sbin"

# NetBSD release version (matches .github/workflows/build-xserver.yml:363)
NETBSD_RELEASE="10.1"
# CPU architecture: NetBSD uses amd64, pkgsrc uses x86_64 for the directory name
NETBSD_ARCH="amd64"
PKGSRC_ARCH="x86_64"

# Install pkgin if not already present
if ! command -v pkgin >/dev/null 2>&1; then
    echo "pkgin not found, installing..."
    # Try multiple mirrors sequentially for pkg_add (PKG_PATH is single path, not colon-separated)
    MIRRORS="
    https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    https://ftp.fr.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    https://mirrorservice.org/sites/ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    https://mirror.planetunix.net/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
    "
    for mirror in $MIRRORS; do
        echo "Trying to install pkgin from $mirror"
        export PKG_PATH="$mirror"
        if pkg_add -v pkgin; then
            echo "Successfully installed pkgin from $mirror"
            break
        else
            echo "Failed to install pkgin from $mirror"
        fi
    done
    if ! command -v pkgin >/dev/null 2>&1; then
        echo "ERROR: Could not install pkgin from any mirror"
        exit 1
    fi
else
    echo "pkgin already installed, skipping pkg_add"
fi

# Remove any default pkgin config that might point to wrong release
rm -f /usr/pkg/etc/pkgin/repositories.conf
rm -f /etc/pkgin/repositories.conf

# Ensure pkgin config directories exist
mkdir -p /usr/pkg/etc/pkgin
mkdir -p /etc/pkgin

# Write repositories file in both possible locations (these are for pkgsrc packages)
{
cat <<EOF
https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/
https://ftp.fr.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/
https://mirrorservice.org/sites/ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/
https://mirror.planetunix.net/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/
https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/
EOF
} > /usr/pkg/etc/pkgin/repositories
cp /usr/pkg/etc/pkgin/repositories /etc/pkgin/repositories

echo "Created pkgin repositories file:"
cat /usr/pkg/etc/pkgin/repositories

# Also set environment variable to force pkgin to use these mirrors (colon-separated)
export PKGIN_REPOSITORIES="https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/:https://ftp.fr.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/:https://mirrorservice.org/sites/ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/:https://mirror.planetunix.net/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/:https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/"
echo "PKGIN_REPOSITORIES=$PKGIN_REPOSITORIES"

# Download and install X11 binary sets FIRST (needed for build dependencies)
# Multiple mirrors for binary sets (cdn.netbsd.org is last as it's frequently failing)
SETS_MIRRORS="
https://ftp.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
https://ftp.fr.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
https://ftp.us.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
https://cdn.netbsd.org/pub/NetBSD/NetBSD-$NETBSD_RELEASE/$NETBSD_ARCH/binary/sets
"

echo "--> Downloading and installing X11 sets (xbase, xetc, xfont, xcomp, xserver)"
for i in xbase xetc xfont xcomp xserver; do
    success=0
    for mirror in $SETS_MIRRORS; do
        url="$mirror/$i.tar.xz"
        echo "Attempting to download $url"
        # Use curl with retries and timeouts
        if curl --retry 3 --connect-timeout 20 -fSL "$url" -o "/$i.tar.xz"; then
            echo "Downloaded $i from $mirror"
            success=1
            break
        else
            echo "Failed to download from $mirror"
            rm -f "/$i.tar.xz"
        fi
    done
    if [ $success -ne 1 ]; then
        echo "ERROR: Could not download $i.tar.xz from any mirror"
        exit 1
    fi
    echo "unpacking /$i.tar.xz"
    tar --unlink -xJf "/$i.tar.xz" -C /
    rm -f "/$i.tar.xz"
done

# Now update package database (pkgsrc) - this may use fallback to 10.0 if 10.1 repos unavailable
echo "--> Updating pkgin package database"
if ! pkgin update; then
    echo "pkgin update failed for NetBSD $NETBSD_RELEASE, falling back to 10.0 packages (ABI compatible)..."
    # Override with 10.0 mirrors
    {
    cat <<EOF
https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/
https://ftp.fr.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/
https://mirrorservice.org/sites/ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/
https://mirror.planetunix.net/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/
https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/
EOF
    } > /usr/pkg/etc/pkgin/repositories
    cp /usr/pkg/etc/pkgin/repositories /etc/pkgin/repositories
    export PKGIN_REPOSITORIES="https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/:https://ftp.fr.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/:https://mirrorservice.org/sites/ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/:https://mirror.planetunix.net/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/:https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/"
    echo "Using fallback repositories (10.0):"
    cat /usr/pkg/etc/pkgin/repositories
    pkgin update
fi

echo "--> installing extra dependencies"
pkgin -y install \
    bash git pkgconf autoconf automake libtool xorgproto meson pixman xtrans \
    libxkbfile libxcvt libpciaccess font-util libepoll-shim libepoxy nettle \
    xkbcomp xcb-util libXcursor libXScrnSaver spice-protocol fontconfig \
    mkfontscale python311 gmake curl

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

export X11_INSTALL_PREFIX=/usr/pkg

build_ac xorg-macros $(fdo_mirror xorg-macros) refs/tags/util-macros-1.20.2
build_ac libxcb-wm   $(fdo_mirror libxcb-wm)   refs/tags/xcb-util-wm-0.4.2
