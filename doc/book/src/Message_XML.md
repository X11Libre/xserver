# Message Definition Language

## Overview

The fundamentals of the Wayland protocol are explained in [Wayland Protocol and
Model of Operation](Protocol.md). This chapter formally defines the language
used to define Wayland protocols.

Wayland is an object-oriented protocol. Each object follows exactly one
interface. An interface is a collection of message and enumeration definitions.
A message can be either a request (sent by a client) or an event (sent by a
server). A message can have arguments. All arguments are typed.

## XML Elements

### protocol

```
protocol ::= (copyright?, description? interface+)
```

`protocol` is the root element in a Wayland protocol XML file. Code generation
tools may optionally use the protocol `name` in API symbol names. The XML file
name should be similar to the protocol name.

The description element should be used to document the intended purpose of the
protocol, give an overview, and give any development stage notices if
applicable.

The copyright element should be used to indicate the copyrights and the license
of the XML file.

**Required attributes**

`name`="`cname`"
  : The name of the protocol (a.k.a protocol extension). The name must start
    with one of the ASCII characters a-z, A-Z, or underscore, and the following
    characters may additionally include numbers 0-9.

    The name should be globally unique. Protocols to be included in
    [wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols)
    must follow the naming rules set there. Other protocols should use a unique
    prefix for the name, e.g. referring to the owning project's name.

### copyright

Parent elements: protocol

```
copyright ::= #PCDATA
```

Contains free-form, pre-formatted text for copyright and license notices.

### description

Parent elements: protocol, interface, request, event, arg, enum, entry

```
description ::= #PCDATA
```

Contains human-readable documentation for its parent element. May contain
formatted text, including paragraphs and bulleted lists.

**Optional attributes**

`summary`="`summary`"
  : A short (half a line at most) description of the documented element.

    When a description element is used, it is recommended to not use the
    `summary` attribute of the parent element.

### interface

Parent elements: protocol

```
interface ::= (description?, (request|event|enum)+)
```

An interface element contains the requests and events that form the interface.
Enumerations can also be defined with enum elements. These all belong into the
namespace of the interface. Code generation tools may use the interface `name`
in API symbol names.

