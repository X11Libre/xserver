# X11 Application Support

## Introduction

Being able to run existing X11 applications is crucial for the adoption of
Wayland, especially on desktops, as there will always be X11 applications that
have not been or cannot be converted into Wayland applications, and throwing
them all away would be prohibitive. Therefore a Wayland compositor often needs
to support running X11 applications.

X11 and Wayland are different enough that there is no "simple" way to translate
between them. Most of X11 is uninteresting to a Wayland compositor. That,
combined with the gigantic implementation effort needed to support X11, makes it
intractable to just write X11 support directly in a Wayland compositor. The
implementation would be nothing short of a real X11 server.

Therefore, Wayland compositors should use Xwayland, the X11 server that lives in
the Xorg server source code repository and shares most of the implementation
with the Xorg server. Xwayland is a complete X11 server, just like Xorg is, but
instead of driving the displays and opening input devices, it acts as a Wayland
client. The rest of this chapter talks about how Xwayland works.

For integration and architecture reasons, while Xwayland is a Wayland client of
the Wayland compositor, the Wayland compositor is an X11 client of Xwayland.
This circular dependency requires special care from the Wayland compositor.

## Two Modes for Foreign Windows

In general, windows from a foreign window system can be presented in one of two
ways: rootless and rootful (not rootless).

In rootful mode, the foreign window system as a whole is represented as a window
(or more) of its own. You have a native window, inside which all the foreign
windows are. The advantage of this approach in Xwayland's case is that you can
run your favourite X11 window manager to manage your X11 applications. The
disadvantage is that the foreign windows do not integrate with the native
desktop. Therefore this mode is not usually used.

In rootless mode, each foreign window is a first-class resident among the native
windows. Foreign windows are not confined inside a native window but act as if
they were native windows. The advantage is that one can freely stack and mix
native and foreign windows, which is not possible in rootful mode. The
disadvantage is that this mode is harder to implement and fundamental
differences in window systems may prevent some things from working. With
rootless Xwayland, the Wayland compositor must take the role as the X11 window
manager, and one cannot use any other X11 window manager in its place.

This chapter concentrates on the rootless mode, and ignores the rootful mode.

## Architecture

A Wayland compositor usually takes care of launching Xwayland. Xwayland works in
cooperation with a Wayland compositor as follows:

**Xwayland architecture diagram**

![](images/xwayland-architecture.png)

An X11 application connects to Xwayland just like it would connect to any X
server. Xwayland processes all the X11 requests. On the other end, Xwayland is a
Wayland client that connects to the Wayland compositor.

The X11 window manager (XWM) is an integral part of the Wayland compositor. XWM
uses the usual X11 window management protocol to manage all X11 windows in
Xwayland. Most importantly, XWM acts as a bridge between Xwayland window state
and the Wayland compositor's window manager (WWM). This way WWM can manage all
windows, both native Wayland and X11 (Xwayland) windows. This is very important
for a coherent user experience.

Since Xwayland uses Wayland for input and output, it does not have any use for
the device drivers that Xorg uses. None of the xf86-video-* or xf86-input-*
modules are used. There also is no configuration file for the Xwayland server.
For optional hardware accelerated rendering, Xwayland uses GLAMOR.

A Wayland compositor usually spawns only one Xwayland instance. This is because
many X11 applications assume they can communicate with other X11 applications
through the X server, and this requires a shared X server instance. This also
means that Xwayland does not protect nor isolate X11 clients from each other,
unless the Wayland compositor specifically chooses to break the X11 client
intercommunications by spawning application specific Xwayland instances. X11
clients are naturally isolated from Wayland clients.

Xwayland compatibility compared to a native X server will probably never reach
100%. Desktop environment (DE) components, specifically X11 window managers, are
practically never supported. An X11 window manager would not know about native
Wayland windows, so it could manage only X11 windows. On the other hand, there
must be an XWM that reserves the exclusive window manager role so that the
Wayland compositor could show the X11 windows appropriately. For other DE
components, like pagers and panels, adding the necessary interfaces to support
them in WWM through XWM is often considered not worthwhile.

## X Window Manager (XWM)

From the X11 point of view, the X window manager (XWM) living inside a Wayland
compositor is just like any other window manager. The difference is mostly in
which process it resides in, and the few extra conventions in the X11 protocol
to support Wayland window management (WWM) specifically.

There are two separate asynchronous communication channels between Xwayland and
a Wayland compositor: one uses the Wayland protocol, and the other one, solely
for XWM, uses X11 protocol. This setting demands great care from the XWM
implementation to avoid (random) deadlocks with Xwayland. It is often nearly
impossible to prove that synchronous or blocking X11 calls from XWM cannot cause
a deadlock, and therefore it is strongly recommended to make all X11
communications asynchronous. All Wayland communications are already asynchronous
by design.

### Window identification

In Xwayland, an X11 window may have a corresponding wl_surface object in
Wayland. The wl_surface object is used for input and output: it is referenced by
input events and used to provide the X11 window content to the Wayland
compositor. The X11 window and the wl_surface live in different protocol
streams, and they need to be matched for XWM to do its job.

When Xwayland creates a wl_surface on Wayland, it will also send an X11
ClientMessage of type atom "WL_SURFACE_ID" to the X11 window carrying the
wl_surface Wayland object ID as the first 32-bit data element. This is how XWM
can associate a wl_surface with an X11 window. Note that the request to create a
wl_surface and the ID message may arrive in any order in the Wayland compositor.
