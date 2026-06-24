#!/bin/sh
#
# Install xserver build dependencies on OpenIndiana (illumos), via IPS (`pkg`).
#
# OpenIndiana (unlike server-only OmniOS) is a desktop-capable illumos distro
# and packages the full X stack — every X library, the protocol headers, Mesa
# and the xcb-util family — under the single `openindiana.org` publisher in the
# standard /usr layout. So everything is installable; nothing has to be built
# from source here, and GLX can stay enabled (Mesa is packaged).
#
# FMRIs verified against the OpenIndiana Hipster catalog.
#

set -ex

PATH="/usr/bin:/usr/sbin:/sbin:$PATH"
export PATH

pkg refresh || true

# Build toolchain — MUST succeed. `pkg install` is transactional, so a single
# unknown/ambiguous FMRI aborts the whole set; keeping this its own step
# *without* `|| true` makes a bad FMRI fail loudly (and named) instead of
# silently leaving us without a compiler/meson.
pkg install -v --accept \
    developer/gcc-14 \
    developer/build/meson \
    developer/build/ninja \
    developer/build/pkg-config

# Runtime + X libraries the server links against. Best-effort, one at a time, so
# a single missing/misnamed FMRI only skips that package (and is named in the
# log) rather than aborting the batch; meson will flag anything genuinely
# required and absent.
for pkg_fmri in \
    runtime/python-313 \
    library/zlib \
    library/graphics/pixman \
    system/library/freetype-2 \
    x11/header/x11-protocols \
    x11/header/xcb-protocols \
    x11/font-util \
    x11/library/libxau \
    x11/library/libxcb \
    x11/library/libxdmcp \
    x11/library/libxext \
    x11/library/libxfixes \
    x11/library/libxrender \
    x11/library/libxt \
    x11/library/libx11 \
    x11/library/xtrans \
    x11/library/libxkbfile \
    x11/library/libxfont2 \
    x11/library/libfontenc \
    x11/library/libxcvt \
    x11/library/libxshmfence \
    x11/library/libepoxy \
    x11/library/xcb-util \
    x11/library/xcb-util-wm \
    x11/library/mesa
do
    # rc 0 = installed, rc 4 = already present / nothing to do — both fine.
    rc=0
    pkg install -v --accept "$pkg_fmri" || rc=$?
    if [ "$rc" -ne 0 ] && [ "$rc" -ne 4 ]; then
        echo "WARN: 'pkg install $pkg_fmri' failed (rc=$rc) — continuing (meson will flag it if required)"
    fi
done
