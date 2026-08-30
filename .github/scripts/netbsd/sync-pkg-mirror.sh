#!/bin/sh
#
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright © 2026 Enrico Weigelt, metux IT consult
#
# In-VM half of the NetBSD build-dependency mirror sync (see
# .github/workflows/netbsd-pkg-mirror.yml). Runs INSIDE a vmactions/netbsd-vm
# that is the SAME image (release) the xserver-build-netbsd job uses — this is
# deliberate: pkgin's dependency closure is computed against the identical base
# system, so the set of .tgz we mirror is exactly the set the build job would
# otherwise download from the official mirrors. (Packages already present in the
# base image are skipped by pkgin here AND in the build job, so the mirror stays
# consistent with what the build actually fetches.)
#
# It does NOT reimplement dependency resolution — it drives the real pkgin
# download-only path (`pkgin -d install`) to get the authoritative closure, then
# builds a TRIMMED pkg_summary.gz (a strict subset of the official one, so
# FILE_SIZE and file bytes stay consistent) covering only the mirrored packages.
#
# Everything is staged into $MIRROR_OUTDIR (a dir inside the shared workspace),
# so the host side of the workflow can pick it up after the VM step (copyback)
# and publish it via `gh release upload`.
#
# POSIX sh (usesh: true). No bashisms.

set -eu

. .github/scripts/netbsd/mirror-conf.sh

export PATH="$PATH:/usr/sbin:/sbin:/usr/local/sbin"

MIRROR_OUTDIR="${MIRROR_OUTDIR:-$PWD/netbsd-mirror-out}"
PKGIN_CACHE="${PKGIN_CACHE:-/var/db/pkgin/cache}"
PKGIN_CONF_DIRS="/usr/pkg/etc/pkgin /etc/pkgin"

rm -rf "$MIRROR_OUTDIR"
mkdir -p "$MIRROR_OUTDIR"

write_repos_conf() {
    for d in $PKGIN_CONF_DIRS; do
        mkdir -p "$d"
        rm -f "$d/repositories.conf"
        for u in "$@"; do
            printf '%s\n' "$u"
        done > "$d/repositories.conf"
    done
    unset PKG_REPOS
}

# --- bootstrap pkgin from the official mirrors ------------------------------
# The sync tolerates official-mirror flakes (it just re-runs next week); it is
# the build job we insulate, not this one.
if ! command -v pkgin >/dev/null 2>&1; then
    echo "Installing pkgin (from official mirrors)..."
    for mirror in $PKGSRC_REPOS_OFFICIAL; do
        export PKG_PATH="$mirror"
        if pkg_add -v pkgin; then
            break
        fi
    done
    unset PKG_PATH
    command -v pkgin >/dev/null 2>&1 || { echo "FATAL: could not install pkgin"; exit 1; }
fi

# --- resolve + download the closure (download-only) -------------------------
write_repos_conf $PKGSRC_REPOS_OFFICIAL
if ! pkgin update; then
    echo "official ${NETBSD_RELEASE} repos unreachable — trying 10.0 fallback..."
    write_repos_conf $PKGSRC_REPOS_FALLBACK
    pkgin update
fi

mkdir -p "$PKGIN_CACHE"
echo "Downloading package closure (download-only) into $PKGIN_CACHE ..."
# -d = download only (populate the cache, do not install). Resolves the full
# dependency closure of $PKGIN_PACKAGES against the base image.
# shellcheck disable=SC2086
pkgin -y -d install $PKGIN_PACKAGES

tgz_count=$(find "$PKGIN_CACHE" -maxdepth 1 -name '*.tgz' | wc -l | tr -d ' ')
if [ "$tgz_count" -eq 0 ]; then
    echo "FATAL: no .tgz downloaded into $PKGIN_CACHE — nothing to mirror"
    exit 1
fi
echo "downloaded $tgz_count package files"