Interfaces form an ancestry tree. Aside from
[wl_display](https://wayland.app/protocols/wayland#wl_display), new protocol
objects are always created through an existing protocol object that may be
referred to as _the factory object_. This can happen in one of two ways: the
factory object's interface either defines or does not define the new object's
interface.

When the factory interface defines the new object's interface, the new object
also inherits the factory object's interface version number. This number defines
the interface version of the new object. The factory object is referred to as
_the parent object_ and the factory interface is referred to as _the parent
interface_. This forms the ancestry tree of interfaces.

When the factory interface does not define the new object's interface, both the
interface name and the version must be communicated explicitly. The foremost
example of this is
[wl_registry.bind](https://wayland.app/protocols/wayland#wl_registry:request:bind).
In this case the terms "parent" or "ancestor" are not used. Interfaces that are
advertised through
[wl_registry](https://wayland.app/protocols/wayland#wl_registry) are called
_global interfaces_, or globals for short.

If objects having the interface can cause protocol errors, the protocol error
codes must be defined within the interface with an enum element with its `name`
set to `"error"`. Protocol error codes are always specific to the interface of
the object referred to in
[wl_display.error](https://wayland.app/protocols/wayland#wl_display:event:error).

The description element should be used to describe the purpose and the general
usage of the interface.

**Required attributes**

`name`="`cname`"
  : The name of the interface. The name must start with one of the ASCII
    characters a-z, A-Z, or underscore, and the following characters may
    additionally include numbers 0-9. The name must be unique in the
    protocol, and preferably it should also be globally unique to avoid API
    conflicts in language bindings of multiple protocols.

    Protocols to be included in
    [wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols)
    must follow the interface naming rules set there. Other protocols should use
    a unique prefix for the name, e.g. referring to the owning project's name.

`version`="`V`"
  : The interface's latest version number `V` must be an integer greater than
    zero. An interface element defines all versions of the interface from 1 to
    `V` inclusive. The contents of each interface version are defined in each of
    the request, event, enum and entry elements using the attributes `since` and
    `deprecated-since`, and in the specification text.

    When an interface is extended, the version number must be incremented on all
    the interfaces part of the same interface ancestry tree. The exception to
    this rule are interfaces which are forever stuck to version 1, which is
    usually caused by having multiple parent interfaces with independent
    ancestor global interfaces. In this case, the `frozen="true"` attribute
    described below should be used.

    A protocol object may have any defined version of the interface. The version
    of the object is determined at runtime either by inheritance from another
    protocol object or explicitly.

    It is possible for a protocol object to have a version higher than defined
    by its interface. This may happen when the interface is stuck at version 1
    as per above. It may also happen when a protocol XML file has not been
    thoroughly updated as required. In such cases the object shall function as
    with the highest defined interface version.

**Optional attributes**

`frozen`="`true`"
  : The interface is frozen and forever stuck at version 1.

    This attribute should be applied to interfaces that have multiple parent
    interfaces with independent ancestor global interfaces, for example
    `wl_buffer` and `wl_callback`.

### request

Parent elements: interface

```
request ::= (description?, arg*)
```

Defines a request, a message from a client to a server. Requests are always
associated with a specific protocol object.

Requests are automatically assigned opcodes in the order they appear inside the
interface element. Therefore the only backwards-compatible way to add requests
to an interface is to add them to the end. Any event elements do not interfere
with request opcode assignments.

The arg elements declare the request's arguments. There can be 0 to 20 arguments
for a request. The order of arg inside the request element defines the order of
the arguments on the wire. All declared arguments are mandatory, and extra
arguments are not allowed on the wire.

The description element should be used to document the request.

**Required attributes**

`name`="`cname`"
  : The name of the request. The name must start with one of the ASCII
    characters a-z, A-Z, or underscore, and the following characters may
    additionally include numbers 0-9. The name must be unique within
    all requests and events in the containing interface.

    Code and language binding generators may use the name in the API they
    create. The `name` of the containing interface provides the namespace for
    requests.

**Optional attributes**

`type`="`destructor`"
  : When this attribute is present, the request is a destructor: it shall
    destroy the protocol object it is sent on. Protocol IPC libraries may use
    this for bookkeeping protocol object lifetimes.

    Libwayland-client uses this information to ignore incoming events for
    destroyed protocol objects. Such events may occur due to a natural race
    condition between the client destroying a protocol object and the server
    sending events before processing the destroy request.

`since`="`S`"
  : `S` must be an integer greater than zero. If `since` is not specified,
    `since="1"` is assumed.

    This request was added in interface `version` `S`. The request does not
    exist if the protocol object has a bound version smaller than `S`. Attempts
    to use it in such a case shall raise the protocol error
    `wl_display.error.invalid_method`.

`deprecated-since`="`D`"
  : `D` must be an integer greater than the value of `since`. If
    `deprecated-since` is not specified, then the request is not deprecated in
    any version of the containing interface.

    This request was deprecated in interface `version` `D` and above, and should
    not be sent on protocol objects of such version. This is informational.
    Compositors must still be prepared to handle the request unless specified
    otherwise.

### event

Parent elements: interface

```
event ::= (description?, arg*)
```

Defines an event, a message from a server to a client. Events are always
associated with a specific protocol object.

Events are automatically assigned opcodes in the order they appear inside the
interface element. Therefore the only backwards-compatible way to add events to
an interface is to add them to the end. Any request elements do not interfere
with event opcode assignments.

The arg elements declare the event's arguments. There can be 0 to 20 arguments
for an event. The order of arg inside the event element defines the order of the
arguments on the wire. All declared arguments are mandatory, and extra arguments
are not allowed on the wire.

The description element should be used to document the event.

**Required attributes**

`name`="`cname`"
  : The name of the event. The name must start with one of the ASCII characters
    a-z, A-Z, or underscore, and the following characters may additionally
    include numbers 0-9. The name must be unique within all requests and events
    in the containing interface.

    Code and language binding generators may use the name in the API they
    create. The `name` of the containing interface provides the namespace for
    events.

**Optional attributes**

`type`="`destructor`"
  : When this attribute is present, the event is a destructor: it shall destroy
    the protocol object it is sent on. Protocol IPC libraries may use this for
    bookkeeping protocol object lifetimes.

    > [!WARNING]
    > Destructor events are an underdeveloped feature in Wayland. They can be
    > used only on client-created protocol objects, and it is the protocol
    > designer's responsibility to design such a message exchange that race
    > conditions cannot occur. The main problem would be a client sending a
    > request at the same time as the server is sending a destructor event. The
    > server will consider the protocol object to be already invalid or even
    > recycled when it proceeds to process the request. This often results in
    > protocol errors, but under specific conditions it might also result in
    > silently incorrect behavior.
    >
    > Destructor events should not be used in new protocols. If a destructor
    > event is necessary, the simplest way to avoid these problems is to have
    > the interface not contain any requests.

`since`="`S`"
  : `S` must be an integer greater than zero. If `since` is not specified,
    `since="1"` is assumed.

    This event was added in interface `version` `S`. The event does not exist if
    the protocol object has a bound version smaller than `S`.

`deprecated-since`="`D`"
  : `D` must be an integer greater than the value of `since`. If
    `deprecated-since` is not specified, then the event is not deprecated in any
    version of the containing interface.

    This event was deprecated in interface `version` `D` and above, and should
    not be sent on protocol objects of such version. This is informational.
    Clients must still be prepared to receive this event unless otherwise
    specified.

### arg

Parent elements: request, event

```
arg ::= description?
```

This element declares one argument for the request or the event.

**Required attributes**

`name`="`cname`"
  : The name of the argument. The name must start with one of the ASCII
    characters a-z, A-Z, or underscore, and the following characters may
    additionally include numbers 0-9. The name must be unique within
    all the arguments of the parent element.

`type`="`T`"
  : The type `T` of the argument datum must be one of:

    `int`
      : 32-bit signed integer.

    `uint`
      : 32-bit unsigned integer.

    `fixed`
      : Signed 24.8-bit fixed-point value.

    `string`
      : UTF-8 encoded string value, NUL byte terminated. Interior NUL bytes are
        not allowed.

    `array`
      : A byte array of arbitrary data.

    `fd`
      : A file descriptor.

        The file descriptor must be open and valid on send. It is not possible
        to pass a null value.

    `new_id`
      : Creates a new protocol object. A request or an event may have at most
        one `new_id` argument.

        If `interface` is specified, the new protocol object shall have the
        specified interface, and the new object's (interface) version shall be
        the version of the object on which the request or event is being sent.

        If `interface` is not specified, the request shall implicitly have two
        additional arguments: A `string` for an interface name, and a `uint` for
        the new object's version. Leaving the interface unspecified is reserved
        for special use,
        [wl_registry.bind](https://wayland.app/protocols/wayland#wl_registry:request:bind)
        for example.

        > [!NOTE]
        > An event argument must always specify the `new_id` `interface`.

    `object`
      : Reference to an existing protocol object.

        The attribute `interface` should be specified. Otherwise IPC libraries
        cannot enforce the interface, and checking the interface falls on user
        code and specification text.

**Optional attributes**

`summary`="`summary`"
  : A short (half a line at most) description. This attribute should not be used
    if a description is used.

`interface`="`iface`"
  : If given, `iface` must be the `name` of some interface, and `type` of this
    argument must be either `"object"` or `"new_id"`. This indicates that the
    existing or new object must have the interface `iface`. Use for other
    argument types is forbidden.

    > [!NOTE]
    > If an interface from another protocol is used, then this creates a
    > dependency between the protocols. If an application generates code for one
    > protocol, then it must also generate code for all dependencies. Therefore
    > this would not be a backwards compatible change.

`allow-null`="`true`" | "`false`"
  : Whether the argument value can be null on send. Defaults to `"false"`,
    meaning it is illegal to send a null value. Can be used only when `type` is
    `"string"` or `"object"`.

    > [!NOTE]
    > Even though this attribute can be used to forbid a compositor from sending
    > a null object as an event argument, an IPC library implementation may not
    > protect the client from receiving a null object. This can happen with
    > libwayland-client when the client has destroyed the protocol object before
    > dispatching an event that referred to it in an argument.

`enum`="`enum-cname-suffix`"
  : If specified, indicates that the argument value should come from the enum
    named `enum-cname-suffix`. If the enumeration is a bitfield, then `type`
    must be `"uint"`. Otherwise `type` must be either `"uint"` or `"int"`.

    The name `enum-cname-suffix` refers to an enum in the same interface by
    default. If it is necessary to refer to an enumeration from another
    interface, the interface name can be given with a period:

    ```
    `enum`="`iface`.`enum-cname-suffix`"
    ```

    > [!NOTE]
    > This attribute alone does not automatically restrict the legal values for
    > this argument. If values from outside of the enumeration need to be
    > forbidden, that must be specified explicitly in the documentation.
    >
    > A common design pattern is to have the server advertise the supported
    > enumeration or bit values with events and explicitly forbid clients from
    > using any other values in requests. This also requires a protocol error
    > code to be specified with the error enum to be raised if a client uses an
    > illegal value, see [interface](#interface).

### enum

Parent elements: protocol

```
enum ::= (description?, entry*)
```

This tag defines an enumeration of integer values. Enumerations are merely a
syntactic construct to give names to arbitrary integer constants. Each constant
is listed as an entry with its name. There are two types of enumerations:
regular enumerations and bitfields.

Regular enumerations do not use `bitfield` attribute, or they set it to
`"false"`. The set of pre-defined values that belong to a regular enumeration is
exactly the set of values listed as entry elements after the protocol object
version is taken into account. See the entry attributes `since` and
`deprecated-since`.

Bitfields set `bitfield` to `"true"`. The set of values that belong to a
bitfield enumeration are all the values that can be formed by the bitwise-or
operator from the set of values listed as entry elements like in the regular
enumeration. Usually also zero is implicitly included.

All the values in a regular enumeration must be either signed or unsigned 32-bit
integers. All the values in a bitfield enumeration must be unsigned 32-bit
integers.

**Required attributes**

`name`="`cname-suffix`"
  : The name of the enumeration. The name must contain only the ASCII characters
    a-z, A-Z, 0-9, or underscore. The name cannot be empty. The name must be
    unique within all enumerations in the containing interface. The name is used
    as the namespace for all the contained entry elements.

**Optional attributes**

`since`="`S`"
  : `S` must be an integer greater than zero. If `since` is not specified,
    `since="1"` is assumed.

    This enumeration was added in interface `version` `S`. The enumeration does
    not exist if the protocol object has a bound version smaller than `S`.

`bitfield`="`true`" | "`false`"
  : Specifies if this enumeration is a bitfield. Defaults to `"false"`.

### entry

Parent elements: enum

```
entry ::= description?
```

Defines a name for an integer constant and makes it part of the set of values of
the containing enumeration.

**Required attributes**

`name`="`cname-suffix`"
  : The name of a value in an enumeration. The name must contain only the ASCII
    characters a-z, A-Z, 0-9, or underscore. The name cannot be empty. The name
    must be unique within all entry elements in the containing enum.

`value`="`V`"
  : An integer value. The value can be given in decimal, hexadecimal, or octal
    representation.

**Optional attributes**

`summary`="`summary`"
  : A short (half a line at most) description. This attribute should not be used
    if a description is used.

`since`="`S`"
  : `S` must be an integer greater than zero. If `since` is not specified,
    `since="1"` is assumed.

    This value was added in interface `version` `S`.

`deprecated-since`="`D`"
  : `D` must be an integer greater than the value of `since`. If
    `deprecated-since` is not specified, then the value is not deprecated in any
    version of the containing interface.

    This value was removed in interface `version` `D`. This does not make the
    value automatically illegal to use, see [arg](#arg) attribute `enum`.
