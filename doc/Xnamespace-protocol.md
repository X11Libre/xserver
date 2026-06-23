Xnamespace protocol extension — design v0.1 (DRAFT)
===================================================

This document specifies an X11 protocol extension for the Xnamespace
feature. Its purpose is to let an authorized management client
**dynamically manage namespaces at runtime** — the settings that today can
only be provisioned statically from the `namespace.conf` file at server
startup (see `doc/Xnamespace.md` and `Xext/namespace/config.c`).

The expected user is a container/session manager: it creates a namespace
when a container starts and deletes it when the container finishes. Traffic
is very low, so requests are kept simple and synchronous (round-tripped)
rather than optimised for throughput — many X11 packets are needlessly
complex and this extension deliberately avoids that.

The wire protocol mirrors the existing in-memory model (`struct Xnamespace`
in `Xext/namespace/namespace.h`) so the extension stays a thin, auditable
layer over data the server already maintains.

Status: DESIGN DRAFT. Opcodes, encodings and the capability bit layout are
not yet frozen.


1. Goals and non-goals
----------------------

Goals:

 * Enumerate namespaces and their settings.
 * Create and delete namespaces at runtime.
 * Query and modify per-namespace capability flags
   (`mouse-motion`, `shape`, `transparency`, `xinput`, `xkeyboard`,
   `superpower`).
 * Add, list and remove authentication tokens that map clients into a
   namespace (the `auth` lines in the config file today).
 * Auto-remove a namespace when its last client exits (transient
   namespaces for transient containers).

Non-goals (decided, not just deferred):

 * **No persistence.** Xservers never persist state; runtime changes live
   only in memory. The config file remains the boot-time seed and is never
   rewritten. There is no `SaveConfig`/`ReloadConfig`.
 * **No client reassignment.** A client's namespace is fixed at connect
   time by its auth token. There is no `AssignClient` request. A namespace
   is emptied by terminating its clients, not by moving them.
 * **No namespace hierarchy yet.** A name delimiter is reserved for future
   nesting (section 3), but inheritance semantics are undefined and using
   the delimiter now is an error.
 * **No events yet.** With a single synchronous manager there is no
   multi-manager consistency problem to solve. A `NamespaceNotify` event
   may be added later if a use case appears.


2. Security model (the load-bearing part)
-----------------------------------------

The entire point of namespaces is isolation, and a management protocol is a
direct path to subverting it. So the access rule is simple and absolute:

> **The whole extension is reachable only by clients in a `superPower`
> namespace** (today only the built-in `root`, plus any namespace
> explicitly granted `superpower`). To every other client the extension
> does not exist.

This is enforced exactly where the existing code already enforces the same
boundary for `SECURITY`, `RECORD` and `XTEST` — the two Xace extension
hooks, both of which already gate on `subj->ns->superPower`:

 * `hook-ext-access.c` (fires on `QueryExtension`): non-superPower clients
   get `BadAccess`, so the extension is **invisible** — it reports as not
   present. A sandboxed client cannot even discover it exists.
 * `hook-ext-dispatch.c` (fires on every request to the extension):
   non-superPower clients get `BadAccess`, so even a client that hard-codes
   the major opcode cannot invoke anything.

Adding `Xnamespace-mgmt` to the blacklist in both hooks is the entire
access-control implementation. Consequences:

 * There is **no per-request privilege table**. Every request handler may
   assume its caller is superPower. One choke point to audit, not twelve.
 * `QueryVersion` is reachable only by superPower clients too. That is
   fine: a client that cannot see the extension has nothing to negotiate,
   and "not present" is the honest answer for it.
 * No client can grant itself capabilities it lacks, because a
   non-superPower client cannot reach any request, mutating or not.

The built-in namespaces `root` and `anon` are **immutable** over the
protocol: their name, capabilities, attributes and token set are fixed at
boot. Any mutating request targeting them (delete, set-flags, add/remove
token) returns `BadAccess`. They are configured only via the config file.


3. Naming and identity
-----------------------

Namespaces are identified **by name** — the same unique key the code
already uses (`XnsFindByName()`, `select_ns()`). Names are the only
namespace identifier on the wire; there are no protocol-level numeric IDs.
The server is free to keep an internal id or resource for its own
bookkeeping, but that is an implementation detail and never appears in a
request, reply or error. Given the low, synchronous traffic, string lookup
costs nothing and a single identifier is simpler to reason about.

