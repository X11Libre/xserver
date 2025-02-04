export X11_PREFIX=/home/runner/x11
export PATH="$X11_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="$X11_PREFIX/lib/x86_64-linux-gnu/pkgconfig:$X11_PREFIX/lib/pkgconfig:$X11_PREFIX/share/pkgconfig:$PKG_CONFIG_PATH"
export FDO_CI_CONCURRENT=`nproc`
export X11_BUILD_DIR=/home/runner/build-deps
export DRV_BUILD_DIR=/home/runner/build-drivers
