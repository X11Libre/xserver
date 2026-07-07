# Color management

## Overview

Color management in Wayland considers only displays. All pictures in Wayland are
always display-referred, meaning that the pixel values are intended as-is for
some specific display where they would produce the light emissions
([stimuli](https://cie.co.at/eilvterm/17-23-002)) the picture's author desired.
Wayland does not support displaying "raw" camera or scanner images as they are
not display-referred, nor are they even pictures without complex and subjective
processing.

Stimuli — the picture itself — are only half of the picture reproduction. The
other half is the environment where a display is viewed. A striking example is
comparing a brightly lit office to a dark movie theater, the stimuli required to
produce a good reading of the picture is greatly different. Therefore
display-referred does not include only the display but the viewing environment
as well.

Window systems have been very well capable of operating without any explicit
consideration to color management. This is because there used to be the implicit
assumption of the standard display, the sRGB display, which all computer
monitors implemented, more or less. The viewing environment was and still is
accounted by adjusting the display and/or the room to produce a workable
experience. Pictures are authored on a computer system by drawing, painting and
adjusting the picture until it looks right on the author's monitor. This
implicitly builds the standard display and environment assumption into the
picture data. Deviations from the sRGB specification were minor enough that they
often did not matter if not in a professional context like the printing
industry. Displaying video material required some more attention to the details,
because video and television standards differ enough from the sRGB display. What
really made explicit color management a hard requirement for entertainment is
the coming of wide color gamut (WCG) and high dynamic range (HDR) materials and
displays.

The color management design in Wayland follows the general Wayland design
principles: compositors tell clients what would be the optimal thing to do,
clients tell the compositors what kind of pictures they are actually producing,
and then compositors display those pictures the best they can.

## Protocol Interfaces

Color management interfaces in Wayland and divided into two protocols:
[color-management](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/tree/main/staging/color-management?ref_type=heads)
and
[color-representation](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/tree/main/staging/color-representation?ref_type=heads).
They are designed to work together, but they can also be used independently when
the other one is not needed.

### Color-management

Color management protocol has two main purposes. First, it puts the
responsibility of color management on the compositor. This means that clients do
not necessarily need to care about color management at all, and can display just
fine by using the traditional standard display assumption even when the actual
display is wildly different. Clients can also choose to target some other
assumed display and let the compositor handle it, or they can explicitly render
for the actual display at hand. Second, when the window system has multiple
different monitors, and a wl_surface happens to span more than one monitor, the
compositor can display the surface content correctly on all spanned monitors
simultaneously, as much as physically possible.

Color-management protocol concentrates on colorimetry: when you have a pixel
with RGB values, what stimulus do those values represent. The stimulus
definition follows the CIE 1931 two-degree observer model. Some core concepts
here are color primaries, white point, transfer function, and dynamic range. The
viewing environment is represented in an extremely simplified way as the
reference white luminance. The connection between pixel RGB values and stimulus
plus viewing environment is recorded in an _image description_ object. Clients
can create image description objects and tag `wl_surface`s with them, to
indicate what kind of surface content there will be. Clients can also ask what
image description the compositor would prefer to have on the `wl_surface`, and
that preference can change over time, e.g. when the `wl_surface` is moved from
one `wl_output` to another. Following the compositor's preference may provide
advantages in image quality and power consumption.

Image description objects can come in two flavors: parametric and ICC-based. The
above was written with parametric image descriptions in mind, and they have
first-class support for HDR. ICC-based image descriptions are wrapping an ICC
profile and have no other data. ICC profiles are the standard tool for standard
dynamic range (SDR) display color management. This means the capabilities
between the two flavors differ, and one cannot always be replaced by the other.
Compositor support for each flavor is optional.

### Color-representation

Color-representation protocol deals with (potentially sub-sampled) YCbCr-RGB
conversion, quantization range, and the inclusion of alpha in the RGB color
channels, a.k.a. pre-multiplication. There are several different specifications
on how an YCbCr-like (including ICtCp) signal, with chroma sub-sampling or not,
is created from a full-resolution RGB image. Again, a client can tag a
`wl_surface` with color-representation metadata to tell the compositor what kind
of pixel data will be displayed through the wl_surface.

The main purpose of color-representation is to correctly off-load the YCbCr-RGB
conversion to the compositor, which can then opportunistically off-load it
further to very power-efficient fixed-function circuitry in a display
controller. This can significantly reduce power consumption when watching videos
compared to using a GPU for the same, and on some embedded hardware platforms it
is a hard requirement for processing high resolution video.
