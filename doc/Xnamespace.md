# Xnamespace extension v1.0

This extension separates clients into several namespaces which are isolated from each other.
It is similar to Linux's kernel namespaces.

Namespaces have their own selections, and clients cannot directly interact
(send messages) or access another client's resources across namespace borders.
The only exceptions are clients in the root namespace.

## Configuration

Namespaces are defined in a separate configuration file, which is loaded at
server startup.
There is no dynamic provisioning in this version yet.
The extension is enabled when a namespace config is passed to the Xserver via the
`-namespace <fn>` flag.
If the **namespace** command hasn't appeared in the configuration, the current namespace is **root** for a non-namespace command. 
Else, the current **namespace** for a non-namespace command is the nearest namespace declared above it.
As a consequence, the **superpower**and **allow** commands have no effect as the root has access to every resource.
So, only the **auth** command matters before the first **namespace** is declared.

### Commands

A configuration file accepts four types of commands.

| Command               | Description                                                                                                                                            |
|-----------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| namespace             | Creates a new namespace and changes the current namespace to the new namespace.                                                                        |
| allow                 | Enables permission to access resources for the current namespace.                                                                                      |
| auth                  | Adds a new token to a namespace so it can be used to access the current namespace. [Permissions mentioned below.](#consequences-of-unallowed-access)   |
| superpower            | Current namespace is given all the powers of the root namespace.                                                                                       |
| container(deprecated) | The original command for declaring a new namespace. It is an alias for the **namespace** command. This may be removed as soon as 26.0.0. Anything that applies to **namespace** command applies to the container **command** as well. |



See `Xext/namespace/ns.conf.example` for a configuration file example.

## Authentication / Namespace assignment

Assignment of clients into namespaces is done by the authentication token the
client is using to authenticate.

An authentification token can either be a **MIT-MAGIC-COOKIE-1** or a **XDM-AUTHORIZATION-1**.
When an XLibre client is created, it can be provided an authentification protocol and token.
These two pieces of information are used to find the correct namespace and permission for the client.

If a token is generated outside xauth, ensure the tokens are unique.
For more information, use the command `man xauth`.

### MIT_MAGIC-COOKIE-1 Protocol

An authentification token for the **MIT_MAGIC-COOKIE-1** is a 16-byte UTF-8 hexadecimal string.
To generate a valid token, you have to options depending whether the X server is already running.

If you have access to a working X server, use the command `xauth generate $DISPLAY MIT-MAGIC-COOKIE-1`.
Then, use `xauth list` to view the generated token.

Else, if you don't have access to X server, use any of the commands below to generate a valid token.

- `od -N32 -x < /dev/urandom | head -n1 |  cut -b9- | sed 's/ //gi'`
- `openssl rand -hex 16`
- `uuidgen | tr -d '-'`
- `xxd -u -l 16 -p /dev/urandom`

For more information on implementing the protocol, please see the [Appendix](#mit-magic-cookie-1-pseudocode).

### XDM-AUTHORIZATION-1

An authentification token for the **XDM-AUTHORIZATION-1** is a 16-byte UTF-8 hexadecimal string where the 17th and
18th character are 0.

`xauth` provides no means to generate a token for this protocol; so, it has to be done by hand.

Any of the commands shown in [**MIT-MAGIC-COOKIE-1**](#mit_magic-cookie-1-protocol) followed by
`| sed 's/^\(.\{16\}\)../\1 00/' | tr -d ' '` will
generate a valid **XDM-AUTHORIZATION-1** token

For more information on implementing the protocol, please see the [Appendix](#xdm-authorization-1-pseudocode).

## How it works

**XNamespace (XN)** uses the **X Access Control Extension Specification (XACE)** to hook into the X server's functions.
Whenever a client tries to access an X server resource, the client's namespace is checked for the correct privileges.
If the client is in the correct namespace with the appropriate permissions, access to the resource is granted;
otherwise, XN will deny access to that resource.

### XACE Callbacks Enums used by XN

| Name                                                                                                                       | Abbreviation | Description                                                                                                                                                                     |
|----------------------------------------------------------------------------------------------------------------------------|--------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [XACE_EXT_DISPATCH](https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/Xserver/XACE-Spec.html#ext_dispatch_hook)       | XED          | This hook allows security extensions to examine all incoming extension protocol requests before they are dispatched.                                                            |
| [XACE_EXT_ACCESS](https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/Xserver/XACE-Spec.html#ext_access_hook)           | XEA          | This hook allows security extensions to approve or deny requests involving which extensions are supported by the server. This allows control over which extensions are visible. |
| [XACE_RECEIVE_ACCESS](https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/Xserver/XACE-Spec.html#receive_access_hook)   | XRecA        | This hook allows security extensions to prevent a client from receiving X events that have been delivered to a given window.                                                    |
| [XACE_RESOURCE_ACCESS](https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/Xserver/XACE-Spec.html#resource_access_hook) | XResA        | This hook allows security extensions to monitor all resource lookups.                                                                                                           |

### Consequences of Unallowed access

| Permissions  | XED                                            | XEA                        | XRecA                                           | XResA                     |
|--------------|------------------------------------------------|----------------------------|-------------------------------------------------|---------------------------|
| mouse-motion | N/A                                            | N/A                        | Status is set to BadAccess and client is logged | N/A                       |
| shape        | Status is not changed and the client is logged | Status is set to BadAccess | N/A                                             | N/A                       |
| transparency | N/A                                            | N/A                        | N/A                                             | Background will be opaque |
| xinput       | Status is not changed and the client is logged | Status is set to BadAccess | N/A                                             | N/A                       |
| xkeyboard    | Status is not changed and the client is logged | N/A                        | N/A                                             | N/A                       |

### Examples

#### Permissions given by example file

Presented below is an example Xnamespace configuration file.

```
## When no namespace are mentioned, all auths and allows configured affect the "root" namespace
    auth MIT-MAGIC-COOKIE-1 46f8e62b78e58962de0ceefc05ad90b0
    
namespace seyex
  auth MIT-MAGIC-COOKIE-1 46f8e62b78e58962de0ceefc05ad90b8
  
  allow mouse-motion
  allow shape
  allow xinput

namespace kcolcx
  auth MIT-MAGIC-COOKIE-1 46f8e62b78e58962de0ceefc05ad90b7
  allow transparency
  allow xkeyboard
 
 namespace foobar
    superpower
```

Using the example conf file above, the table below represents what each namespace is allowed to do.

| namespace | mouse-motion | shape | xinput | xkeyboard | transparency |
|-----------|--------------|-------|--------|-----------|--------------|
| root      | ✔️           | ✔️    | ✔️     | ✔️        | ✔️           |
| seyex     | ✔️           | ✔️    | ✔️     |           |              |
| kcolcx    |              |       |        | ✔️        | ✔️           |
| foobar    | ✔️           | ✔️    | ✔️     | ✔️        | ✔️           |

#### Example of Clients trying to access xinput and each other from different namespaces

| Client  | Namespace | Access to xinput    | Access transparency | Communication with others                 |
|---------|-----------|---------------------|---------------------|-------------------------------------------|
| Client1 | root      | Succeeds (implicit) | Succeeds (implicit) | Can communicate with App2, App3, and App4 |
| Client2 | seyex     | Succeeds (explicit) | Fails               | Cannot access App1 or App3                |
| Client3 | kcolcx    | Fails               | Succeeds (explicit) | Cannot access App1 or App2                |
| Client4 | foobar    | Succeeds (implicit) | Succeeds (implicit) | Can communicate with App2, App3, and App1 |

## Appendix

### MIT-MAGIC-COOKIE-1 Pseudocode

1. Declare and initialize an empty string
2. Generate a random number from 0 to 15
3. Convert the number to its hexadecimal equivalent character
4. Add character to the string
5. If the string's length is less than 32, go back to step 2
6. Store the string so you can use it for authentification

### XDM-AUTHORIZATION-1 Pseudocode

1. Declare and initialize an empty string
2. Generate a random number from 0 to 15
3. Convert the number to its hexadecimal equivalent character
4. Add character to the string
5. If the string's length is less than 32, go back to step 2
6. Set the 17th and 18th characters to 0.
7. Store the string so you can use it for authentification
