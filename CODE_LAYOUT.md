Overview of Code Layout
=======================

Summary of code layout. See `docs/Xserver-spec.xml` for more detail.

Historical Note
---------------

This server implementation began as a reference implementation, created by
the X Protocol developers.


General Structure
-----------------
The server was architected to have a distinct separation between 'Device
Independent' elements (DIX), e.g. protocol decoding, and 'Device Dependent'
elements (DDX), e.g. OS-specific startup.

Generally, DIX code is in the `dix/` and `mi/` trees, and DDX code is in the `hw/`
and `os/` trees, with many extensions being added in individual directories.

Entry Point
-----------

This server uses 'nested' mains.

If OS- or platform-specific code or actions are required as a part of the
startup process, these are to be done in platform-specific main(). These
can be found in:

| Platform          | Entry Point     | File Path                     |
| ----------------- | --------------- | ----------------------------- |
| Unix/Linux/POSIX  | `main()`        | `hw/xfree86/xorg-wrapper.c`   |
| Microsoft Windows | `main()`        | `hw/xwin/InitOutput.c`        |
| MacOS X           | `server_main()` | `hw/xquartz/quartzStartup.c`  |
| Xephyr            | `main()`        | `hw/kdrive/ephyr/ephyrinit.c` |
| Stub/example      | `main()`        | `dix/stubmain.c`              |

The platform-specific main then (ultimately) calls `dix_main()`, which contains
the primary event loop.

`dix_main()` is located in `dix/main.c`

Directories
-----------

Non-exhaustive list of directories and descriptions:

```
.
├── composite - Composite extension.
├── config - Dynamic configuration (DBus, udev, wscons, hal, etc.)
├── damageext - Damage extension.
├── dbe - Double-buffer extension.
├── dix - Cross-platform (DIX) core code.
├── doc - Additional documentation, various formats.
├── dri3 - Direct Rendering Infrastructure v3 Extension
├── exa - Display driver interface spec.
├── fb - Framebuffer extension.
├── glamor - GLAMOR, 2D rendering acceleration (OpenGL/EGL).
├── glx - GLX (OpenGL on X support) extension.
├── hw
│   ├── kdrive - Embedded/small-footprint and X-in-X server.
│   ├── vfb - Virtual framebuffer server.
│   ├── xfree86 - Unix/Linux/POSIX server.
│   ├── xnest - Nested (X-in-X) server.
│   ├── xquartz - MacOS X server.
│   └── xwin - Win32 server.
├── include - Header files for public interface (for drivers and modules).
├── man - Man pages.
├── mi - Machine-independent (low-level) drawing/rendering code.
├── miext - Machine-independent extension code and drawing overrides.
├── os - OS-specific code.
├── present - Present extension.
├── pseudoramiX - Minimal Xinerama implementation for rootless servers.
├── randr - RandR extension.
├── record - Event recording extension.
├── render - Render extension.
├── test - Unit and other tests.
├── Xext - Various extensions.
├── xfixes - Xfixes extension.
├── Xi - Xinput extension.
└── xkb - Xkeyboard extension.
```
