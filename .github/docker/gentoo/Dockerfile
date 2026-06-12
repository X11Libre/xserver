# Prebuilt Gentoo image with all X-server build dependencies baked in.
#
# Why: the official Gentoo binhost is built with the default-profile USE flags
# (no X), so the X-USE variants we need (freetype[harfbuzz], libglvnd[X], mesa,
# and mesa's llvm) have no matching binary package and must be compiled from
# source — a multi-hour job. Doing that once here, off the CI critical path,
# lets the xserver-build-gentoo job start from a ready image and only run meson
# (~5 min) instead of emerging 115 packages (~20 min) on every run.
#
# Built + pushed to GHCR by .github/workflows/gentoo-image.yml.
FROM gentoo/stage3:latest

# Sync the portage tree (stage3 ships none) and configure portage for a CI
# build: official binhost for the bulk, our X USE flags for the few that must
# be compiled, sandbox off (unprivileged container).
RUN emerge-webrsync --quiet && \
    cat >> /etc/portage/make.conf <<'EOF'

# --- xlibre CI build settings ---
ACCEPT_LICENSE="*"
USE="${USE} X gbm egl gles2 udev harfbuzz png -systemd"
FEATURES="${FEATURES} getbinpkg -sandbox -usersandbox -ipc-sandbox -network-sandbox -pid-sandbox"
EMERGE_DEFAULT_OPTS="--jobs=4 --quiet-build=y --getbinpkg --binpkg-respect-use=y --autounmask-continue=y"
EOF

# Emerge the full X-server build dependency set (binary where the binhost has a
# USE-matching package, from source otherwise — this is the one-time slow part).
RUN emerge --oneshot \
    app-emulation/spice-protocol \
    dev-build/meson \
    dev-build/ninja \
    dev-libs/expat \
    dev-libs/libbsd \
    dev-libs/libevdev \
    dev-libs/libffi \
    dev-libs/libgcrypt \
    dev-libs/libinput \
    dev-libs/nettle \
    dev-python/mako \
    dev-vcs/git \
    media-fonts/font-util \
    media-libs/libepoxy \
    media-libs/libglvnd \
    media-libs/mesa \
    net-libs/libtirpc \
    sys-apps/dbus \
    sys-libs/libunwind \
    virtual/libudev \
    virtual/pkgconfig \
    x11-apps/xkbcomp \
    x11-base/xorg-proto \
    x11-libs/cairo \
    x11-libs/libdrm \
    x11-libs/libpciaccess \
    x11-libs/libX11 \
    x11-libs/libXau \
    x11-libs/libXaw \
    x11-libs/libxcb \
    x11-libs/libXdmcp \
    x11-libs/libXext \
    x11-libs/libXfixes \
    x11-libs/libXfont2 \
    x11-libs/libXi \
    x11-libs/libXinerama \
    x11-libs/libxkbfile \
    x11-libs/libXmu \
    x11-libs/libXpm \
    x11-libs/libXrandr \
    x11-libs/libXrender \
    x11-libs/libXres \
    x11-libs/libxshmfence \
    x11-libs/libXt \
    x11-libs/libXtst \
    x11-libs/libXv \
    x11-libs/libxcvt \
    x11-libs/pango \
    x11-libs/pixman \
    x11-libs/xcb-util \
    x11-libs/xcb-util-image \
    x11-libs/xcb-util-keysyms \
    x11-libs/xcb-util-renderutil \
    x11-libs/xcb-util-wm \
    x11-misc/xkeyboard-config

# Drop the caches/tree the build job does not need, to keep the image lean
# (the dependencies themselves are installed into the rootfs and stay).
RUN rm -rf /var/cache/binpkgs/* /var/cache/distfiles/* /var/db/repos/gentoo
