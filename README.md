# XLibre Xserver

<p>
    <figure><a href="https://github.com/orgs/X11Libre/discussions/211#discussioncomment-13796738"><img src="https://github.com/X11Libre/website/blob/1d16316c0dbcfa5d09531136fb52ed7ad037c9b6/readme/img/xlibre-freebsd.png" alt="XLibre running on FreeBSD"></a><figcaption>XLibre running on FreeBSD. See more <a href="https://github.com/orgs/X11Libre/discussions/211">liberated screens here</a>.</figcaption>
    </figure>
</p>

XLibre is a display server implementation of the [X Window System Protocol Version 11 (Wikipedia)](https://en.wikipedia.org/wiki/X_Window_System_core_protocol), in short X11. It has been forked from the [X.Org Server (Wikipedia)](https://en.wikipedia.org/wiki/X.Org_Server). For the reasons of the fork please see the [HISTORY.md](HISTORY.md).


## Our mission

The XLibre contributors strive to cleanup and strengthen the existing code base while maintaining backward compatibility to make X11 a viable choice for the future. Another goal is to actively enhance the functionality of the Xserver and its drivers. We also take care of the improvements to the X.Org Server that have been unreleased for several years or were made to Xwayland only. Our decision making is based on merit and our active community keen to bring X forward.


## Our achievements

Since the fork on June 5, 2025 our by now more than 30 contributors have e.g. released numerous code cleanups, the [Xnamespace extension](https://github.com/X11Libre/xserver/blob/master/doc/Xnamespace.md) for separating X clients and backported the June 2025 [X](https://github.com/X11Libre/xserver/commit/c430c829d58a79a5d75ce43547fb649126baed01)[.](https://github.com/X11Libre/xserver/commit/899afa4c1097ed99858754677c37e1792ed3338f)[O](https://github.com/X11Libre/xserver/commit/3151e489e4754c0b426e7a771075d8f5d1b09144)[r](https://github.com/X11Libre/xserver/commit/a1e44d3c4ff997772c695c578286e2735e17f445)[g](https://github.com/X11Libre/xserver/commit/0d6af5a5429c2df1e5d7dff85d4a62599eb05504) [C](https://github.com/X11Libre/xserver/commit/da5f8d197fc25d898212714c653d66a91cbae7ab)[V](https://github.com/X11Libre/xserver/commit/948630fa428d8e0111c29a882c45b4c8bee5a796)[E](https://github.com/X11Libre/xserver/commit/923837e2c92c226ab9d4c57b94ac80fbe98bdf08) fixes. Together we integrated [TearFree by default](https://github.com/X11Libre/xserver/commit/0dacee6c5149b63a563e9bed63502da2e9f1ac1f) and [allowed enabling atomic modesetting](https://github.com/X11Libre/xserver/commit/461411c798c263f70daa96f7136614dfefda6adc). Xnest was ported to xcb, [per-ABI driver directories](https://github.com/X11Libre/xserver/commit/49c6431695f817845d921f74bf24e9e30ddd89a5) were introduced and [XQuartz has been added to our build jobs](https://github.com/X11Libre/xserver/commit/f40afc89832d1faf19beb4d394956208a98462cc). We have fought through [all](https://github.com/X11Libre/xserver/pulls?q=is%3Apr) [the](https://github.com/X11Libre/misc/issues?q=is%3Aissue) [issues](https://github.com/X11Libre/xserver/issues?q=is%3Aissue) that have been thrown at us, reached the first stage of the associated workflow and established our support for [packaging XLibre by volunteers](https://github.com/orgs/X11Libre/discussions/categories/xlibre-on-other-distributions). We have also made a [first release announcement](https://www.freelists.org/post/xlibre/Xlibre-250-summer-solstice-release) and created the [NEWS](NEWS).

A side note: If your XLibre Xserver is already set up and running you may want to skip to [our roadmap](#our-roadmap)

<p>
    <figure><a href="https://github.com/orgs/X11Libre/discussions/211#discussioncomment-13768576"><img src="https://github.com/X11Libre/website/blob/1d16316c0dbcfa5d09531136fb52ed7ad037c9b6/readme/img/xlibre-rk3588.png" alt="XLibre running accelerated on RK3588"></a><figcaption>XLibre running accelerated on <a href="https://github.com/choushunn/awesome-RK3588">RK3588</a>. See more <a href="https://github.com/orgs/X11Libre/discussions/211">liberated screens here</a>.</figcaption>
    </figure>
</p>


## Switching to XLibre

The easiest way to install and run XLibre is to use your distribution's provided packages. Please see the [Are We XLibre Yet? - (X11Libre/xserver Wiki)](https://github.com/X11Libre/xserver/wiki/Are-We-XLibre-Yet%3F) page for a list of the available options. If there is no option then go on with building and installing XLibre from source.


### Building XLibre

After cloning the [Xserver repository](https://github.com/X11Libre/xserver.git) or unpacking the sources and installing the dependencies, change into the source directory and run the [Meson](https://mesonbuild.com) build tool:

```shell
cd "<source dir of xserver>"
meson setup <prefix> build <meson_options>
ninja -C build install
```

You may specify the install `<prefix>` with for example `--prefix="$(pwd)/image"` and add build time [`<meson_options>`](https://github.com/X11Libre/xserver/blob/master/meson_options.txt) like so: `-Dxnest=false`. You may also want to build and install some graphics and input drivers. Please refer to the [Building XLibre (X11Libre/xserver Wiki)](https://github.com/X11Libre/xserver/wiki/Building-XLibre) page for more details.


### Configuring XLibre

To enable loading of the **proprietary Nvidia driver**, please add the following to your X configuration, e.g. `/etc/X11/xorg.conf`:

```
Section "ServerFlags"
  Option "IgnoreABI" "1"
EndSection
```

Please see the [Compatibility of XLibre (X11Libre/xserver Wiki)](https://github.com/X11Libre/xserver/wiki/Compatibility-of-XLibre) page for [more details on the Nvidia driver](https://github.com/X11Libre/xserver/wiki/Compatibility-of-XLibre#nvidia-proprietary-driver) and compatibility in general.

Until XLibre releases its own, you can find a detailed description of the configuration on the [Configuration - Xorg (ArchWiki)](https://wiki.archlinux.org/title/Xorg#Configuration) page. If you have built and installed XLibre yourself then change into the `<prefix>` directory with `cd <prefix>` and create a directory `etc/X11` with a file `xorg.conf` and adjust it accordingly.


### Running XLibre

If you installed XLibre using your distribution's provided packages then the Xserver is usually started by [init (Wikipedia)](https://en.wikipedia.org/wiki/Init) on system start. On other systems it should be possible to manually start XLibre with user permissions by invoking `startx`. Please refer to [`man startx`](https://linux.die.net/man/1/startx) for how to use it.

If you have built and installed XLibre yourself then you may want to shutdown other Xservers, change into the `<prefix>` directory and create a simple `testx.sh` file with the following contents:

```shell
#!/bin/sh
./bin/X :1 vt8 &
_pid=$!
sleep 10 && kill $_pid
```

You can adjust the `:1 vt8` and other options in the `testx.sh` file as detailed in [`man Xorg`](https://linux.die.net/man/1/xorg). Make the `testx.sh` executable and run it:

```shell
chmod 0770 testx.sh
./testx.sh
```

This should give you glorious 10 seconds of a black and beautyful and empty screen. Afterwards the Xserver complains about being killed but there should be no other critical errors for a "test passed". For more details please see [Building XLibre (X11Libre/xserver Wiki)](https://github.com/X11Libre/xserver/wiki/Building-XLibre).


## Our roadmap

Roughly speaking: We will continue to cleanup and modernize the codebase, enhance our continous integration, add static code analysis and improve manual and automated testing. Therefore, we will consolidate our build infrastructure and release process and also add more platforms to the test cycle. We will also further look into separating X clients by the Xnamespace extension and providing practical examples of how to use it.

Some of the many ideas and feature requests we received will be refined and prepared for implementation and we will go on to integrate relevant but unreleased Xorg and Xwayland features as well. Our documentation will see improvements on how to build, configure and switch to XLibre. A revamp of our website, the creation of a logo and more rebranding to XLibre are also on the map. End of roughly speaking.

One of the very next steps is to concretize our roadmap by using the Github Projects feature to arrange and prioritize bug reports, feature requests and other issues. This will give anybody a clear picture of what is next. All in the open so you can follow along and more so participate.


## I want to help!

That's great, there's enough to do for everyone. You may consider [one](https://github.com/orgs/X11Libre/discussions/categories/1-new-ideas) of the [many](https://github.com/orgs/X11Libre/discussions/categories/2-rfcs-of-the-core-team) [ideas](https://github.com/orgs/X11Libre/discussions/categories/3-ideas-soon-to-be-addressed) and [feature requests](https://github.com/X11Libre/xserver/issues?q=is%3Aissue%20state%3Aopen%20label%3Aenhancement) out there like [adding static code analysis/coverage](https://github.com/orgs/X11Libre/discussions/239) or [creating a logo for XLibre](https://github.com/X11Libre/xserver/issues/112) or becoming a [XLibre Test Driver](https://github.com/orgs/X11Libre/discussions/240).

There is also a good chance to [enhance the documentation of Xnamespace](https://github.com/X11Libre/xserver/issues/458) and add more details to the [Building XLibre](https://github.com/X11Libre/xserver/wiki/Building-XLibre) and [Graphics devices & drivers](https://github.com/X11Libre/xserver/wiki/Graphics-devices-&-drivers) wiki pages. Or to update the [desktop environments](https://github.com/X11Libre/xserver/wiki/Are-We-XLibre-Yet%3F#desktop-environments) and [display managers](https://github.com/X11Libre/xserver/wiki/Are-We-XLibre-Yet%3F#display-managers) lists as things work for you.

To wrap things up, please have a look at the [Xlibre On Other Distributions discussion](https://github.com/orgs/X11Libre/discussions/categories/xlibre-on-other-distributions). There is already some nice work going on and also some more could be done like [creating XLibre packages in OpenSuse Build Service (OBS)](https://github.com/orgs/X11Libre/discussions/235), a [PPA for Ubuntu/Linux Mint](https://github.com/orgs/X11Libre/discussions/118) or [RPM packaging support](https://github.com/orgs/X11Libre/discussions/126). Or create a [Platform cheatsheet](https://github.com/orgs/X11Libre/discussions/241) in general.

That's not enough? Then have a glance on the [good first](https://github.com/X11Libre/xserver/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22good%20first%20issue%22) and [help wanted](https://github.com/X11Libre/xserver/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22help%20wanted%22) issues. There are more to come, especially for code cleanups. If you want to work on anything, just let us know. If you have any questions, [just ask](https://github.com/orgs/X11Libre/discussions/categories/q-a). We thank you!


## You're welcome!

<p>
    <figure><a href="https://github.com/orgs/X11Libre/discussions/211#discussioncomment-13818114"><img src="https://github.com/X11Libre/website/blob/1d16316c0dbcfa5d09531136fb52ed7ad037c9b6/readme/img/xlibre-nixos.jpg" alt="XLibre running on NixOS"></a><figcaption>XLibre running on NixOS. See more <a href="https://github.com/orgs/X11Libre/discussions/211">liberated screens here</a>.</figcaption>
    </figure>
</p>

This is an independent project, not at all affiliated with BigTech or any of their subsidiaries or tax evasion tools, nor any political activists groups, state actors, etc. It's explicitly free of any "DEI" or similar discriminatory policies. Anybody who's treating others nicely is welcomed.

It doesn't matter which country you're coming from, your political views, your race, your sex, your age, your food menu, whether you wear boots or heels, whether you're furry or fairy, Conan or McKay, comic character, a small furry creature from Alpha Centauri, or just a boring average person. Anybody who's interested in bringing X forward is welcome.

Together we'll make X great again!


## Contact

[XLibre Discussions at Github](https://github.com/orgs/X11Libre/discussions) | [XLibre mailing list at FreeLists](https://www.freelists.org/list/xlibre) | [@x11dev channel at Telegram](https://t.me/x11dev) | [#xlibre room at Matrix](https://matrix.to/#/#xlibre:matrix.org) | [XLibre security contact at Github](https://github.com/X11Libre/xserver/security/policy)

[Interview: Meet Enrico Weigelt, the maintainer of the new XLibre fork - Felipe Contreras](https://felipec.wordpress.com/2025/06/11/enrico-weigelt/)

