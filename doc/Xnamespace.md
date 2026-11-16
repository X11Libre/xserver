Xnamespace extension v1.0
=========================

This extension separates clients into several namespaces (a bit similar to
Linux's kernel namespaces), which are isolated from each other. For example,
namespaces have their own selections and clients cannot directly interact
(send messages) or access other client's resources across namespace borders.

An exception is the `root` namespace, which completely is unrestricted.

Configuration
-------------

Namespaces are defined in a separate configuration file, which is loaded at
server startup (no dynamic provisioning in this version yet). The extension
is enabled when a namespace config is passed to the Xserver via the
`-namespace <fn>` flag.


See `Xext/namespace/ns.conf.example` for a configuration file example.

All enforcement components are enabled by default. Use `disable` to turn off
individual components server-wide, for example:

```
disable clipboard
disable devices events
```

`enable` accepts the same component names and turns them back on. Directives
are processed in order, so the last setting for a component wins. They apply
to every namespace, including namespaces created at runtime, regardless of
which namespace stanza contains the directive. Settings are loaded at server
startup and reset to the defaults when the configuration is loaded again.
Unknown component names are logged and ignored.

| Component | Enforcement disabled by `disable` |
|-----------|-----------------------------------|
| `clipboard` | Selection name isolation (including PRIMARY, SECONDARY, CLIPBOARD, and XFixes selection notifications) |
| `properties` | Root-window property redirection and property access hooks |
| `clients` | Cross-namespace client access checks |
| `devices` | Input device access checks |
| `extensions` | Extension visibility and request filtering |
| `server` | Server configuration and server grab access checks |
| `resources` | Resource access checks, including window transparency restrictions |
| `events` | Event send and receive filtering |

Disabling a component relaxes isolation for that component. Other components
continue to enforce their restrictions: for example, disabling `clipboard`
shares selection names, but resource and event checks can still prevent
cross-namespace clipboard data transfers. Namespace assignment, client
lifecycle tracking, virtual root creation, and authorization of the namespace
management protocol always remain active. In particular, disabling
`extensions` does not expose namespace management to unprivileged clients.

Authentication / Namespace assignment
-------------------------------------

Assignment of clients into namespaces is done by the authentication token the
client is using to authenticate. Thus, token authentication needs to enabled.
