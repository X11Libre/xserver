#!/bin/bash

set -e
set -o xtrace

. .github/scripts/util.sh

HOST=$1

# Debian's cross-pkg-config wrappers are broken for MinGW targets, since
# dpkg-architecture doesn't know about MinGW target triplets.
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=930492
cat >/usr/local/bin/${HOST}-pkg-config <<EOF
#!/bin/sh

PKG_CONFIG_SYSROOT_DIR=/usr/${HOST} PKG_CONFIG_LIBDIR=/usr/${HOST}/lib/pkgconfig:/usr/${HOST}/share/pkgconfig pkg-config \$@
EOF
chmod +x /usr/local/bin/${HOST}-pkg-config

# when cross-compiling, some autoconf tests cannot be run:

# --enable-malloc0returnsnull
export xorg_cv_malloc0_returns_null=yes

die() {
    echo "FAILED: $*" >&1
    exit 1
}

# retry <max_attempts> <sleep_seconds> <command...>
retry() {
    local max_attempts=$1
    local sleep_time=$2
    shift 2

    local attempt=1
    while true; do
        "$@" && return 0   # success → return immediately
        if [ $attempt -ge $max_attempts ]; then
            return 1       # failed after all attempts
        fi
        attempt=$((attempt+1))
        sleep "$sleep_time"
    done
}

try_clone() {
    local url="$1"
    local commit="$2"
    local name="$3"

    if [[ $commit =~ ^[[:xdigit:]]{1,}$ ]]
    then
        git clone ${url} ${name} || return 1
        git -C ${name} checkout ${commit} || return 1
    else
        git clone --depth 1 --branch ${commit:-master} --recurse-submodules -c advice.detachedHead=false ${url} ${name} || return 1
    fi
}

build() {
    url=$1
    commit=$2
    config=$3

    name=$(basename ${url} .git)

    retry 10 10 try_clone "$url" "$commit" "$name" || die "failed cloning $url - $commit"

    pushd ${name}
    NOCONFIGURE=1 ./autogen.sh || ./.bootstrap
    ./configure ${config} --host=${HOST} --prefix= --with-sysroot=/usr/${HOST}/
    make -j$(nproc)
    DESTDIR=/usr/${HOST} make install

    popd
    rm -rf ${OLDPWD}
}

build_fdo() {
    local name="$1"
    shift
    build $(fdo_mirror $name) "$@"
}

build_fdo 'pixman'                  'pixman-0.38.4'
build_fdo 'pthread-stubs'           '0.4'
# we can't use the xorgproto pkgconfig files from /usr/share/pkgconfig, because
# these would add -I/usr/include to CFLAGS, which breaks cross-compilation
build_fdo 'xorgproto'               'xorgproto-2024.1' '--datadir=/lib'
build_fdo 'libXau'                  'libXau-1.0.9'
build_fdo 'xcbproto'                'xcb-proto-1.17.0'
build_fdo 'libxcb'                  'libxcb-1.17.0'
build_fdo 'libxtrans'               'xtrans-1.6.0'
# the default value of keysymdefdir is taken from the includedir variable for
# xproto, which isn't adjusted by pkg-config for the sysroot
# Using -fcommon to address build failure when cross-compiling for windows.
# See discussion at https://gitlab.freedesktop.org/xorg/xserver/-/merge_requests/913
CFLAGS="-fcommon" build_fdo 'libX11' 'libX11-1.6.9' "--with-keysymdefdir=/usr/${HOST}/include/X11"
build_fdo 'libxkbfile'              'libxkbfile-1.1.0'
# freetype needs an explicit --build to know it's cross-compiling
# disable png as freetype tries to use libpng-config, even when cross-compiling
build_fdo 'freetype'                'VER-2-10-1' "--build=$(cc -dumpmachine) --with-png=no"
build_fdo 'font-util'               'font-util-1.3.2'
build_fdo 'libfontenc'              'libfontenc-1.1.4'
build_fdo 'libXfont'                'libXfont2-2.0.3'
build_fdo 'libXdmcp'                'libXdmcp-1.1.3'
build_fdo 'libXfixes'               'libXfixes-5.0.3'
build_fdo 'libxcb-util'             'xcb-util-0.4.1-gitlab'
build_fdo 'libxcb-image'            'xcb-util-image-0.4.1-gitlab'
build_fdo 'libxcb-wm'               'xcb-util-wm-0.4.2'
build_fdo 'libxcb-render-util'      'master'
build_fdo 'libxcb-keysyms'          'master'

# workaround xcb_windefs.h leaking all Windows API types into X server build
# (some of which clash which types defined by Xmd.h) XXX: This is a bit of a
# hack, as it makes this header depend on xorgproto. Maybe an upstreamable
# fix would involve a macro defined in the X server (XFree86Server?
# XCB_NO_WINAPI?), which makes xcb_windefs.h wrap things like XWinsock.h
# does???
sed -i s#winsock2#X11/Xwinsock# /usr/${HOST}/include/xcb/xcb_windefs.h
