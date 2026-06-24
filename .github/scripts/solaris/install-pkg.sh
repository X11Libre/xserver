#!/bin/sh
#
# Install xserver build dependencies on OmniOS (illumos), via IPS (`pkg`).
#
# NOTE (first bring-up): the package FMRIs below are a best-effort starting
# point. OmniOS splits packages between the core publisher and the extra
# `ooce` publisher, and not every X dependency is packaged (e.g. libxcvt and a
# recent xorgproto may be missing). On the first CI run, let `meson setup`
# report what is actually missing and adjust this list; deps with no package
# can be built from source the way the NetBSD job does
# (.github/scripts/netbsd/install-pkg.sh + .github/scripts/util.sh helpers).
#

set -ex

PATH="/opt/ooce/bin:/usr/bin:/usr/sbin:/sbin:$PATH"
export PATH

pkg refresh || true

# Toolchain + build system (ooce publisher) and the X libraries the server
# links against. `|| true` so a single wrong FMRI doesn't abort the whole set
# during bring-up — meson will then point at whatever is genuinely absent.
pkg install -v --accept \
    developer/gcc14 \
    developer/build/meson \
    developer/build/ninja \
    developer/build/pkg-config \
    developer/build/gnu-make \
    developer/versioning/git \
    runtime/python-312 \
    library/pixman \
    x11/header/x11-protocols \
    x11/library/libxau \
    x11/library/libxcb \
    x11/library/libxdmcp \
    x11/library/libxext \
    x11/library/libxfixes \
    x11/library/libxfont2 \
    x11/library/libxkbfile \
    x11/library/libxrender \
    x11/library/libxshmfence \
    x11/library/libxt \
    x11/library/libx11 \
    x11/library/xtrans \
    x11/library/libepoxy \
    || true