Name rules (violations return `BadName`):

 * Must be non-empty.
 * Must be unique (duplicate on `CreateNamespace` → `BadName`).
 * Must not contain the reserved nesting delimiter `'/'`. The delimiter is
   reserved now so that a future hierarchy (`parent/child`) can be added
   without a grammar break; using it today is rejected.
 * Should be restricted to printable, non-whitespace characters so that any
   protocol-created name remains expressible in `namespace.conf` (the
   config parser splits on whitespace and treats `#` as a comment).
   Implementations reject other characters with `BadName`.

`BadName` (core error 15) is reused as the single name-related error:
malformed, reserved-delimiter, duplicate-on-create, or no-such-namespace.

Auth tokens, which have no natural human name, are referenced within a
namespace by a small server-assigned opaque handle (`CARD32`) returned at
creation. This is a per-namespace bookkeeping handle, not a global resource.


4. Capabilities and attributes
-------------------------------

Two `CARD32` words describe a namespace.

**Capabilities** — access permissions (the `allow*` / `superPower` fields):

 | Bit     | Name                           |
 | ------- | ------------------------------ |
 | 1 << 0  | `XNS_CAPABILITY_MOUSE_MOTION`  |
 | 1 << 1  | `XNS_CAPABILITY_SHAPE`         |
 | 1 << 2  | `XNS_CAPABILITY_TRANSPARENCY`  |
 | 1 << 3  | `XNS_CAPABILITY_INPUT`         |
 | 1 << 4  | `XNS_CAPABILITY_KEYBOARD`      |
 | 1 << 5  | `XNS_CAPABILITY_ADMIN`         |

**Attributes** — lifecycle/status (new fields on `struct Xnamespace`):

 | Bit     | Name                  | meaning                                 |
 | ------- | --------------------- | --------------------------------------- |
 | 1 << 0  | `XNS_ATTR_IMMUTABLE`  | read-only; set by server for root/anon  |
 | 1 << 1  | `XNS_ATTR_TRANSIENT`  | destroy namespace when last client exits|

Unused bits in either word are reserved, must be 0, and yield `BadValue`.
New capabilities/attributes are added by allocating the next bit and
bumping the minor version.


5. Versioning
-------------

`QueryVersion` negotiates (major, minor). This document is major 1, minor 0.
The client sends the highest version it supports; the server replies with
the highest it supports that is not greater than the client's.


6. Requests
-----------

Every request begins with the standard 4-byte extension header (major
opcode, 1-byte minor opcode, 16-bit length in 4-byte units). All requests
are 4-byte aligned. A namespace name is encoded as a single length-prefixed
STRING8 (`CARD16` length + bytes + zero padding to a 4-byte boundary) — one
uniform encoding, easy to parse.

All mutating requests carry a reply so the manager round-trips and learns
success or failure synchronously, instead of relying on asynchronous error
delivery. Replies are deliberately minimal.

Minor opcodes:

 | Opcode | Request              | Reply |
 | ------ | -------------------- | ----- |
 |   0    | `QueryVersion`       | yes   |
 |   1    | `ListNamespaces`     | yes   |
 |   2    | `CreateNamespace`    | yes (ack) |
 |   3    | `DeleteNamespace`    | yes (ack) |
 |   4    | `QueryNamespace`     | yes   |
 |   5    | `SetNamespaceFlags`  | yes (ack) |
 |   6    | `AddAuthToken`       | yes   |
 |   7    | `RemoveAuthToken`    | yes (ack) |
 |   8    | `ListAuthTokens`     | yes   |
 |   9    | `GetClientNamespace` | yes   |

### 6.0 QueryVersion

```
  QueryVersion
    client-major-version: CARD16
    client-minor-version: CARD16
  =>
    major-version: CARD16
    minor-version: CARD16
```

### 6.1 ListNamespaces

```
  ListNamespaces
  =>
    namespaces: LISTofNAMESPACEINFO
```

where each NAMESPACEINFO is a fixed header followed by the name:

```
    capabilities: CARD32       (section 4)
    attributes:   CARD32       (section 4)
    refcnt:       CARD32        (clients currently assigned)
    num-tokens:   CARD32
    name:         STRING8       (length-prefixed, padded)
```

