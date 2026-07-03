#!/usr/bin/env bash
#
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright © 2026 Enrico Weigelt, metux IT consult
#
# Host (ubuntu) half of the NetBSD build-dependency mirror sync (see
# .github/workflows/netbsd-pkg-mirror.yml). Runs AFTER the vmactions VM step has
# staged the mirror into $MIRROR_OUTDIR (copied back to the host). It publishes
# the staged files as assets of a single, stable, non-versioned GitHub Release
# (tag $MIRROR_TAG), replacing assets in place (--clobber) and pruning assets no
# longer part of the set so the release does not grow unbounded.
#
# Requires: gh (authenticated with contents:write on $MIRROR_REPO).

set -euo pipefail

# mirror-conf.sh is POSIX sh; source it here to get MIRROR_REPO / MIRROR_TAG /
# the expected file set without duplicating them.
# shellcheck disable=SC1091
. .github/scripts/netbsd/mirror-conf.sh

MIRROR_OUTDIR="${MIRROR_OUTDIR:-${1:-$PWD/netbsd-mirror-out}}"

if [ ! -d "$MIRROR_OUTDIR" ]; then
    echo "FATAL: staging dir $MIRROR_OUTDIR does not exist (did the VM step run / copy back?)"
    exit 1
fi

# Collect the assets to publish (skip dotfiles / scratch).
assets=()
while IFS= read -r f; do
    assets+=("$f")
done < <(find "$MIRROR_OUTDIR" -maxdepth 1 -type f ! -name '.*' | sort)

if [ "${#assets[@]}" -eq 0 ]; then
    echo "FATAL: no files to publish in $MIRROR_OUTDIR"
    exit 1
fi

# Sanity: the mirror is useless without the index and at least the X11 sets.
if [ ! -f "$MIRROR_OUTDIR/pkg_summary.gz" ]; then
    echo "FATAL: $MIRROR_OUTDIR/pkg_summary.gz missing — refusing to publish a mirror with no index"
    exit 1
fi

echo "publishing ${#assets[@]} assets to ${MIRROR_REPO} release ${MIRROR_TAG}"

# Ensure the release exists (stable tag; created once, reused thereafter).
if ! gh release view "$MIRROR_TAG" --repo "$MIRROR_REPO" >/dev/null 2>&1; then
    echo "creating release $MIRROR_TAG"
    gh release create "$MIRROR_TAG" \
        --repo "$MIRROR_REPO" \
        --title "NetBSD CI build-dependency mirror" \
        --notes "Scoped mirror of the pkgsrc packages + X11 OS-release binary sets consumed by the \`xserver-build-netbsd\` CI lane, so the lane does not depend on ftp/cdn.netbsd.org being reachable. Auto-refreshed by \`.github/workflows/netbsd-pkg-mirror.yml\`; do not edit assets by hand. Layout is a flat pkgin repository base (pkg_summary.gz + <FILE_NAME>.tgz) plus <set>.tar.xz." \
        --latest=false
fi

# Upload (replace in place).
echo "uploading assets..."
gh release upload "$MIRROR_TAG" --repo "$MIRROR_REPO" --clobber "${assets[@]}"

# Prune assets that are no longer part of the current set.
declare -A want=()
for a in "${assets[@]}"; do
    want["$(basename "$a")"]=1
done

echo "pruning stale assets..."
while IFS= read -r existing; do
    [ -n "$existing" ] || continue
    if [ -z "${want[$existing]:-}" ]; then
        echo "  deleting stale asset: $existing"
        gh release delete-asset "$MIRROR_TAG" "$existing" --repo "$MIRROR_REPO" -y || echo "    (delete failed for $existing)"
    fi
done < <(gh release view "$MIRROR_TAG" --repo "$MIRROR_REPO" --json assets --jq '.assets[].name')

echo "done. current mirror assets:"
gh release view "$MIRROR_TAG" --repo "$MIRROR_REPO" --json assets \
    --jq '.assets[] | "  \(.name)\t\(.size) bytes"'
