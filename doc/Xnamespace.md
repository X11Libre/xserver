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
server startup, or provisioned dynamically at runtime via the `X-NAMESPACE`
protocol extension or a `cgroup` rule in the configuration file. The extension
is enabled when a namespace config is passed to the Xserver via the
`-namespace <fn>` command-line flag, or configured in `xorg.conf` /
`/etc/X11/xorg.conf.d/*.conf`:

```xorg.conf
Section "ServerFlags"
    Option "NamespaceConfig" "/etc/X11/xnamespace.conf"
EndSection
```

If the `NAMESPACE` extension is enabled via `Section "Extensions"` without an
explicit `NamespaceConfig` path, `/etc/X11/xnamespace.conf` is used by default.

See `Xext/namespace/ns.conf.example` for a configuration file example.
Per-namespace capability directives supported in the config file:
`allow mouse-motion`, `allow shape`, `allow transparency`, `allow xinput`,
`allow xkeyboard`, `allow clipboard`, `allow clipboard-auto`, and `superpower`.

Dynamic cgroup-based provisioning can be configured in `xnamespace.conf` using
`default` and `cgroup`:

```conf
default root

cgroup /xns
  allow shape
  allow xinput
  allow clipboard-auto
```

Authentication / Namespace assignment
-------------------------------------

Assignment of clients into namespaces is done in order of precedence:
1. the authentication token the client is using to authenticate (if it matches
   a configured or runtime-registered namespace token),
2. the local client's Linux cgroup v2 path (`/proc/<pid>/cgroup`) when a
   `cgroup /prefix` block is configured (matching `0::/prefix/[<uid>/]<name>`),
   which automatically creates a transient namespace `<name>` with the
   capabilities specified in the `cgroup` block and removes it when its last
   client disconnects,
3. verified local processes (`pid > 0`) outside the configured cgroup prefix
   are assigned to `root` if `default root` is set (or `anon` by default),
   while non-local (e.g. TCP) or unidentifiable clients always fall back to
   `anon`.

Clipboard Modes
---------------

Each namespace operates in one of three clipboard modes:
- **Isolated (`off`, default):** both `clipboard` and `clipboard-auto` are
  unset; selections are rewritten to `<ns>CLIPBOARD` / `<ns>PRIMARY`.
- **Shared (`on`, `allow clipboard`):** unrestricted access to global
  selections.
- **Focus-gated (`auto`, `allow clipboard-auto`):** a focused namespace may
  claim ownership of global selections (copy out), while reading a global
  selection owned by another namespace requires a genuine physical key or
  button press delivered while the namespace holds focus, and is revoked as
  soon as focus leaves the namespace or another namespace takes ownership of a
  global selection. Cross-namespace property writes during `ConvertSelection`
  are restricted to the active `(requestor, property)` pair while the sender
  still owns that selection.
