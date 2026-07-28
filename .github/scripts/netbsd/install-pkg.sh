#!/bin/sh

set -ex

. .github/scripts/util.sh
. .github/scripts/netbsd/mirror-conf.sh

export PATH="$PATH:/usr/sbin:/sbin:/usr/local/sbin"

# NETBSD_RELEASE / NETBSD_ARCH / PKGSRC_ARCH and all mirror URLs + the package
# list come from mirror-conf.sh (shared with the mirror-sync workflow).

PKGIN_CONF_DIRS="/usr/pkg/etc/pkgin /etc/pkgin"

# Write repositories.conf (one URL per line) everywhere pkgin looks for it.
write_repos_conf() {
    for d in $PKGIN_CONF_DIRS; do
        mkdir -p "$d"
        rm -f "$d/repositories.conf"
        for u in "$@"; do
            printf '%s\n' "$u"
        done > "$d/repositories.conf"
    done
    # Unset PKG_REPOS so pkgin reads repositories.conf instead.
    unset PKG_REPOS
}

# Update the pkgin db from the configured repos and install the package set.
# Returns non-zero on any failure so the caller can fall back to another repo
# set. (Invoked in an `if` condition, so `set -e` is suspended for its body.)
pkgin_install_from() {
    write_repos_conf "$@"
    echo "pkgin update from: $*"
    if ! pkgin update; then
        return 1
    fi
    # shellcheck disable=SC2086
    pkgin -y install $PKGIN_PACKAGES
}

# Install pkgin itself if the VM image ships without it. Try the GitHub mirror
# first (its .tgz + deps are mirrored for exactly this), then official.
if ! command -v pkgin >/dev/null 2>&1; then
    echo "Installing pkgin..."
    for mirror in "$MIRROR_BASE_URL" $PKGSRC_REPOS_OFFICIAL; do
        echo "Trying pkgin from $mirror"
        export PKG_PATH="$mirror"
        if pkg_add -v pkgin; then
            echo "pkgin installed from $mirror"
            break
        fi
    done
    unset PKG_PATH
    if ! command -v pkgin >/dev/null 2>&1; then
        echo "Failed to install pkgin"
        exit 1
    fi
fi

# Install build dependencies: try the GitHub-hosted scoped mirror FIRST (so a
# green run never depends on ftp/cdn.netbsd.org being up), and only fall back to
# the official mirrors if our mirror is unreachable, stale, or missing a package
# (e.g. the list above was extended but the mirror has not re-synced yet). This
# is a soft fallback, deliberately NOT a hard cutover.
echo "Installing build dependencies (GitHub mirror first, official fallback)..."
if pkgin_install_from "$MIRROR_BASE_URL"; then
    echo "build dependencies installed from GitHub mirror"
else
    echo "GitHub mirror unavailable/incomplete — falling back to official NetBSD mirrors"
    if ! pkgin_install_from $PKGSRC_REPOS_OFFICIAL; then
        echo "official ${NETBSD_RELEASE} repos had failures — trying 10.0 fallback..."
        write_repos_conf $PKGSRC_REPOS_FALLBACK
        pkgin update || true
        # shellcheck disable=SC2086
        pkgin -y install $PKGIN_PACKAGES || true
    fi
fi

# X11 OS-release binary sets (not pkgsrc). Try the GitHub mirror first, then the
# official NetBSD release mirrors.
echo "Downloading and installing X11 sets (GitHub mirror first)..."
for i in $X11_SETS; do
    ok=0
    for urlbase in "$MIRROR_BASE_URL" $SETS_MIRRORS_OFFICIAL; do
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

# Build the couple of autotools deps that are not packaged / need a pinned rev.
mkdir -p "$X11_BUILD_DIR"
cd "$X11_BUILD_DIR"

export X11_INSTALL_PREFIX=/usr/pkg

build_ac xorg-macros $(fdo_mirror xorg-macros) refs/tags/util-macros-1.20.2
build_ac libxcb-wm   $(fdo_mirror libxcb-wm)   refs/tags/xcb-util-wm-0.4.2
