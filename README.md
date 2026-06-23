# XLibre Xserver

The XLibre Xserver is the community-managed display server for the [X Window System Protocol Version 11 (Wikipedia)](https://en.wikipedia.org/wiki/X_Window_System_core_protocol), in short, X11.

<p>
    <figure><a href="https://github.com/orgs/X11Libre/discussions/504#discussioncomment-17195498"><img src="https://github.com/X11Libre/website/blob/eb05723185647561390dade472edea5938333113/build/img/xlibre-one-year.jpg" alt="1 year of XLibre screenshot"></a><figcaption>One Year of XLibre. See more <a href="https://github.com/orgs/X11Libre/discussions/211">liberated screens here</a>.</figcaption>
    </figure>
</p>

## Selected Features

* All the good things from [X.Org Server](https://en.wikipedia.org/wiki/X.Org_Server), including its unreleased features
* [TearFree modesetting](https://github.com/X11Libre/xserver/commit/0dacee6c5149b63a563e9bed63502da2e9f1ac1f) by default and optionally [atomic modesetting](https://github.com/X11Libre/xserver/commit/461411c798c263f70daa96f7136614dfefda6adc)
* Support for the Nvidia drivers 340, 390, 470, 570, and newer
* [Xnamespace extension](https://github.com/X11Libre/xserver/blob/master/doc/Xnamespace.md) for separating X clients
* Support for seat management via [seatd](https://sr.ht/~kennylevinsen/seatd) besides [systemd-logind](https://www.freedesktop.org/software/systemd/man/latest/systemd-logind.service.html)
* [Xfbdev](https://github.com/X11Libre/xserver/tree/master/hw/kdrive), the generic framebuffer Xserver for Linux
* CI builds for several BSDs, Linux, MacOS, and Microsoft Windows
* Active community, cleanups, fixes, and development based on merit

To learn more about the features and mission of XLibre, please [visit our homepage](https://xlibre.net).


## Switching to XLibre

The easiest way to install and run XLibre is to use your distribution's provided packages. Please see the [Are We XLibre Yet? - (X11Libre/xserver Wiki)](https://github.com/X11Libre/xserver/wiki/Are-We-XLibre-Yet%3F) page for a list of the available options. If there is no option, then go on with building and installing XLibre from source.


### Building XLibre

After cloning the [Xserver repository](https://github.com/X11Libre/xserver.git) or unpacking the sources and installing the dependencies, change into the source directory and run the [Meson](https://mesonbuild.com) build tool:

```shell
cd "<source dir of xserver>"
meson setup <prefix> build <meson_options>
ninja -C build install
```

You may specify the install `<prefix>` with, for example, `--prefix="$(pwd)/image"` and add build time [`<meson_options>`](https://github.com/X11Libre/xserver/blob/master/meson_options.txt) like so: `-Dxnest=false`. You may also want to build and install some graphics and input drivers. Please refer to the [Building XLibre (X11Libre/xserver Wiki)](https://github.com/X11Libre/xserver/wiki/Building-XLibre) page for more details.


### Configuring XLibre

Until XLibre releases its own, you can find a detailed description of the configuration on the [Configuration - Xorg (ArchWiki)](https://wiki.archlinux.org/title/Xorg#Configuration) page. If you have built and installed XLibre yourself, then change into the `<prefix>` directory with `cd <prefix>` and create a directory `etc/X11` with a file `xorg.conf` and adjust it accordingly.

Starting with version 25.0.0.16, the proprietary Nvidia driver is autodetected and handled internally without any special configuration. Please see the [Compatibility of XLibre (X11Libre/xserver Wiki)](https://github.com/X11Libre/xserver/wiki/Compatibility-of-XLibre) page for [more details on the Nvidia driver](https://github.com/X11Libre/xserver/wiki/Compatibility-of-XLibre#nvidia-proprietary-driver) and compatibility in general.


### Running XLibre

If you installed XLibre using your distribution's provided packages, then the Xserver is usually started by [init (Wikipedia)](https://en.wikipedia.org/wiki/Init) on system start. On other systems it should be possible to manually start XLibre with user permissions by invoking `startx`. Please refer to [`man startx`](https://linux.die.net/man/1/startx) for how to use it.

If you have built and installed XLibre yourself, then you may want to shutdown other Xservers, change into the `<prefix>` directory, and create a simple `testx.sh` file with the following contents:

```shell
#!/bin/sh
./bin/X :1 vt8 -logfile /dev/stdout &
_pid=$!
sleep 10 && kill $_pid
```

You can adjust the `:1 vt8` and other options in the `testx.sh` file as detailed in [`man Xorg`](https://linux.die.net/man/1/xorg). Make the `testx.sh` executable and run it:

```shell
chmod 0770 testx.sh
./testx.sh
```

This should give you 10 glorious seconds of a black and beautiful and empty screen. Afterwards the Xserver complains about being killed, but there should be no other critical errors for a "test passed." For more details, please see [Building XLibre (X11Libre/xserver Wiki)](https://github.com/X11Libre/xserver/wiki/Building-XLibre).

If you are using seatd for rootless operation instead of (e)logind, create the following file named `.xserverrc` in your home directory:
```shell
#!/bin/sh
exec X -keeptty "$@"
```
Then, run `chmod +x ~/.xserverrc`.


## I want to help!

That's great; there's enough to do for everyone. You may consider [one](https://github.com/orgs/X11Libre/discussions/categories/1-new-ideas) of the [many](https://github.com/orgs/X11Libre/discussions/categories/2-rfcs-of-the-core-team) [ideas](https://github.com/orgs/X11Libre/discussions/categories/3-ideas-soon-to-be-addressed) and [feature requests](https://github.com/X11Libre/xserver/issues?q=is%3Aissue%20state%3Aopen%20label%3Aenhancement) out there. To help in testing, you may consider becoming an [XLibre Test Driver](https://github.com/X11Libre/xserver/wiki/XLibre-Test-Drivers). Please also have a look at the [good first](https://github.com/X11Libre/xserver/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22good%20first%20issue%22) and [help wanted](https://github.com/X11Libre/xserver/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22help%20wanted%22) issues and the [Liberated Screens](https://github.com/orgs/X11Libre/discussions/211).

If you want to work on anything, just let us know. If you have any questions, [just ask](https://github.com/orgs/X11Libre/discussions/categories/q-a). Thank you!


## Contact

[XLibre Discussions at GitHub](https://github.com/orgs/X11Libre/discussions) | [XLibre mailing list at FreeLists](https://www.freelists.org/list/xlibre) | [@x11dev channel at Telegram](https://t.me/x11dev) | [#xlibredev space at Matrix](https://matrix.to/#/#xlibredev:matrix.org) | [XLibre security contact at GitHub](https://github.com/X11Libre/xserver/security/policy)
