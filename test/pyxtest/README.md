# pyxtest - pytest-based X server test suite

This is a pytest-based test suite that launches X servers and sends crafted
protocol requests to verify that security vulnerabilities and other bugs
are properly handled.

It can be run against Xvfb, Xwayland, or Xorg but the latter is potentially
flaky and requires some setup outside the test suite. The test suite
uses both AddressSanitizer (ASAN) and valgrind for detecting
memory errors such as out-of-bounds reads/writes and use-after-free.


## Running tests

### Via meson
The test suite (via Xvfb) is integrated into the meson tests and can be
run with normal meson commands.

```sh
# run the python test suite
meson test --suite pyxtest
# run a set of tests
meson test pyxtest-test_randr.py
```
Consult the meson documentation for further details.

### Directly with pytest

For running against a custom path, point the test suite at the server binary to
test using environment variables or CLI options:

```sh
# Using environment variable
XVFB_PATH=build/hw/vfb/Xvfb pytest test/pyxtest/ -v

# Using --server-path
pytest test/pyxtest/ -v --server-path=build/hw/vfb/Xvfb

# Using the system Xvfb (fallback if no path is set)
pytest test/pyxtest/ -v
```

The normal pytest options work as expected (`-k` for test selection, etc.)

Tests can be run against a manually-started server using the `--display`
option:

```sh
./build/hw/vfb/Xvfb :2
pytest test/pyxtest --display :2
```

### Running with AddressSanitizer (ASAN)

ASAN is a compile-time instrumentation that detects memory errors such as
heap buffer overflows and use-after-free. To use ASAN, build the server with
sanitizer support:

```sh
meson setup build-asan -Db_sanitize=address -Db_lundef=false
meson compile -C build-asan
```

Then run the tests against the ASAN-built binary:

```sh
XSERVER_ASAN=1 XVFB_PATH=build-asan/hw/vfb/Xvfb pytest test/pyxtest/ -v
```

When using meson test, `XSERVER_ASAN` is set automatically if the build
was configured with `-Db_sanitize=address`.

Tests marked with `@pytest.mark.asan` are skipped unless `XSERVER_ASAN=1`
is set. When ASAN detects an error, the server process is killed and the
ASAN error report is included in the test failure message.

**Note:** ASAN and valgrind are mutually incompatible. When `XSERVER_ASAN=1`
is set, valgrind wrapping is automatically disabled even if `--valgrind` is
passed.

### Running with valgrind

The `--valgrind` flag runs **all** servers under valgrind:

```sh
pytest test/pyxtest/ -v --valgrind
```

Tests marked with `@pytest.mark.valgrind` automatically run their server
under valgrind even without the `--valgrind` flag. This is useful for
bugs that are only detectable via valgrind (e.g. use of uninitialised
values).

### Testing multiple server types

By default only `Xvfb` is tested. Use `--server-type` to test additional
servers. Tests using the `xserver` fixture are automatically run once per
server type:

```sh
pytest test/pyxtest/ -v --server-type=xvfb --server-type=xwayland
```

## CLI options

| Option                         | Description                               |
|--------------------------------|-------------------------------------------|
| `--valgrind`                   | Run all X servers under valgrind memcheck |
| `--valgrind-suppressions=PATH` | Path to a valgrind suppressions file      |
| `--server-type=TYPE`           | Server type to test (`xvfb`, `xwayland`, `xorg`). Repeatable. Default: `xvfb` |
| `--server-path=PATH`           | Explicit path to the X server binary      |

## Environment variables

The server binary is located by checking, in order:

1. `--server-path` CLI option
2. `XVFB_PATH` / `XWAYLAND_PATH` / `XORG_PATH` environment variable
3. `XSERVER_BUILDDIR` environment variable (looks for `hw/vfb/Xvfb` etc.)
4. `build/` directory relative to the source root
5. System `PATH` (prints a warning)

`VALGRIND_SUPPRESSIONS` can point to a suppressions file.

`XSERVER_ASAN` set to `1` indicates the server binary was built with
AddressSanitizer. This is set automatically by meson when
`-Db_sanitize=address` is used. It can also be set manually.

## Test markers

| Marker                        | Effect                                                      |
|-------------------------------|-------------------------------------------------------------|
| `@pytest.mark.asan`           | Test requires ASAN (`XSERVER_ASAN=1` must be set            |
| `@pytest.mark.valgrind`       | Test requires valgrind (skipped if `XSERVER_ASAN` is set)   |
| `@pytest.mark.xwayland_only`  | Test is skipped unless `--server-type=xwayland`             |
| `@pytest.mark.xorg_only`      | Test is skipped unless `--server-type=xorg`                 |
| `@pytest.mark.swapped_client` | Test uses a byte-swapped (big-endian) client connection     |


## Writing a new test

1. Create or edit a `test_*.py` file.

2. Use the `xserver` and `xclient` or `xclient_swapped` fixtures to get a
   running server and connection:

   ```python
   def test_something(self, xserver, xclient):
       # xclient is a RawX11Connection to xserver
       ...
       assert xserver.is_alive, "Server crashed"
   ```

3. If the test needs an extension, create a fixture that handles
   negotiation:

   ```python
   @pytest.fixture
   def render_client(xclient):
       ext = xclient.query_extension(Extension.RENDER)
       if not ext:
           pytest.skip("RENDER not available")
       # ... send version negotiation ...
       return xclient
   ```

4. Build protocol requests using dataclasses from `proto/`:

   ```python
   from proto import xi

   req = xi.XIChangeHierarchyRequest(
       opcode=opcode,
       num_changes=1,
       changes_data=change_data,
   )
   xclient.send_request(req.to_bytes())
   ```

5. If a new extension module is needed, create `proto/myext.py` with
   constants and `@dataclass` request builders following the existing
   pattern.

6. If the bug is only detectable via a memory sanitizer (OOB reads,
   use-after-free), mark the test with `@pytest.mark.asan`.  Use
   `@pytest.mark.valgrind` only for bugs that specifically require
   valgrind (e.g. use of uninitialised values that ASAN does not
   detect).
