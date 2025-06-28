
. .github/scripts/conf.sh

clone_source() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"

    if [ ! -f $pkgname/.git/config ]; then
        echo "need to clone $pkgname"
        git clone $url $pkgname --branch=$ref --depth 1
    else
        echo "already cloned $pkgname"
    fi
}

build_meson() {
    local pkgname="$1"
    local url="$2"
    local ref="$3"
    shift
    shift
    shift
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
    shift
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        clone_source "$pkgname" "$url" "$ref"
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
    shift
    shift
    shift
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
    shift
    if [ -f $X11_PREFIX/$pkgname.DONE ]; then
        echo "package $pkgname already built"
    else
        clone_source "$pkgname" "$url" "$ref"
        (
            cd $pkgname
            CFLAGS='-fcommon'
            if [ "$X11_OS" = "Darwin" ]; then
                # xts5/include/XtTest.h includes <X11/Intrinsic.h> => needs xt
                # xts5/src/libXtaw/*.c include <X11/Xaw/*.h> and <X11/Xmu/*.h> => need xmu and xaw7
                sed -E -i~ 's|(\[XTS\], \[)|\1xt xmu xaw7 |' configure.ac
                # xts5/Xlib14/X{mb,wc}TextListToTextProperty.m define a function accepting `XTextProperty` but call it passing
                #`XTextProperty*`; since the parameter is seemingly meant for output, accept it as pointer
                sed -E -i~ -e 's|(XTextProperty)[[:space:]]+(text_prop_good)|\1 *\2|' -e 's|(style_good),[[:space:]]*&(text_prop_good)|\1,\2|' -e 's|text_prop_good\.|text_prop_good->|' xts5/Xlib14/X{mb,wc}TextListToTextProperty.m
                # xts5/Xlib*/*.m forward-declare `strcpy()` which is incompatible with _FORTIFY_SOURCE > 0 where `strcpy` is
                # a macro; set _FORTIFY_SOURCE to 0 as we don't care much about security in this test code
                CFLAGS="$CFLAGS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0"
                # declarations for `XtAppSetError{,Msg}Handler()` in <X11/Intrinsic.h> (libXt) make it hard to write warning-
                # free code: they want a noreturn-annotated handler as input but return old handler without noreturn annotation,
                # so e.g. `XtAppSetErrorHandler(XtAppSetErrorHandler(NULL))` (similar to xts5/Xt13/XtAppSetError*Handler.m)
                # doesn't compile complaining about incompatible function pointers, at least with Apple Clang 16 (not sure why
                # it treats this warning as error by default though)
                if cc -Werror=unknown-warning-option -Wincompatible-function-pointer-types -c -xc -o /dev/null /dev/null 2>/dev/null; then
                    CFLAGS="$CFLAGS -Wno-error=incompatible-function-pointer-types"
                fi
            fi
            ./autogen.sh --prefix=$X11_PREFIX CFLAGS="$CFLAGS"
            make -j${FDO_CI_CONCURRENT:-4} install
        )
        touch $X11_PREFIX/$pkgname.DONE
    fi
}
