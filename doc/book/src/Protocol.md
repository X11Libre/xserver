# Wayland Protocol and Model of Operation

## Basic Principles

The Wayland protocol is an asynchronous object oriented protocol. All requests
are method invocations on some object. The requests include an object ID that
uniquely identifies an object on the server. Each object implements an interface
and the requests include an opcode that identifies which method in the interface
to invoke.

The protocol is message-based. A message sent by a client to the server is
called request. A message from the server to a client is called event. A message
has a number of arguments, each of which has a certain type (see [Wire
Format](#wire-format) for a list of argument types).

Additionally, the protocol can specify `enum`s which associate names to specific
numeric enumeration values. These are primarily just descriptive in nature: at
the wire format level enums are just integers. But they also serve a secondary
purpose to enhance type safety or otherwise add context for use in language
bindings or other such code. This latter usage is only supported so long as code
written before these attributes were introduced still works after; in other
words, adding an enum should not break API, otherwise it puts backwards
compatibility at risk.

`enum`s can be defined as just a set of integers, or as bitfields. This is
specified via the `bitfield` boolean attribute in the `enum` definition. If this
attribute is true, the enum is intended to be accessed primarily using bitwise
operations, for example when arbitrarily many choices of the enum can be ORed
together; if it is false, or the attribute is omitted, then the enum arguments
are a just a sequence of numerical values.

The `enum` attribute can be used on either `uint` or `int` arguments, however if
the `enum` is defined as a `bitfield`, it can only be used on `uint` args.

The server sends back events to the client, each event is emitted from an
object. Events can be error conditions. The event includes the object ID and the
event opcode, from which the client can determine the type of event. Events are
generated both in response to requests (in which case the request and the event
constitutes a round trip) or spontaneously when the server state changes.

- State is broadcast on connect, events are sent out when state changes. Clients
  must listen for these changes and cache the state. There is no need (or
  mechanism) to query server state.

- The server will broadcast the presence of a number of global objects, which in
  turn will broadcast their current state.

## Code Generation

The interfaces, requests and events are defined in
[protocol/wayland.xml](https://gitlab.freedesktop.org/wayland/wayland/-/blob/main/protocol/wayland.xml).
This xml is used to generate the function prototypes that can be used by clients
and compositors.

The protocol entry points are generated as inline functions which just wrap the
`wl_proxy_*` functions. The inline functions aren't part of the library ABI and
language bindings should generate their own stubs for the protocol entry points
from the xml.

## Wire Format

The protocol is sent over a UNIX domain stream socket, where the endpoint
usually is named `wayland-0` (although it can be changed via _WAYLAND_DISPLAY_
in the environment). Beginning in Wayland 1.15, implementations can optionally
support server socket endpoints located at arbitrary locations in the filesystem
by setting _WAYLAND_DISPLAY_ to the absolute path at which the server endpoint
listens. The socket may also be provided through file descriptor inheritance, in
which case _WAYLAND_SOCKET_ is set.

Every message is structured as 32-bit words; values are represented in the
host's byte-order. The message header has 2 words in it:

- The first word is the sender's object ID (32-bit).

- The second has 2 parts of 16-bit. The upper 16-bits are the message size in
  bytes, starting at the header (i.e. it has a minimum value of 8).The lower is
  the request/event opcode.

The payload describes the request/event arguments. Every argument is always
aligned to 32-bits. Where padding is required, the value of padding bytes is
undefined. There is no prefix that describes the type, but it is inferred
implicitly from the xml specification.

The representation of argument types are as follows:

int
uint
  : The value is the 32-bit value of the signed/unsigned int.

fixed
  : Signed 24.8 decimal numbers. It is a signed decimal type which offers a sign
    bit, 23 bits of integer precision and 8 bits of decimal precision. This is
    exposed as an opaque struct with conversion helpers to and from double and
    int on the C API side.

string
  : Starts with an unsigned 32-bit length (including null terminator), followed
    by the UTF-8 encoded string contents, including terminating null byte, then
    padding to a 32-bit boundary. A null value is represented with a length of
    0. Interior null bytes are not permitted.

object
  : 32-bit object ID. A null value is represented with an ID of 0.

new_id
  : The 32-bit object ID. Generally, the interface used for the new object is
    inferred from the xml, but in the case where it's not specified, a new_id is
    preceded by a `string` specifying the interface name, and a `uint`
    specifying the version.

array
  : Starts with 32-bit array size in bytes, followed by the array contents
    verbatim, and finally padding to a 32-bit boundary.

fd
  : The file descriptor is not stored in the message buffer, but in the
    ancillary data of the UNIX domain socket message (msg_control).

The protocol does not specify the exact position of the ancillary data in the
stream, except that the order of file descriptors is the same as the order of
messages and `fd` arguments within messages on the wire.

In particular, it means that any byte of the stream, even the message header,
may carry the ancillary data with file descriptors.

Clients and compositors should queue incoming data until they have whole
messages to process, as file descriptors may arrive earlier or later than the
corresponding data bytes.

## Versioning

Every interface is versioned and every protocol object implements a particular
version of its interface. For global objects, the maximum version supported by
the server is advertised with the global and the actual version of the created
protocol object is determined by the version argument passed to
wl_registry.bind(). For objects that are not globals, their version is inferred
from the object that created them.

In order to keep things sane, this has a few implications for interface
versions:

- The object creation hierarchy must be a tree. Otherwise, inferring object
  versions from the parent object becomes a much more difficult to properly
  track.

- When the version of an interface increases, so does the version of its parent
  (recursively until you get to a global interface)

- A global interface's version number acts like a counter for all of its child
  interfaces. Whenever a child interface gets modified, the global parent's
  interface version number also increases (see above). The child interface then
  takes on the same version number as the new version of its parent global
  interface.

To illustrate the above, consider the wl_compositor interface. It has two
children, wl_surface and wl_region. As of wayland version 1.2, wl_surface and
wl_compositor are both at version 3. If something is added to the wl_region
interface, both wl_region and wl_compositor will get bumpped to version 4. If,
afterwards, wl_surface is changed, both wl_compositor and wl_surface will be at
version 5. In this way the global interface version is used as a sort of
"counter" for all of its child interfaces. This makes it very simple to know the
version of the child given the version of its parent. The child is at the
highest possible interface version that is less than or equal to its parent's
version.

It is worth noting a particular exception to the above versioning scheme. The
wl_display (and, by extension, wl_registry) interface cannot change because it
is the core protocol object and its version is never advertised nor is there a
mechanism to request a different version.

## Connect Time

When a client connects, the server sends no data but silently provides access to
the singleton `wl_display` under object ID 1. Global objects are managed through
an instance of [wl_registry](#registry).

## Security and Authentication

- mostly about access to underlying buffers, need new drm auth mechanism (the
  grant-to ioctl idea), need to check the cmd stream?

- getting the server socket depends on the compositor type, could be a system
  wide name, through fd passing on the session dbus. or the client is forked by
  the compositor and the fd is already opened.

## Creating Objects

Each object has a unique ID. The IDs are allocated by the entity creating the
object (either client or server). IDs allocated by the client are in the range
[2, 0xfeffffff] while IDs allocated by the server are in the range [0xff000000,
0xffffffff]. The 0 ID is reserved to represent a null or non-existent object and
ID 1 is automatically assigned to the singleton `wl_display` instance when a
client connects. For efficiency purposes, the IDs are densely packed in the
sense that the ID N will not be used until N-1 has been used. This ordering is
not merely a guideline, but a strict requirement, and there are implementations
of the protocol that rigorously enforce this rule, including the ubiquitous
libwayland.

## Display

The display is a singleton global object and always has the ID 1. Clients often
call its `get_registry` method after connecting to the server to obtain the
object registry.

See [wl_display](https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_display)
for the protocol description.

## Registry

The registry is a global object, instantiated using
[`wl_display.get_registry`](https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_display-request-get_registry),
that acts as the interface to Wayland's object system between client and server:
it initially informs the client of all globals available on the server,
announces to the client whenever objects are added to or removed from the
server, and allows the client to bind to the objects to interact with them.

After instantiating a new registry, the server advertises all available objects
to the client using
[`wl_registry.global`](https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_registry-event-global).
Thereafter, whenever a new object is added, the server advertises it to its
clients using `wl_registry.global`, and whenever an object is removed, this fact
is advertised using
[`wl_registry.global_remove`](https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_registry-event-global_remove).

See [wl_registry](https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_registry)
for the protocol description.

## Compositor

The compositor is a global object, advertised through the registry.

See [wl_compositor](https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_compositor)
for the protocol description.

## Surfaces

A surface manages a rectangular grid of pixels that clients create for
displaying their content to the screen. Clients don't know the global position
of their surfaces, and cannot access other clients' surfaces.

Once the client has finished writing pixels, it 'commits' the buffer; this
permits the compositor to access the buffer and read the pixels. When the
compositor is finished, it releases the buffer back to the client.

See [wl_surface](https://wayland.app/protocols/wayland#wl_surface) for the
protocol description.

## Input

A seat represents a group of input devices including mice, keyboards and
touchscreens. It has a keyboard and pointer focus. Seats are global objects.
Pointer events are delivered in surface-local coordinates.

The compositor maintains an implicit grab when a button is pressed, to ensure
that the corresponding button release event gets delivered to the same surface.
But there is no way for clients to take an explicit grab. Instead, surfaces can
be mapped as 'popup', which combines transient window semantics with a pointer
grab.

To avoid race conditions, input events that are likely to trigger further
requests (such as button presses, key events, pointer motions) carry serial
numbers, and requests such as wl_surface.set_popup require that the serial
number of the triggering event is specified. The server maintains a
monotonically increasing counter for these serial numbers.

Input events also carry timestamps with millisecond granularity. Their base is
undefined, so they can't be compared against system time (as obtained with
clock_gettime or gettimeofday). They can be compared with each other though, and
for instance be used to identify sequences of button presses as double or triple
clicks.

See [wl_seat](https://wayland.app/protocols/wayland#wl_seat) for the protocol
description.

Talk about:

- keyboard map, change events

- xkb on Wayland

- multi pointer Wayland

A surface can change the pointer image when the surface is the pointer focus of
the input device. Wayland doesn't automatically change the pointer image when a
pointer enters a surface, but expects the application to set the cursor it wants
in response to the pointer focus and motion events. The rationale is that a
client has to manage changing pointer images for UI elements within the surface
in response to motion events anyway, so we'll make that the only mechanism for
setting or changing the pointer image. If the server receives a request to set
the pointer image after the surface loses pointer focus, the request is ignored.
To the client this will look like it successfully set the pointer image.

Setting the pointer image to NULL causes the cursor to be hidden.

The compositor will revert the pointer image back to a default image when no
surface has the pointer focus for that device.

What if the pointer moves from one window which has set a special pointer image
to a surface that doesn't set an image in response to the motion event? The new
surface will be stuck with the special pointer image. We can't just revert the
pointer image on leaving a surface, since if we immediately enter a surface that
sets a different image, the image will flicker. If a client does not set a
pointer image when the pointer enters a surface, the pointer stays with the
image set by the last surface that changed it, possibly even hidden. Such a
client is likely just broken.

## Output

An output is a global object, advertised through the registry. Outputs may be
added or removed during the lifetime of the server.

See [wl_output](https://wayland.app/protocols/wayland#wl_output) for the
protocol description.

- laid out in a big (compositor) coordinate system

- basically xrandr over Wayland

- geometry needs position in compositor coordinate system

- events to advertise available modes, requests to move and change modes

## Data sharing between clients

The Wayland protocol provides clients a mechanism for sharing data that allows
the implementation of copy-paste and drag-and-drop. The client providing the
data creates a `wl_data_source` object and the clients obtaining the data will
see it as `wl_data_offer` object. This interface allows the clients to agree on
a mutually supported mime type and transfer the data via a file descriptor that
is passed through the protocol.

The next section explains the negotiation between data source and data offer
objects. [Data devices](#data-devices) explains how these objects are created
and passed to different clients using the `wl_data_device` interface that
implements copy-paste and drag-and-drop support.

See [wl_data_offer](https://wayland.app/protocols/wayland#wl_data_offer),
[wl_data_source](https://wayland.app/protocols/wayland#wl_data_source),
[wl_data_device](https://wayland.app/protocols/wayland#wl_data_device) and
[wl_data_device_manager](https://wayland.app/protocols/wayland#wl_data_device_manager)
for protocol descriptions.

MIME is defined in RFC's 2045-2049. A [registry of MIME
types](https://www.iana.org/assignments/media-types/media-types.xhtml) is
maintained by the Internet Assigned Numbers Authority (IANA).

### Data negotiation

A client providing data to other clients will create a `wl_data_source` object
and advertise the mime types for the formats it supports for that data through
the `wl_data_source.offer` request. On the receiving end, the data offer object
will generate one `wl_data_offer.offer` event for each supported mime type.

The actual data transfer happens when the receiving client sends a
`wl_data_offer.receive` request. This request takes a mime type and a file
descriptor as arguments. This request will generate a `wl_data_source.send`
event on the sending client with the same arguments, and the latter client is
expected to write its data to the given file descriptor using the chosen mime
type.

### Data devices

Data devices glue data sources and offers together. A data device is associated
with a `wl_seat` and is obtained by the clients using the
`wl_data_device_manager` factory object, which is also responsible for creating
data sources.

Clients are informed of new data offers through the `wl_data_device.data_offer`
event. After this event is generated the data offer will advertise the available
mime types. New data offers are introduced prior to their use for copy-paste or
drag-and-drop.

#### Selection

Each data device has a selection data source. Clients create a data source
object using the device manager and may set it as the current selection for a
given data device. Whenever the current selection changes, the client with
keyboard focus receives a `wl_data_device.selection` event. This event is also
generated on a client immediately before it receives keyboard focus.

The data offer is introduced with `wl_data_device.data_offer` event before the
selection event.

#### Drag and Drop

A drag-and-drop operation is started using the `wl_data_device.start_drag`
request. This requests causes a pointer grab that will generate enter, motion
and leave events on the data device. A data source is supplied as argument to
start_drag, and data offers associated with it are supplied to clients surfaces
under the pointer in the `wl_data_device.enter` event. The data offer is
introduced to the client prior to the enter event with the
`wl_data_device.data_offer` event.

Clients are expected to provide feedback to the data sending client by calling
the `wl_data_offer.accept` request with a mime type it accepts. If none of the
advertised mime types is supported by the receiving client, it should supply
NULL to the accept request. The accept request causes the sending client to
receive a `wl_data_source.target` event with the chosen mime type.

When the drag ends, the receiving client receives a `wl_data_device.drop` event
at which it is expected to transfer the data using the `wl_data_offer.receive`
request.
