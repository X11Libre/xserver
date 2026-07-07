# Types of Compositors

Compositors come in different types, depending on which role they play in the
overall architecture of the OS. For instance, a [system
compositor](#system-compositor) can be used for booting the system, handling
multiple user switching, a possible console terminal emulator and so forth. A
different compositor, a [session compositor](#session-compositor) would provide
the actual desktop environment. There are many ways for different types of
compositors to co-exist.

In this section, we introduce three types of Wayland compositors relying on
[libwayland-server](https://gitlab.freedesktop.org/wayland/wayland).

## System Compositor

A system compositor can run from early boot until shutdown. It effectively
replaces the kernel vt system, and can tie in with the systems graphical boot
setup and multiseat support.

A system compositor can host different types of session compositors, and let us
switch between multiple sessions (fast user switching, or secure/personal
desktop switching).

A linux implementation of a system compositor will typically use libudev, egl,
kms, evdev and cairo.

For fullscreen clients, the system compositor can reprogram the video scanout
address to read directly from the client provided buffer.

## Session Compositor

A session compositor is responsible for a single user session. If a system
compositor is present, the session compositor will run nested under the system
compositor. Nesting is feasible because the protocol is asynchronous; roundtrips
would be too expensive when nesting is involved. If no system compositor is
present, a session compositor can run directly on the hardware.

X applications can continue working under a session compositor by means of a
root-less X server that is activated on demand.

Possible examples for session compositors include

- gnome-shell

- moblin

- kwin

- kmscon

- rdp session

- Weston with X11 or Wayland backend is a session compositor nested in another
  session compositor.

- fullscreen X session under Wayland

## Embedding Compositor

X11 lets clients embed windows from other clients, or lets clients copy pixmap
contents rendered by another client into their window. This is often used for
applets in a panel, browser plugins and similar. Wayland doesn't directly allow
this, but clients can communicate GEM buffer names out-of-band, for example,
using D-Bus, or command line arguments when the panel launches the applet.
Another option is to use a nested Wayland instance. For this, the Wayland server
will have to be a library that the host application links to. The host
application will then pass the Wayland server socket name to the embedded
application, and will need to implement the Wayland compositor interface. The
host application composites the client surfaces as part of its window, that is,
in the web page or in the panel. The benefit of nesting the Wayland server is
that it provides the requests the embedded client needs to inform the host about
buffer updates and a mechanism for forwarding input events from the host
application.

An example for this kind of setup is firefox embedding the flash player as a
kind of special-purpose compositor.
