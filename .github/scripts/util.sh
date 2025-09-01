
. .github/scripts/conf.sh

SOURCE_DIR=`pwd`

pkg_dir() {
    echo -n "${X11_DEPS_DIR}/$1"
}

clone_source() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"

    $SOURCE_DIR/.github/scripts/git-smart-checkout.sh \
        --name "$pkgname" \
        --url "$url" \
        --ref "$ref"
}

build_meson() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"
    shift
    shift
    shift || true
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        clone_source "$pkgname" "$url" "$ref"
        (
            cd $pkgdir
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
    shift
    shift
    shift || true
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        clone_source "$pkgname" "$url" "$ref"
        (
            cd $pkgdir
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
    shift
    shift
    shift || true
    clone_source "$pkgname" "$url" "$ref"
    (
        cd $pkgdir
        ./autogen.sh # --prefix=$X11_PREFIX
        make -j${FDO_CI_CONCURRENT:-4} # install
    )
}

build_ac_xts() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"
    shift
    shift
    shift || true
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        echo "::group::Build XTS"
        clone_source "$pkgname" "$url" "$ref"
        (
            cd $pkgdir
            CFLAGS='-fcommon'
            ./autogen.sh --prefix=$X11_PREFIX CFLAGS="$CFLAGS"
            if [ "$X11_OS" = "Darwin" ]; then
                make -j${FDO_CI_CONCURRENT:-4} install tetexec.cfg
            else
                xvfb-run make -j${FDO_CI_CONCURRENT:-4} install tetexec.cfg
            fi
        )
        touch $X11_PREFIX/$pkgname.DONE
        echo "::endgroup::"
    fi
}
