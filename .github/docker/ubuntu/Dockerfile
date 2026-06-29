# Base image for the ubuntu xserver / SDK / driver builds.
#
# Bakes in everything those jobs install before building the xserver itself:
#   - the apt build dependencies (.github/scripts/ubuntu/install-pkg.sh)
#   - the from-source prereqs that are otherwise rebuilt/actions-cached each run
#     (.github/scripts/ubuntu/install-prereq.sh: rendercheck, libdrm, libxcvt,
#      xorgproto, xts, piglit, goxproto)
#
# So CI jobs based on this image skip apt + the prereq build/restore entirely
# and go straight to building the xserver. This is step 1 of the driver-pipeline
# rework; later steps build the xserver+SDK on top of it and feed the driver
# builds from a per-commit image instead of the eviction-prone actions/cache.
#
# Built + pushed to GHCR by .github/workflows/ubuntu-deps-image.yml. The prereq
# versions come from .github/scripts/conf.sh (PKG_*_REF), so a change there (or
# to the install scripts) rebuilds the image via that workflow's path filter.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
# Stable, documented locations for the baked prereqs; consuming jobs point
# X11_PREFIX / X11_BUILD_DIR (and PKG_CONFIG_PATH) here.
ENV X11_PREFIX=/opt/x11
ENV X11_BUILD_DIR=/opt/x11-build

COPY .github /src/.github
WORKDIR /src

# apt build dependencies (same list the ubuntu-install-pkg action uses)
RUN apt-get update && \
    ./.github/scripts/ubuntu/install-pkg.sh && \
    rm -rf /var/lib/apt/lists/*

# from-source prereqs (rendercheck, libdrm, libxcvt, xorgproto, xts, piglit,
# goxproto) — installed into $X11_PREFIX, sources/test trees kept in
# $X11_BUILD_DIR (piglit/goxproto/xts are used directly from there at test time)
RUN ./.github/scripts/ubuntu/install-prereq.sh
