# Per-commit X-server + SDK image, built on top of the prebuilt deps base image.
#
# Step 2 of the driver-pipeline rework: builds the xserver with -Dxorg-sdk=true
# and installs it (into the /opt/x11 prefix that the base image already uses for
# its prereqs), so the driver builds can later consume a ready SDK from a
# per-commit image (ghcr.io/.../xserver-sdk:<sha>) instead of the eviction-prone
# actions/cache. Built + pushed by .github/workflows/sdk-image.yml.
#
# Multi-stage so the final image carries only the installed SDK (/opt/x11), not
# the xserver source tree.

# --- build stage: compile + install the SDK into /opt/x11 -------------------
FROM ghcr.io/x11libre/xserver-ubuntu-build:latest AS build

COPY . /src/xserver
WORKDIR /src/xserver

# The base image keeps its from-source prereqs (xorgproto, libxcvt, …) in
# /opt/x11; point pkg-config there so the xserver build finds them, and install
# the SDK into the same prefix. Args mirror the drivers-prepare-ubuntu job
# (-Dxorg-sdk=true, Xorg DDX on, the X test servers off).
RUN MACHINE="$(gcc -dumpmachine)" && \
    export PKG_CONFIG_PATH="/opt/x11/lib/$MACHINE/pkgconfig:/opt/x11/lib/pkgconfig:/opt/x11/share/pkgconfig" && \
    meson setup _build \
        -Dprefix=/opt/x11 \
        -Dxorg-sdk=true -Dxorg=true \
        -Dxephyr=false -Dxvfb=false -Dxnest=false -Dxfbdev=false \
        -Dwerror=false -Dxcsecurity=false && \
    ninja -C _build install

# --- final stage: base + the installed SDK, no source -----------------------
FROM ghcr.io/x11libre/xserver-ubuntu-build:latest
COPY --from=build /opt/x11 /opt/x11
