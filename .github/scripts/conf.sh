export X11_PREFIX="${X11_PREFIX:-$HOME/x11}"
export X11_BUILD_DIR="${X11_BUILD_DIR:-$HOME/build-deps}"
export DRV_BUILD_DIR="${DRV_BUILD_DIR:-$HOME/build-drivers}"

export FDO_CI_CONCURRENT=`nproc`

export PATH="$X11_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="$X11_PREFIX/lib/x86_64-linux-gnu/pkgconfig:$X11_PREFIX/lib/pkgconfig:$X11_PREFIX/share/pkgconfig:$PKG_CONFIG_PATH"
