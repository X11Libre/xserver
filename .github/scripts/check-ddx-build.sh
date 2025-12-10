#!/bin/bash

set -e
set -o xtrace

check_executable () {
    if [[ ! -x $MESON_BUILDDIR/$1 ]]; then
        echo "$1 not found after build"
        exit 1
    fi
    return 0
}

if [[ -z "$MESON_BUILDDIR" ]]; then
    echo "\$MESON_BUILDDIR not set"
    exit 1
fi

[[ "$BUILD_XEPHYR" == true ]]   && check_executable "hw/kdrive/ephyr/Xephyr"
[[ "$BUILD_XNEST" == true ]]    && check_executable "hw/xnest/Xnest"
[[ "$BUILD_XORG" == true ]]     && check_executable "hw/xfree86/Xorg"
[[ "$BUILD_XQUARTZ" == true ]]  && check_executable "hw/xquartz/mach-startup/Xquartz"
[[ "$BUILD_XVFB" == true ]]     && check_executable "hw/vfb/Xvfb"

exit 0
