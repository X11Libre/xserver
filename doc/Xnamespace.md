Xnamespace extension v1.0
=========================

This extension separates clients into several namespaces (a bit similar to
Linux's kernel namespaces), which are isolated from each other. For example,
namespaces have their own selections and clients cannot directly interact
(send messages) or access other client's resources across namespace borders.

An exception is the `root` namespace, which completely is unrestricted.

Enabling
-------------

Namespaces are defined in a separate configuration file, which is loaded at
server startup (no dynamic provisioning in this version yet). The extension
is enabled when a namespace config is passed to the Xserver via the
`-namespace <fn>` flag.


See `Xext/namespace/ns.conf.example` for a configuration file example.

Authentication / Namespace assignment
-------------------------------------

Assignment of clients into namespaces is done by the authentication token the
client is using to authenticate. Thus, token authentication needs to enabled.


Configuration Specifics
-----------------------

The top of the file is the `root` namespace, the only thing that can be added
is auth tokens.

Namespaces are created in the config file using the `namespace` keyword
```
namespace <name>
```
NOTE: The `<name>` has nothing to do with client assignment.\
Until the next namespace keyword, following lines will only apply to the
namespace\
\
The `superpower` keyword enables complete access to all resources, but is one
directional. A namespace with superpowers is still protected from other
namespaces.
```
superpower
```
Auth tokens (aka cookies) are added with the auth keyword
```
auth <protocol> <hex-key>
```
(optionally) to generate a key at runtime
```
auth generate
```
Resource permissions are set by the `allow` keyword. current options include:
- mouse-motion\
mouse location data
- shape\
xorg SHAPE extension access
- transparency\
ability to render transparency in windows
- xinput\
access to XINPUT resources
- xkeyboard\
access to XKEYBOARD resources
- globalxkeyboard\
global keyboard access (raw input requests)
- render\
ability to render using (RENDER, GLX, DRI2, DRI3)
- randr\
xorg RANDR extension access
- screen\
complete screen access via MIT-SHM
- composite\
global access to COMPOSITE elements
```
allow <permission>
or chained
allow <permission> <permission> ...
```
Clients can be designated to a namespace using the `client` keyword.
This is an all-or-nothing assignment by using a programs basename.
```
client <program>
or chained
client <program> <program> ...
```
The `default` keyword will designate the global default namespace, or
optionally create a new namespace per client connection OR deny new
unauthorized connections.
```
default <namespace>
or
default <new_ns OR deny>
```

Technical Information
---------------------
All client connections to the Xorg server are assigned to a
`XnamespaceClientPriv` containing the `authID` and namespace reference.
\
A `namespace` is an `xorg_list` entry into the linked list `ns_list`
that contains at least `name`, `rootWindow`, `refcnt`, `perms`,
and `auth_token`.
\
When loading the Xnamespace extension, 
By default, the `ns_root` and `ns_anon` namespaces are made. The server
window itself is attached to `ns_root`. 
From there, the config file is parsed and new namespaces are added. 
A new `rootWindow` is created and linked to each new namespace. This is
where most of the "sandboxing" happens.
Upon client connection, a ClientPriv is allocated for the connection,
and the `clientstate` hooks begin to assign the ClientPriv to a namespace.
First the pre-assign client list is checked, then the `XAUTHORITY` env
variable is checked.

Events send and receive, Resource access, all is handled by the various hooks
which check the different permissions.
Example: If a namespace is made with `allow transparency` it will be allowed to
render a transparent segment in a window. 
