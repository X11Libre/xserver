
. .github/scripts/conf.sh

[ "$X11_INSTALL_PREFIX" ] || X11_INSTALL_PREFIX="$X11_PREFIX"

SOURCE_DIR=`pwd`

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
    shift
    shift
    shift || true
    mkdir -p $X11_PREFIX
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        clone_source "$pkgname" "$url" "$ref"
        (
            cd $pkgname
            ./autogen.sh --prefix=$X11_INSTALL_PREFIX
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
        cd $pkgname
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
            cd $pkgname
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

fdo_mirror() {
    local repo="$1"
    echo -n "https://github.com/X11Libre/mirror.fdo.$1"
}

xl_mirror() {
    local repo="$1"
    echo -n "https://github.com/X11Libre/$1"
}

drv_tag() {
    local name="$1"
    local version="$2"
    case "$2" in
        refs/*)
            echo -n "$2"
        ;;
        *)
            echo -n "xlibre-xf86-$name-$version"
        ;;
    esac
}

build_xf86drv_ac() {
    local drv_name="$1"
    local version="$2"
    local repo_name="xf86-$drv_name"
    local tag_name="xlibre-xf86-$drv_name-$version"
    build_drv_ac "$repo_name" "$(xl_mirror $repo_name)" "$(drv_tag $drv_name $version)"
}
