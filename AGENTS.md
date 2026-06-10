# AGENTS.md — XLibre X Server

## Build system

- **Meson** (≥ 0.61.0), not autotools.
- Standard build:
  ```
  meson setup build -Dprefix=/usr -Dxorg=true -Dxvfb=true -Dxnest=true -Dxephyr=true -Dxfbdev=true
  ninja -C build
  meson test -C build --print-errorlogs
  ```
- CI uses `MESON_ARGS` env var; see `.github/workflows/build-xserver.yml` for typical flags.
- Builddir defaults to `__BUILD` in CI (env `MESON_BUILDDIR`).
- Run tests: `meson test -C build --print-errorlogs`.
- Run a single test: `meson test -C build --suite xvfb <test_name>`.

## Project structure

- **Core**: `dix/` (dispatch, events, resources), `mi/` (machine-independent), `fb/` (framebuffer), `os/` (sockets, I/O, auth).
- **Extensions**: `composite/`, `dbe/`, `dri3/`, `glx/`, `present/`, `randr/`, `record/`, `render/`, `xfixes/`, `Xi/`, `xkb/`, `Xext/` (includes SHM, Sync, Xinerama, and XLibre's `namespace/` for client isolation).
- **Acceleration**: `glamor/` (OpenGL 2D, modern), `exa/` (older 2D fallback).
- **DDX servers** (under `hw/`): `xfree86/` (Xorg), `vfb/` (Xvfb), `xnest/` (Xnest), `kdrive/` (includes Xephyr, Xfbdev), `xquartz/` (macOS), `xwin/` (Windows).
- **Entrypoint**: `dix/main.c` (dispatch loop).

## Coding conventions

- Language: C (gnu99), compiled with `-fno-strict-aliasing -fvisibility=hidden -fno-common`.
- Project-wide warnings (as errors): `-Werror=implicit`, `-Werror=return-type`, `-Werror=write-strings`, `-Wshadow`, `-Wvla`, `-Wincompatible-pointer-types`, etc.
- Code style matches existing patterns; no comment noise.
- New functions/types should have in-code documentation for doc generation.
- Always use braces on if/while/etc
- In macros with arguments, the argument calls shall be explicitly enclosed in parantheses
- never replace whole files if not needed, just change what necessary

## Key policies

- **Every commit needs a Signed-off-by** line (PR check enforced).
- systemd-logind and seatd-libseat are **mutually exclusive**.
- Module ABI tag: `xlibre-25` (set in `meson.build:784`).
- Nvidia legacy driver quirks: `-Dlegacy_nvidia_padding=true -Dlegacy_nvidia_340x=true`.

## Test quirks

- Tests require `build_xserver=true` and non-Windows.
- Xvfb-based tests: suite name `xvfb`. Xephyr-glamor: suite name `xephyr-glamor`.
- Unit tests: suite default, require `build_xorg=true`, link against `xorg_link`.
- Some Xephyr GLES tests may fail without DRI; `-Dtest_xephyr_gles=false` to skip.
- Rendercheck triangle tests may fail on Xephyr; `-Dtest_rendercheck_triangles=false` to skip.
- CI uses piglit + XTS for integration testing; expects `$PIGLIT_DIR` and `$XTEST_DIR`.

## Common meson options

See `meson_options.txt` for all 170+ options. Notable:
- Server variants: `-Dxorg=auto`, `-Dxvfb=true`, `-Dxnest=auto`, `-Dxephyr=false`, `-Dxfbdev=false`, `-Dxquartz=auto`, `-Dxwin=auto`.
- Extensions: `-Dglamor=auto`, `-Dglx=true`, `-Ddri3=auto`, `-Dnamespace=true`, `-Dxinerama=true`, `-Dxres=true`.
- Platform: `-Dudev=auto`, `-Dudev_kms=auto`, `-Dsystemd_logind=false`, `-Dseatd_libseat=auto`.
- Debugging: `-Dlibunwind=true`, `-Dwerror=true`, build with `-Db_sanitize=address`.
- SHA1 backend: `-Dsha1=auto` (probes libc, CommonCrypto, CryptoAPI, libmd, libsha1, libnettle, libgcrypt, libcrypto).

## Debugging tips

- `./X :1 vt8 -logfile /dev/stdout` to run a test X server.
- Use Xephyr for safe nested testing: `Xephyr :1 -glamor -screen 1280x1024`.
- For driver debugging, place `.so` in `$prefix/lib/xorg/modules/xlibre-25/input/`.
- Use SSH/serial for gdb — VT switching is unavailable during breakpoints.

## CI
- ignore `xorg` remote / gitlab.freedesktop.org
- github repo: https://github.com/X11Libre/xserver (mapped as `origin`)
- use local `gh` tool for github access
- incubator branches may have commits with PR ID's prefixed

## Known issues
- Auto-fixer cannot handle: `##`/`#` contexts, type declarations, string-literal concatenation, double-paren `ErrorF`/`FatalError`. Must revert manually after each run.
- `import('python3')` deprecation left untouched (Windows-only module).
- Two meson fixes need re-application if the branch is rebased again.

## Rebase pitfalls (learned the hard way)
- `GIT_SEQUENCE_EDITOR=true` produces an empty todo list — use `GIT_SEQUENCE_EDITOR=:` (no-op) to accept the autosquashed list as-is.
- A remote PR branch may have >1 commit (e.g. original + revert on top). Always inspect the full log before cherry-picking; `cherry-pick HEAD` only gets the tip.
- When rebuilding a branch from scratch, diff each changed file against the old branch to catch missing commits.

## AGENTS.md file
- should never go into any PR, changes always committed separately, use commit message prefix "WIP: AGENTS.md"
- from time to time, those commits should be squashed together into one (housekeeping)
