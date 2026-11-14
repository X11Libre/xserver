# Driver ABI check tools

Scripts that let any Xorg module check itself against staged XLibre
servers. They are installed with the server SDK; find them with:

    pkg-config --variable=abi_tools_dir xorg-server

Each server is built with debug info and staged under `ABI_WORK` with
`DESTDIR`, so nothing is installed into the system.

| Script | What it checks |
| --- | --- |
| `run-matrix.sh` | Builds the module against every server in `ABI_SERVERS` and checks that every server symbol it needs is exported by each of them (built for one release, loaded into another). |
| `gate-driver.sh` | Builds the committed tree (`BEFORE_REF`, default `HEAD`) and the working tree against each server, and fails if libabigail reports a changed layout of a server type the module uses. |
| `check-symbols.sh` | One module against one staged server. |
| `abigail-compare.sh` | One libabigail comparison; added types pass, removed symbols and changed layouts fail; `--allow TYPE` accepts a known change. |
| `check-private-includes.py` | For meson modules: every server-internal header a source file includes must be needed by it. |

`build-server.sh`, `build-driver.sh`, `staged-pc.sh` and `common.sh` are
the helpers the scripts above use.

## Environment

- `ABI_WORK`: work directory (required); put it on disk, the servers
  take a few hundred MB each.
- `ABI_MODULE`: the module inside a build directory, for
  `run-matrix.sh` and `gate-driver.sh`: `src/<name>_drv.so` for meson,
  `src/.libs/<name>_drv.so` for autotools.
- `ABI_DRIVER_SRC`: the module's source tree (default: the git top level
  of the current directory).
- `ABI_SERVERS`: server branches or tags (default: `release/25.0
  release/25.1 release/25.2 master`).
- `XSERVER_SRC`: a local xserver clone to take sources from instead of
  cloning from GitHub; `XSERVER_REMOTE` names its upstream remote.

Example, from a meson driver's source tree:

    tools=$(pkg-config --variable=abi_tools_dir xorg-server)
    ABI_WORK=$HOME/abi-work ABI_MODULE=src/amdgpu_drv.so "$tools/run-matrix.sh"
    ABI_WORK=$HOME/abi-work ABI_MODULE=src/amdgpu_drv.so "$tools/gate-driver.sh"

Needs meson, ninja, git, pkg-config, binutils (`nm`) and libabigail
(`abidiff`); autotools modules also need autoconf, automake and libtool.
