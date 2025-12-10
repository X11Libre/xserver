#!/bin/bash

set -e
set -o xtrace

if [[ -z "$MESON_BUILDDIR" ]]; then
    echo "\$MESON_BUILDDIR not set"
    exit 1
fi

check_piglit_results ()
{
    local EXPECTED_RESULTS="$MESON_BUILDDIR"/test/piglit-results/$1
    local DEPENDENCY="$MESON_BUILDDIR"/$2

    if ! test -e $DEPENDENCY; then
	return
    fi

    if test -e $EXPECTED_RESULTS; then
	return
    fi

    echo Expected $EXPECTED_RESULTS does not exist
    exit 1
}

check_piglit_results xephyr-glamor hw/kdrive/ephyr/Xephyr.p/ephyr_glamor.c.o
check_piglit_results xvfb hw/vfb/Xvfb
