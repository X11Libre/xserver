# Overview of XLibre Server Source Code Structure

The XLibre Server is a modular codebase that implements a X Window System (X11)
server. It's written in C, and supports a wide variety of platforms, hardware
configurations, and extensions.

The codebase is organized into:
 - OS abstractions,
 - core components (device-independent and machine-independent code),
 - extensions,
 - hardware drivers and servers (device-dependant code), and
 - supporting utilities.

The Meson build system orchestrates compilation, handling dependencies,
conditional builds, and configuration options.

## Key Principles of the Codebase

**Modularity**: Code is split into libraries (e.g., `libxserver_*`) that are
linked into different server binaries (`Xorg`, `Xnest`, `Xquartz`, etc.).

**Platform Independence**: Device Independent X (DIX) and Machine Independent
(MI) layers abstract hardware and OS differences.

**Extensions**: Many features (e.g., `RandR`, `Composite`) are implemented as
loadable extensions. These can depend on one another, and many rely on
`Render` (render/) for drawing.

**Build Flexibility**: Meson options allow enabling/disabling features, servers,
and extensions, with auto-detection for many.

## Diagram of XLibre

Below is a visual overview of how the components of the XLibre Server fit
together. This diagram should be read from the bottom up to understand how each
layer builds on those below.

```
       +==========================================================+
       |                DDX (Hardware and Servers)                |
       +==========================================================+
       | Xorg (hw/xfree86/: modes, drivers, int10)                |
       +----------------------------+-----------------------------+
       | Xnest (hw/xnest/: nested)    | Xvfb (hw/vfb/: virtual)   |
       +------------------------------+---------------------------+
       | Xquartz (hw/xquartz/: macOS) | XWin (hw/xwin/: Windows)  |
       +----------------------------------------------------------+
                                    |
                                    |
   +=================================================================+
   |                    Extensions & Acceleration                    |
   +=================================================================+
   | Others: XFixes, Record, DBE, Sync (miext/sync), DRI3, etc.      |
   |         Xnamespace (Xext/namespace/: isolation)                 |
   +-----------------------+------------------+----------------------+
   | XInput (Xi/: devices) | XKB (xkb/: keys) | GLX (glx/: OpenGL)   |
   +-----------------------+----------------+-+----------------------+
   | Glamor (glamor/: OpenGL 2D accel.)     | EXA (exa/: 2D accel.)  |
   +--------------+------------+------------+------------------------+
   | Composite    | Damage     | Present    | RandR    | Xinerama    |
   | (effects)    | (tracking) | (vsync)    | (resize) | (monitors)  |
   +--------------+------------+------------+------------------------+
   | Render (render/: 2D primitives, glyphs, etc.)                   |
   | --> Core rendering, used by Composite, Glamor, etc.             |
   +-----------------------------------------------------------------+
                                    |
                                    |
   +=================================================================+
   |                        DIX (Core Layers)                        |
   +=================================================================+
   | DIX (dix/: dispatch, events, windows, resources)                |
   | --> Central hub for requests; extensions register here.         |
   +-----------------------------------------------------------------+
   | MI (mi/: generic draw, GC, sprites)   | FB (fb/: framebuffer)   |
   +---------------------------------------+-------------------------+
                                    |
                                    |
       +==========================================================+
       |                    OS Abstraction Layer                  |
       +==========================================================+
       | OS (os/: sockets, I/O, auth, etc.)                       |
       | --> Provides poll, log, connection, etc. for all above.  |
       +----------------------------------------------------------+

 +=======================================================================+
 |      External Deps (Protocols/Libs: xproto, pixman, libdrm, etc.)     |
 +=======================================================================+
 | Protocols (xorgproto) | Rendering (pixman, etc.) | OS (libudev, etc.) |
 +-----------------------------------------------------------------------+
```

The flow of a typical request is:

 1. Client request arrives (via OS layer)
 2. DIX dispatches request (Core op? Handle in DIX. No? Check extensions.)
 3. DIX/MI/FB/Extension handles request (including input, rendering, etc.)
 4. Output via DDX (hardware or simulated)

## Top-Level Directory Layout

The source tree is organized by functional areas. Below is an *introductory*
summary of each directory; this is a starting point, not a comprehensive
explanation.

### Core

