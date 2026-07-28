#!/bin/sh
#
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright © 2026 Enrico Weigelt, metux IT consult
#
# Shared configuration for the NetBSD CI build-dependency mirror.
#
# The `xserver-build-netbsd` CI lane installs its build deps from the official
# NetBSD pkgsrc / OS-release mirrors (ftp.netbsd.org, cdn.netbsd.org). Those
# servers flake intermittently (e.g. `pkg_add: ... Undefined error: 0`), which
# reds an otherwise-green PR for reasons unrelated to its diff. To insulate the
# lane we host a SCOPED mirror — just the packages + X11 sets this job actually
# needs (~230 MB), not the full ~65 GB pkgsrc archive — as GitHub Release
# assets, refreshed weekly by `.github/workflows/netbsd-pkg-mirror.yml`.
#
# This file is the single source of truth sourced by BOTH consumers, so the
# mirror and the build job can never drift on which packages / release / arch
# they mean:
#   - .github/scripts/netbsd/install-pkg.sh   (build job: install from mirror,
#                                               fall back to official)
#   - .github/scripts/netbsd/sync-pkg-mirror.sh (weekly sync: resolve+download
#                                               the closure, build the mirror)
#
# POSIX sh — sourced inside the NetBSD VM (usesh: true). No bashisms.

# --- target NetBSD release / architecture -----------------------------------
# Must match the `release:` of the vmactions/netbsd-vm step in
# build-xserver.yml (and the sync workflow).
NETBSD_RELEASE="10.1"
NETBSD_ARCH="amd64"      # OS-release binary-set arch (X11 sets path)
PKGSRC_ARCH="x86_64"     # pkgsrc package arch

# --- GitHub-hosted scoped mirror --------------------------------------------
# Release assets are served flat under the tag, which is exactly the layout
# pkgin expects for a repository base URL (pkg_summary.gz + <FILE_NAME> by
# relative name). The tag is stable/non-versioned; assets are replaced in place
# each sync (gh release upload --clobber).
MIRROR_REPO="X11Libre/xserver"
MIRROR_TAG="netbsd-pkgsrc-mirror"
MIRROR_BASE_URL="https://github.com/${MIRROR_REPO}/releases/download/${MIRROR_TAG}"

# --- official upstream mirrors (fallback for the build job; source for sync) -
# NOTE: the `<rel>/All` path 302-redirects to a dated quarterly snapshot
# (e.g. 10.0_2026Q1/All); curl -L / pkgin follow it dynamically — do NOT
# hardcode the dated path here, it moves over time.
PKGSRC_REPOS_OFFICIAL="\
https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All
https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/${NETBSD_RELEASE}/All"

# Last-ditch fallback if the target release's repo is unreachable entirely.
PKGSRC_REPOS_FALLBACK="\
https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/All
https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/${PKGSRC_ARCH}/10.0/All"

# OS-release X11 binary sets (NOT pkgsrc — NetBSD's own release sets).
SETS_MIRRORS_OFFICIAL="\
https://cdn.netbsd.org/pub/NetBSD/NetBSD-${NETBSD_RELEASE}/${NETBSD_ARCH}/binary/sets
https://ftp.netbsd.org/pub/NetBSD/NetBSD-${NETBSD_RELEASE}/${NETBSD_ARCH}/binary/sets
https://ftp.us.netbsd.org/pub/NetBSD/NetBSD-${NETBSD_RELEASE}/${NETBSD_ARCH}/binary/sets"

# --- the actual dependency set ----------------------------------------------
# X11 OS-release binary sets fetched as <name>.tar.xz and extracted into /.
X11_SETS="xbase xetc xfont xcomp xserver"

# pkgin packages the build needs. KEEP THIS IN SYNC with what the build
# actually requires — this is the list both install-pkg.sh installs and
# sync-pkg-mirror.sh resolves+mirrors. Editing it is safe: the next mirror
# sync picks it up; until then the build job just falls back to official for
# any not-yet-mirrored package.
#
# `pkgin` itself is listed so its .tgz (+ deps) are mirrored for the pkg_add
# bootstrap in install-pkg.sh when the VM image ships without it.
PKGIN_PACKAGES="\
pkgin \
bash git pkgconf autoconf automake libtool xorgproto meson pixman xtrans \
libxkbfile libxcvt libpciaccess font-util libepoll-shim libepoxy nettle \
xkbcomp xcb-util libXcursor libXScrnSaver spice-protocol fontconfig \
mkfontscale python311 gmake curl"