### 6.2 CreateNamespace

```
  CreateNamespace
    capabilities: CARD32       (initial caps)
    attributes:   CARD32       (XnsAttrAutoRemove honored; XnsAttrBuiltin
                                ignored/illegal)
    name:         STRING8
  =>
    (empty ack)
```

Errors: `BadName` (empty / illegal char / reserved `'/'` / duplicate),
`BadValue` (reserved capability or attribute bit set), `BadAlloc`.

### 6.3 DeleteNamespace

```
  DeleteNamespace
    onclients: CARD8           (0 = fail if any client present,
                                1 = terminate all clients, then delete)
    pad:       3 bytes
    name:      STRING8
  =>
    (empty ack)
```

 * Deleting a built-in namespace → `BadAccess`.
 * `onclients == 0` and the namespace still has clients → `BadAccess`
   (busy). The manager must terminate them first or pass `onclients == 1`.
 * `onclients == 1`: the server disconnects every client in the namespace
   (as if each had closed its connection), then frees the namespace and
   removes its auth tokens (calling `RemoveAuthorization()` for each).
 * Any other `onclients` value → `BadValue`.

### 6.4 QueryNamespace

```
  QueryNamespace
    name: STRING8
  =>
    capabilities: CARD32
    attributes:   CARD32
    refcnt:       CARD32
    num-tokens:   CARD32
```

`BadName` if no such namespace.

### 6.5 SetNamespaceFlags

Atomically updates the subset of capability bits selected by `value-mask`:

```
  SetNamespaceFlags
    value-mask: CARD32         (which capability bits to apply)
    values:     CARD32         (new values for the masked bits)
    name:       STRING8
  =>
    capabilities: CARD32       (resulting capabilities)
```

`new_caps = (old_caps & ~value-mask) | (values & value-mask)`. Targeting a
built-in namespace → `BadAccess`. A reserved bit in `value-mask` →
`BadValue`. `BadName` if no such namespace. (Attributes are set only at
creation in v0.1; there is no runtime attribute change.)

### 6.6 AddAuthToken

Runtime equivalent of an `auth` config line; internally calls
`AddAuthorization()` exactly as `config.c` does and records the token.

```
  AddAuthToken
    name:        STRING8        (namespace name)
    auth-proto:  STRING8        (e.g. "MIT-MAGIC-COOKIE-1")
    auth-data:   STRING8        (raw key bytes, length-prefixed — NOT hex)
  =>
    token-handle: CARD32        (per-namespace handle for removal)
```

The key is carried as raw bytes; the hex encoding in the config file
(`hex2bin()` in `config.c`) is a text-file affordance with no place in a
binary protocol. Targeting a built-in namespace → `BadAccess`. `BadName`
if no such namespace; `BadValue` if `auth-proto` is empty; `BadAlloc` on
failure.

(Three length-prefixed STRING8s in a fixed order — the request stays
trivially parseable despite three variable-length fields.)

### 6.7 RemoveAuthToken

```
  RemoveAuthToken
    token-handle: CARD32
    name:         STRING8
  =>
    (empty ack)
```

Calls `RemoveAuthorization()` and unlinks the token. Removing a token does
not disconnect clients already authenticated with it (standard X
authorization semantics); it only prevents future connections from using
it. `BadMatch` if the handle does not belong to the named namespace;
`BadAccess` for a built-in namespace; `BadName` if no such namespace.

### 6.8 ListAuthTokens

```
  ListAuthTokens
    name: STRING8
  =>
    tokens: LISTofAUTHTOKENINFO
```

where AUTHTOKENINFO is:

```
    token-handle: CARD32
    proto:        STRING8       (length-prefixed, padded)
```

Secret key material is **never** returned — only the protocol name and the
handle. A compromised manager cannot exfiltrate cookies it did not create.

### 6.9 GetClientNamespace

Read-only introspection: which namespace a given client belongs to. Useful
for auditing a namespace's members before a `DeleteNamespace onclients=1`.

```
  GetClientNamespace
    client-resource: CARD32     (a resource base; 0 = the calling client)
  =>
    is-server: BOOL
    pad:       3 bytes
    name:      STRING8          (the client's namespace name)
```

