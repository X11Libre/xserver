dnl Copyright 2005 Red Hat, Inc
dnl 
dnl Permission to use, copy, modify, distribute, and sell this software and its
dnl documentation for any purpose is hereby granted without fee, provided that
dnl the above copyright notice appear in all copies and that both that
dnl copyright notice and this permission notice appear in supporting
dnl documentation.
dnl 
dnl The above copyright notice and this permission notice shall be included
dnl in all copies or substantial portions of the Software.
dnl 
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
dnl OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
dnl MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
dnl IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
dnl OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
dnl ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
dnl OTHER DEALINGS IN THE SOFTWARE.
dnl 
dnl Except as contained in this notice, the name of the copyright holders shall
dnl not be used in advertising or otherwise to promote the sale, use or
dnl other dealings in this Software without prior written authorization
dnl from the copyright holders.
dnl 

# XORG_DRIVER_CHECK_EXT(MACRO, PROTO)
# --------------------------
# Checks for the MACRO define in xorg-server.h (from the sdk).  If it
# is defined, then add the given PROTO to $REQUIRED_MODULES.

AC_DEFUN([XORG_DRIVER_CHECK_EXT],[
	AC_REQUIRE([PKG_PROG_PKG_CONFIG])
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS `$PKG_CONFIG --cflags xorg-server`"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include "xorg-server.h"
#if !defined $1
#error $1 not defined
#endif
		]])],
		[_EXT_CHECK=yes],
		[_EXT_CHECK=no])
	CFLAGS="$SAVE_CFLAGS"
	AC_MSG_CHECKING([if $1 is defined])
	AC_MSG_RESULT([$_EXT_CHECK])
	if test "$_EXT_CHECK" != no; then
		REQUIRED_MODULES="$REQUIRED_MODULES $2"
	fi
])

# XLIBRE_MODULE_VERSION
# --------------------
# Defines XLIBRE_MODULE_VERSION_{MAJOR|MINOR|PATCHLEVEL} for modules to use.

AC_DEFUN([XLIBRE_MODULE_VERSION],[
    AC_DEFINE_UNQUOTED([XLIBRE_MODULE_VERSION_MAJOR],
                       [`echo $PACKAGE_VERSION | cut -d . -f 1`], [major version])
    PVM=`echo $PACKAGE_VERSION | cut -d . -f 2 | cut -d - -f 1`
    if test "x$PVM" = "x"; then
        PVM="0"
    fi
    AC_DEFINE_UNQUOTED([XLIBRE_MODULE_VERSION_MINOR], [$PVM], [minor version])
    PVP=`echo $PACKAGE_VERSION | cut -d . -f 3 | cut -d - -f 1`
    if test "x$PVP" = "x"; then
        PVP="0"
    fi
    AC_DEFINE_UNQUOTED([XLIBRE_MODULE_VERSION_PATCH], [$PVP], [patch version])
])

AC_DEFUN([XLIBRE_SDK_VAR], [
    $1=`$PKG_CONFIG xlibre-server --variable=$2`
    if test "x$1" == "x" ; then
        AC_MSG_ERROR([xlibre-server.pc missing '$2' variable])
    fi
    AC_MSG_NOTICE([$1=${$1}])
    AC_SUBST($1)
])

AC_DEFUN([XLIBRE_PROBE_SDK], [
    PKG_PROG_PKG_CONFIG([0.25])
    PKG_CHECK_MODULES(XLIBRE_SERVER, xlibre-server xproto $XLIBRE_EXTRA_MODULES)

    XLIBRE_SDK_VAR([xlibre_server_name],       [server_name])
    XLIBRE_SDK_VAR([xlibre_server_config],     [server_config])
    XLIBRE_SDK_VAR([xlibre_module_dir],        [moduledir])
    XLIBRE_SDK_VAR([xlibre_libtool_flags],     [libtool_flags])
    XLIBRE_SDK_VAR([xlibre_input_drivers_dir], [input_drivers_dir])
    XLIBRE_SDK_VAR([xlibre_video_drivers_dir], [video_drivers_dir])
    XLIBRE_SDK_VAR([xlibre_sdk_dir],           [sdkdir])
    XLIBRE_SDK_VAR([xlibre_conf_dir],          [xserverconfigdir])
    XLIBRE_SDK_VAR([xlibre_driver_man_dir],    [driver_man_dir])
    XLIBRE_SDK_VAR([xlibre_driver_man_section],[driver_man_section])
    XLIBRE_SDK_VAR([xlibre_misc_man_dir],      [misc_man_dir])
    XLIBRE_SDK_VAR([xlibre_misc_man_section],  [misc_man_section])
    XLIBRE_SDK_VAR([xlibre_app_man_dir],       [app_man_dir])
    XLIBRE_SDK_VAR([xlibre_app_man_section],   [app_man_section])
    XLIBRE_SDK_VAR([xlibre_file_man_dir],      [file_man_dir])
    XLIBRE_SDK_VAR([xlibre_file_man_section],  [file_man_section])
])

AC_DEFUN([XLIBRE_INIT_MODULE], [
    XLIBRE_MODULE_VERSION
    XLIBRE_PROBE_SDK

    dnl AC_CONFIG_AUX_DIR(build-aux)
    AC_CONFIG_AUX_DIR(.)
    AC_DISABLE_STATIC
    AC_PROG_LIBTOOL
    AC_PROG_CC

    AH_TOP([#include "xorg-server.h"])
    xlibre_driver_name="$1"
    AC_SUBST(xlibre_driver_name)
])

AC_DEFUN([XLIBRE_INIT_MODULE_AM], [
    XLIBRE_INIT_MODULE($1)
    AC_CONFIG_SRCDIR([Makefile.am])
    AM_INIT_AUTOMAKE([dist-bzip2])
    AC_CONFIG_HEADERS([config.h])
])
