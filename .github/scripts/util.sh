
. .github/scripts/conf.sh

clone_source() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"
    local commit="$4"

    if [ ! -f $pkgname/.git/config ]; then
        echo "need to clone $pkgname"
        if [ "$commit" ]; then
            git clone $url $pkgname --branch=$ref
        else
            git clone $url $pkgname --branch=$ref --depth 1
        fi
    else
        echo "already cloned $pkgname"
    fi

    if [ "$commit" ]; then
        ( cd $pkgname && git checkout -f "$commit" )
    fi
}

build_meson() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"
    local commit="$4"
    shift
    shift
    shift
    shift || true
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        clone_source "$pkgname" "$url" "$ref" "$commit"
        (
            cd $pkgname
            meson "$@" build -Dprefix=$X11_PREFIX
            ninja -j${FDO_CI_CONCURRENT:-4} -C build install
        )
        touch $X11_PREFIX/$pkgname.DONE
    fi
}

build_ac() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"
    local commit="$4"
    shift
    shift
    shift
    shift || true
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        clone_source "$pkgname" "$url" "$ref" "$commit"
        (
            cd $pkgname
            ./autogen.sh --prefix=$X11_PREFIX
            make -j${FDO_CI_CONCURRENT:-4} install
        )
        touch $X11_PREFIX/$pkgname.DONE
    fi
}

build_drv_ac() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"
    local commit="$4"
    shift
    shift
    shift
    shift || true
    clone_source "$pkgname" "$url" "$ref" "$commit"
    (
        cd $pkgname
        ./autogen.sh # --prefix=$X11_PREFIX
        make -j${FDO_CI_CONCURRENT:-4} # install
    )
}

build_ac_xts() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"
    local commit="$4"
    shift
    shift
    shift
    shift || true
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        clone_source "$pkgname" "$url" "$ref" "$commit"
        (
            cd $pkgname
            CFLAGS='-fcommon'
            ./autogen.sh --prefix=$X11_PREFIX CFLAGS="$CFLAGS"
            make -j${FDO_CI_CONCURRENT:-4} install
        )
        touch $X11_PREFIX/$pkgname.DONE
    fi
}