This is the protocol view of `XnsClientPriv(client)->ns`.


7. Errors
---------

The extension defines no new error codes; it reuses core errors:

 * `BadAccess` — operation on an immutable built-in namespace; delete of a
   busy namespace without `onclients=1`. (Also the result of a
   non-superPower client reaching the extension, though the Xace hooks make
   that unreachable in practice — defence in depth.)
 * `BadName`   — namespace name empty, illegal, reserved-delimiter,
   duplicate on create, or not found.
 * `BadValue`  — reserved capability/attribute bit set; bad `onclients`
   value; empty auth protocol.
 * `BadMatch`  — token handle does not belong to the named namespace.
 * `BadAlloc`  — out of memory.


8. Mapping to the existing implementation
-----------------------------------------

Registered like any other extension (`AddExtension`, cf.
`Xext/dpms/dpms.c:515`) from `NamespaceExtensionInit()` in
`Xext/namespace/namespace.c`, after `XnsLoadConfig()` succeeds — no point
exposing management when namespaces are disabled.

 | Protocol concept       | Existing code                                   |
 | ---------------------- | ----------------------------------------------- |
 | namespace record       | `struct Xnamespace` (`namespace.h`)             |
 | capability bits        | `allow*` / `superPower` bool fields             |
 | find by name           | `XnsFindByName()`                               |
 | create namespace       | `select_ns()` logic, factored out of `config.c` |
 | add token              | `AddAuthorization()` + `xorg_list_append`       |
 | remove token           | `RemoveAuthorization()` + `xorg_list_del`       |
 | client → namespace     | `XnsClientPriv()->ns`                           |
 | refcount               | `struct Xnamespace.refcnt`                      |

Required changes:

 1. **Access gate.** Add `Xnamespace-mgmt` to the blacklist in both
    `hook-ext-access.c` (invisible to non-superPower) and
    `hook-ext-dispatch.c` (`BadAccess` on dispatch), alongside
    `SECURITY`/`RECORD`/`XTEST`.
 2. **Shared setters.** Factor the per-field parsing in `parseLine()` into
    helpers (`XnsCreateNamespace`, `XnsSetCaps`, `XnsAddToken`,
    `XnsRemoveToken`) so the config loader and the protocol handlers share
    one code path and cannot drift apart.
 3. **Removable tokens.** Add an opaque per-namespace `token-handle` to
    `struct auth_token` and support unlinking + `RemoveAuthorization()`
    (today tokens are only ever appended at boot).
 4. **New attribute fields.** Add `Bool builtin` is already present; add
    `Bool autoRemove`. Set `builtin` for `root`/`anon`.
 5. **Auto-remove.** Centralise in `XnamespaceAssignClient()`: after the
    `priv->ns->refcnt--` path, if the namespace is non-builtin, has
    `autoRemove` set, and refcnt reached 0, destroy it (free tokens via
    `RemoveAuthorization()`, unlink from `ns_list`, free). This is reached
    naturally from `hookClientDestroy()`, which already calls
    `XnamespaceAssignClient(subj, NULL)`. `root`/`anon` are safe because
    they are builtin and start at `refcnt = 1`.
 6. **Client termination on delete.** For `DeleteNamespace onclients=1`,
    walk clients whose `XnsClientPriv()->ns` matches and close them
    (`CloseDownClient`), then free the namespace.
 7. **Dispatch.** A `ProcXnsDispatch` / `SProcXnsDispatch` pair with the
    usual byte-swapped variants for each request.

Serialization: everything runs in the single-threaded dispatch loop, so no
locking is needed. Refcount and list mutations are safe as they happen only
from request handlers and the existing callbacks.


9. Remaining minor questions
----------------------------

 1. **Reserved name charset.** `'/'` is reserved for nesting. Confirm the
    full allowed charset (proposal: printable ASCII excluding whitespace,
    `#`, and `/`) so protocol-created names round-trip through the config
    grammar.
 2. **Auto-remove vs. explicit delete race.** If a transient namespace
    auto-removes just before a manager issues `DeleteNamespace`, the delete
    sees no such name and returns `BadName`. Is that acceptable (treat as
    "already gone, success") or should auto-removed names linger briefly?
    Proposal: `BadName` is fine; the namespace is gone either way.