# Copy the package files into the staging dir and record their names.
KEEP="$MIRROR_OUTDIR/.keep-filenames"
: > "$KEEP"
for f in "$PKGIN_CACHE"/*.tgz; do
    [ -f "$f" ] || continue
    cp -f "$f" "$MIRROR_OUTDIR/"
    basename "$f" >> "$KEEP"
done

# --- build the trimmed pkg_summary.gz ---------------------------------------
# Fetch the FULL official summary from the same (redirect-resolved) repo pkgin
# just used, then keep only the records whose FILE_NAME is one we mirrored. We
# do NOT regenerate records — a strict subset keeps every FILE_SIZE / metadata
# byte-identical to upstream, which is what pkgin verifies against.
FULL_SUMMARY="$MIRROR_OUTDIR/.pkg_summary.full"
got_summary=0
for repo in $PKGSRC_REPOS_OFFICIAL $PKGSRC_REPOS_FALLBACK; do
    for sfx in gz bz2 xz; do
        echo "fetching $repo/pkg_summary.$sfx ..."
        if curl -sL --connect-timeout 25 --max-time 300 -f -o "$MIRROR_OUTDIR/.summary.$sfx" "$repo/pkg_summary.$sfx"; then
            case "$sfx" in
                gz)  gzip  -dc "$MIRROR_OUTDIR/.summary.$sfx" > "$FULL_SUMMARY" ;;
                bz2) bzip2 -dc "$MIRROR_OUTDIR/.summary.$sfx" > "$FULL_SUMMARY" ;;
                xz)  xz    -dc "$MIRROR_OUTDIR/.summary.$sfx" > "$FULL_SUMMARY" ;;
            esac
            rm -f "$MIRROR_OUTDIR/.summary.$sfx"
            got_summary=1
            break
        fi
    done
    [ "$got_summary" -eq 1 ] && break
done
if [ "$got_summary" -ne 1 ]; then
    echo "FATAL: could not fetch pkg_summary from any official mirror"
    exit 1
fi

echo "trimming pkg_summary to the $tgz_count mirrored packages ..."
awk -v keepfile="$KEEP" '
    BEGIN {
        # Read the keep-list under the default RS="\n" (line at a time) BEFORE
        # switching to paragraph mode — otherwise RS="" would slurp the whole
        # keepfile as one key and nothing would match.
        while ((getline l < keepfile) > 0) { keep[l] = 1 }
        RS = ""; FS = "\n";
    }
    {
        fn = "";
        for (i = 1; i <= NF; i++) {
            if ($i ~ /^FILE_NAME=/) { fn = substr($i, 11) }
        }
        if (fn != "" && (fn in keep)) { printf "%s\n\n", $0 }
    }
' "$FULL_SUMMARY" > "$MIRROR_OUTDIR/pkg_summary"

kept=$(grep -c '^PKGNAME=' "$MIRROR_OUTDIR/pkg_summary" || true)
echo "trimmed summary has $kept records (of $tgz_count mirrored files)"
if [ "$kept" -eq 0 ]; then
    echo "FATAL: trimmed pkg_summary is empty — FILE_NAME mismatch?"
    exit 1
fi
gzip -9 -f "$MIRROR_OUTDIR/pkg_summary"      # -> pkg_summary.gz

# --- fetch the X11 OS-release binary sets -----------------------------------
echo "fetching X11 sets ..."
for i in $X11_SETS; do
    ok=0
    for urlbase in $SETS_MIRRORS_OFFICIAL; do
        echo "fetching $urlbase/$i.tar.xz ..."
        if curl -sL --connect-timeout 25 --max-time 600 -f -o "$MIRROR_OUTDIR/$i.tar.xz" "$urlbase/$i.tar.xz"; then
            ok=1
            break
        fi
    done
    if [ "$ok" -ne 1 ]; then
        echo "FATAL: could not fetch X11 set $i.tar.xz"
        exit 1
    fi
done

# --- clean up staging-only scratch files ------------------------------------
rm -f "$FULL_SUMMARY" "$KEEP"

echo "mirror staged in $MIRROR_OUTDIR:"
ls -la "$MIRROR_OUTDIR"
du -sh "$MIRROR_OUTDIR"
