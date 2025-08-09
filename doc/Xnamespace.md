Xnamespace extension v1.0
=========================

This extension separates clients into several namespaces which are isolated from each other.
It is similar to Linux's kernel namespaces.

Namespaces have their own selections, and clients cannot directly interact
(send messages) or access another client's resources across namespace borders.
The only exceptions are clients in the root namespace.

# Configuration

Namespaces are defined in a separate configuration file, which is loaded at
server startup.
There is no dynamic provisioning in this version yet.
The extension is enabled when a namespace config is passed to the Xserver via the
`-namespace <fn>` flag.

See `Xext/namespace/ns.conf.example` for a configuration file example.

# Authentication / Namespace assignment

Assignment of clients into namespaces is done by the authentication token the
client is using to authenticate; so, token authentication needs to be enabled.

An authentification token can either be a **MIT-MAGIC-COOKIE-1** or a **XDM-AUTHORIZATION-1**.

For more information, use the command `man xauth`.

## MIT_MAGIC-COOKIE-1 Protocol
An authentification token for the **MIT_MAGIC-COOKIE-1** is a 16-byte UTF-8 hexadecimal string.
### How to generate a valid token

#### Working X server is available
If you have access to a working X server, use the command `xauth generate $DISPLAY MIT-MAGIC-COOKIE-1`.
Then use `xauth list` to view the generated token.

#### Working X server is unavailable
If you don't have access to X server, there are two additional methods.

##### Command Line
If you have access to the command line, there are multiple commands
    - `od -N32 -x < /dev/urandom | head -n1 |  cut -b9- | sed 's/ //gi'`
    - `openssl rand -hex 16`

##### Pseudocode with an implemented example
1. Declare and initialize an empty string
2. Generate a random number from 0 to 15
3. Convert the number to its hexadecimal equivalent character
4. Add character to the string
5. If the string's length is less than 32, go back to step 2
6. Store the string so you can use it for authentification

```c
int main(void)
{
    // Declare and initialize an empty string
    char hexStr[33] = {0}; // Must be 33 in C to account for null-terminator
    srand(time(NULL)); // seeding srand
    for (int i = 0; i < 32; i++)
    {
        const int value = (rand() % 16); // Generate a random number from 0 to 15
        char hexChar = (value < 10 ? '0' : 'a' - 10) + value; // Convert the number to its hexadecimal equivalent character
        hexStr[i] = hexChar; // Add character to the string
    } // If the string's length is less than 32, go back to step 2
    printf("%s\n", hexStr); // Store the string so you can use it for authentification
}
```

## XDM-AUTHORIZATION-1
An authentification token for the **XDM-AUTHORIZATION-1** is a 16-byte UTF-8 hexadecimal string where the 17th and 
18th character are 0.
### How to generate a valid token
#### Working X server is available
Create a random string of 32 hexadecimal characters and set the 17th and 18th characters to 0. 
 - aabbccddeeffaabb00aabbccddeeffaa
If you have access to a working X server,
 use the command `xauth add :0 XDM-AUTHORIZATION-1 aabbccddeeffaabb00aabbccddeeffaa` to add it to your xauth.

#### Working X server is unavailable
If you don't have access to X server, there are two additional methods.

##### Command Line
If you have access to the command line, there are multiple commands
- `od -N32 -x < /dev/urandom | head -n1 |  cut -b9- | sed 's/ //gi' | sed 's/^\(.\{16\}\)../\1 00/' | tr -d ' '`
- `openssl rand -hex 16 | sed 's/^\(.\{16\}\)../\1 00/' | tr -d ' '`

##### Pseudocode with an implemented example
1. Declare and initialize an empty string
2. Generate a random number from 0 to 15
3. Convert the number to its hexadecimal equivalent character
4. Add character to the string
5. If the string's length is less than 32, go back to step 2
6. Set the 17th and 18th characters to 0.
7. Store the string so you can use it for authentification

```c
int main(void)
{
    // Declare and initialize an empty string
    char hexStr[33] = {0}; // Must be 33 in C to account for null-terminator
    srand(time(NULL)); // seeding srand
    for (int i = 0; i < 32; i++)
    {
        const int value = (rand() % 16); // Generate a random number from 0 to 15
        char hexChar = (value < 10 ? '0' : 'a' - 10) + value; // Convert the number to its hexadecimal equivalent character
        hexStr[i] = hexChar; // Add character to the string
    } // If the string's length is less than 32, go back to step 2
    
    // Set the 17th and 18th characters to 0.
    hexStr[16] = '0'; // In C the first character is 0 not 1 and so on.
    hexStr[17] = '0';
    
    printf("%s\n", hexStr); // Store the string so you can use it for authentification
}
```

# How it works

**XNamespace (XN)** uses the **X Access Control Extension Specification (XACE)** to hook into the X server's functions. 
Whenever a client tries to access an X server resource, the client's namespace is checked for the correct privileges. 
If the client is in the correct namespace with the appropriate permissions, access to the resource is granted; 
otherwise, XN will deny access to that resource.

## XACE Callbacks Enums used by XN
- XACE_EXT_DISPATCH (XED)
- XACE_EXT_ACCESS (XEA)
- XACE_RECEIVE_ACCESS (XRecA)
- XACE_RESOURCE_ACCESS (XResA)


## Consequences of Unallowed access

| Property          | XED                                            | XEA                        | XRecA                                           | XResA                     |
|-------------------|------------------------------------------------|----------------------------|-------------------------------------------------|---------------------------|
| allowMouseMotion  | N/A                                            | N/A                        | Status is set to BadAccess and client is logged | N/A                       |
| allowShape        | Status is not changed and the client is logged | Status is set to BadAccess | N/A                                             | N/A                       |
| allowTransparency | N/A                                            | N/A                        | N/A                                             | Background will be opaque |
| allowXInput       | Status is not changed and the client is logged | Status is set to BadAccess | N/A                                             | N/A                       |
| allowXKeyboard    | Status is not changed and the client is logged | N/A                        | N/A                                             | N/A                       |

## Examples
### Permissions given by example file
Use the example conf file below, the table represent what each namespace is allowed to do.

```
# When no container are mentioned, all auths and allows configured affect the "root" namespace
    auth MIT-MAGIC-COOKIE-1 46f8e62b78e58962de0ceefc05ad90b0
    
container xeyes
  auth MIT-MAGIC-COOKIE-1 46f8e62b78e58962de0ceefc05ad90b8
  
  allow mouse-motion
  allow shape
  allow xinput

container xclock
  auth MIT-MAGIC-COOKIE-1 46f8e62b78e58962de0ceefc05ad90b7
```

| Container | mouse-motion | shape | xinput |
|-----------|--------------|-------|--------|
| root      | ✔️           | ✔️    | ✔️     |
| xeyes     | ✔️           | ✔️    | ✔️     |
| xclock    |              |       |        |

### Example of Clients trying to access xinput and each other from different namespaces

| Client  | Namespace | Access to xinput    | Communication with others          |
|---------|-----------|---------------------|------------------------------------|
| Client1 | root      | Succeeds (implicit) | Can communicate with App2 and App3 |
| Client2 | xeyes     | Succeeds (explicit) | Cannot access App1 or App3         |
| Client3 | xclock    | Fails               | Cannot access App1 or App2         |