- **config/**: Server configuration handling  
    Manages server configuration parsing and hotplugging, including input
    devices, monitors, and modules via files like `config.c` and `udev.c`. It
    interconnects with the input extensions (e.g., Xi/ for device detection)
    and OS layer (os/ for platform-specific I/O), relying on optional
    dependencies like libudev for dynamic device handling on Linux.

- **dix/**: Device Independent X (DIX) - Core server logic  
    Orchestrates protocol dispatching, event handling, resource management,
    and window operations through files like `dispatch.c` and `main.c`. It
    serves as the central hub, interconnecting with all extensions (which
    register here via `extension.c`) and the MI layer (mi/ for generic
    implementations). Depends on protocols from xorgproto for request routing.
    Key components include: Request handlers, property system, colormap
    management, etc.

- **hw/**: Device Dependant X (DDX) - Hardware-specific code and servers  
    Hardware-dependent drivers and server variants, including subdirectories
    like `xfree86/` (the main XLibre DDX), `kdrive/` (includes Xephyr),
    `vfb/`, `xnest/`, `xquartz/` (macOS), and `xwin/` (Windows). These link
    the various XLibre libraries/components (libxserver_*) into executables.
    
    NOTE: Drivers (e.g., modesetting, intel, amd) are currently in separate
    repositories and are loaded as modules. There is an ongoing discussion
    about moving these into the core source tree in the future.

- **include/**: Global headers and configuration  
    Global headers and generated configs (like `dix-config.h`), defining core
    structures (e.g., `dixstruct.h`) and macros (e.g., `misc.h`) used across
    the codebase. This directory interconnects with nearly every component as
    an inclusion base, supporting platform independence by abstracting types
    and configs for DIX, MI, and extensions.

- **fb/**: Framebuffer abstraction  
    A software framebuffer abstraction that implements generic rendering
    primitives (e.g., `fbpict.c` for pictures) as a fallback when hardware
    acceleration is unavailable. It interconnects closely with the MI layer
    (mi/ for drawing ops) and Render extension (render/ for accelerated
    paths). Depends on pixman for pixel manipulation.

- **mi/**: Machine Independent (MI) - Generic implementations  
    The Machine Independent layer offers generic, non-hardware-specific
    implementations for drawing, window management, and graphics contexts
    via files like `miwindow.c` and `migc.c`. It acts as a fallback interconnect
    between DIX (for protocol handling) and FB (for rendering), extended via
    miext/ for specialized MI features like damage tracking.

- **os/**: OS abstraction layer  
    The OS abstraction layer manages platform-agnostic I/O, sockets, signals,
    timers, and authentication (e.g., `connection.c` and `xdmcp.c` if enabled).
    It interconnects as the foundational bridge for all upper layers, supporting
    DIX events and extensions via polling (`ospoll.c`). Optionally depends on
    SHA1 providers for authentication and dbus for systemd integration.

### Rendering Acceleration

- **exa/**: EXA acceleration  
    EXA provides an older framework for 2D hardware acceleration, handling
    operations like rendering and glyphs through e.g., `exa_render.c`. It
    provides a fallback to Glamor. It interconnects with the Render extension
    for primitives. Primarily used in XLibre or Xephyr builds. Depends on
    pixman.

- **glamor/**: Glamor acceleration  
    Enables OpenGL-based 2D acceleration as a modern replacement for EXA, with
    core logic in `glamor_render.c` for efficient drawing and XV support. It
    interconnects with the Render extension for primitives and GLX for OpenGL.
    Depends on epoxy and gbm for hardware access.

### Extensions

- **composite/**: Composite extension  
    Supports window redirection and compositing effects (e.g., transparency)
    via files like `compwindow.c`. It interconnects with Render (for drawing)
    and Damage (for efficiency).

- **damageext/**: Damage extension  
    Tracks damaged screen regions for optimized redraws, implemented in
    `damageext.c`. It interconnects with miext/damage/ for MI-level tracking
    and Composite for effects. Depends on damageproto.

- **dbe/**: Double Buffer Extension (DBE)  
    Provides double-buffering to minimize flicker in drawing operations, with
    `midbe.c` offering MI support. It interconnects as a simple extension to DIX
    for protocol handling, and optionally integrats with Render for buffered
    primitives.

- **dri3/**: Direct Rendering Infrastructure 3 (DRI3) extension  
    Facilitates modern buffer sharing for direct rendering, handled in e.g.,
    `dri3_request.c`. It interconnects with GLX for OpenGL and hw/xfree86/dri/
    for hardware. Depends on libdrm and xshmfence.

- **glx/**: OpenGL Extension to X (GLX)  
    Extends X for OpenGL applications, dispatching commands via `glxcmds.c`.
    It interconnects with Glamor/DRI for acceleration and Render for 2D ops.

- **miext/**: MI Extensions  
    Contains Machine Independent extensions for specialized features like:
     - `damage` (MI damage tracking)
     - `rootless` (rootless windowing for Xquartz)
     - `shadow` (shadow framebuffer)
     - `sync` (fence synchronization)
    Interconnects as helpers for core MI (mi/) and extensions like Damage or
    Rootless (for Xquartz)

- **present/**: Present extension  
    Manages vsync and buffer flipping for smooth graphics, via e.g.,
    `present_vblank.c`. It interconnects with DRI for hardware sync and RandR
    for screen ops. A modern alternative to Xv. Depends on presentproto.

- **pseudoramiX/**: PseudoramiX  
    Emulates/Provides multi-monitor support for specific servers, implemented in
    `pseudoramiX.c`. It interconnects with Xwin or Xquartz servers for platform
    multi-head setups, integrating with RandR for compatibility in non-native
    environments.

- **randr/**: Resize and Rotate (RandR) extension  
    RandR handles dynamic screen resizing, rotation, and multi-monitor configs
    via files like `rrtransform.c` and `rrcrtc.c`. It interconnects with
    hw/xfree86/modes/ for hardware modesetting and Render for drawing. Depends
    on randrproto.

- **record/**: Record extension  
    Captures protocol streams for testing or debugging, via `record.c`. It
    interconnects with DIX for request interception. Depends on recordproto.

- **render/**: Render extension  
    Accelerates 2D operations like glyphs and gradients via files like
    `render.c` and `picture.c`. As a central extension, it interconnects with
    Composite/Damage for effects, Glamor/EXA for acceleration, and FB for
    fallback. Depends on pixman.

- **Xext/**: Core X extensions  
    Bundles core extensions like SHM (`shm.c`), Sync (`sync.c`), BigRequests
    (`bigreq.c`), VidMode (`vidmode.c`), Xinerama (`panoramiX.c`), and others.
    It interconnects broadly with DIX for base protocol enhancements and miext/
    for MI support. In XLibre, namespace/ adds client isolation/containers.

- **xfixes/**: X Fixes extension  
    Protocol fixes and enhancements, like cursor confinement via `cursor.c`.
    It interconnects with XInput for input tweaks and Render for drawing.
    Depends on fixesproto.

- **Xi/**: X Input extension  
    Manages advanced input devices, touch, and gestures, e.g. through
    `xiquerydevice.c`. It interconnects with config/ for hotplugging and
    XKB for keyboard integration.

- **xkb/**: X Keyboard (XKB) extension  
    Oversees keyboard layouts, mappings, and actions, e.g. in `xkbActions.c`
    and `xkbEvents.c`. It interconnects with Xi/ for input devices and config/
    for rules loading. Depends on kbproto.

### Documentation and Tests

- **doc/**: Documentation
- **man/**: Manual pages for the server and tools
- **test/**: Unit/integration tests

### Other Files

- **Meson.build** and **meson_options.txt** provide build process and
  configuration.

- **include/dix-config.h** is generated as part of the build process.

## Key Information for Working on the Code

- **Entry Point**: Server starts in `dix/main.c` (Dispatch loop).
  Extensions load via `dix/extension.c`.

- **Important Headers**: `include/dix-config.h` (config macros),
  `dix/dispatch.h` (request handlers), `os/osdep.h` (OS funcs).

- **X11 Protocol**: Read the `xorgproto` docs. Core is in DIX,
  extensions add opcodes.

- **Resources**: Read `doc/` output. Read Xorg docs at
  (freedesktop.org/wiki/Xorg)[freedesktop.org/wiki/Xorg]. Check GitHub issues
  and discussions.

- **Modules**: Drivers and extensions load dynamically from
  `module_dir` (default: lib/xorg/modules).

- **Debugging**: Use `-verbose` and `-logverbose` server flags for detailed
  output and logging. Enable libunwind for better backtraces. Enable
  AddressSanitizer in your build. Use tools like gdb, valgrind, perf, etc. 
  
  Use Xephyr for isolated testing in a nested X11 window.
  
  For debugging on real hardware, you need functional input devices
  (keyboard/mouse) to interact with the server or switch back to a text console
  (VT). Input is provided by separate driver modules (e.g., by
  [xf86-input-libinput](https://github.com/X11Libre/xf86-input-libinput) for
  modern devices via libinput), which need to be "installed" with your
  development build.
  
  For example, for `xf86-input-libinput` place `libinput_drv.so` in
  `your_prefix/lib64/xorg/modules/xlibre-25/input` (you can copy this from your
  main XLibre `usr/lib/` directory as long as it is ABI-compatible) and place
  `80-libinput.conf` in `your_prefix/share/X11/xorg.conf.d/`.
  
  For gdb debugging, it is highly recommended to use SSH from another machine or
  a serial terminal to avoid input lockup during breakpoints, which prevents
  local VT switching and could freeze your session.

- **Contributing and Community Interaction**: See the `CONTRIBUTING.md` and
  `README.md` files for additional information.
