export X11_OS=`uname -s`

export X11_PREFIX="${X11_PREFIX:-$HOME/x11}"
export X11_BUILD_DIR="${X11_BUILD_DIR:-$HOME/build-deps}"
export DRV_BUILD_DIR="${DRV_BUILD_DIR:-$HOME/build-drivers}"

case "$X11_OS" in
Darwin) export FDO_CI_CONCURRENT=`sysctl -n hw.logicalcpu` ;;
Linux) export FDO_CI_CONCURRENT=`nproc` ;;
esac

export PKG_CONFIG_PATH="$X11_PREFIX/lib/x86_64-linux-gnu/pkgconfig:$X11_PREFIX/lib/pkgconfig:$X11_PREFIX/share/pkgconfig:$PKG_CONFIG_PATH"

export PKG_PIGLIT_REF=59111996534f875ca88bce51f21fa2e6564895da
export PKG_XTS_REF=6cf94400a09abecd6b86e4eb6441741acecd51f6
export PKG_RENDERCHECK_REF=rendercheck-1.6
export PKG_LIBDRM_REF=libdrm-2.4.121
export PKG_LIBXCVT_REF=libxcvt-0.1.0
export PKG_XORGPROTO_REF=xorgproto-2024.1
