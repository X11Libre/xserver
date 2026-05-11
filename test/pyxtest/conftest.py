# SPDX-License-Identifier: MIT
#
# pytest configuration and fixtures for X server testing.

import os
import shutil
import pytest
from pathlib import Path

from xserver import ExternalXServer, XServerProcess
from xclient import RawX11Connection, X11ConnectionError, XlibConnection


def pytest_addoption(parser):
    parser.addoption(
        "--valgrind",
        action="store_true",
        default=False,
        help="Run X server under valgrind memcheck",
    )
    parser.addoption(
        "--valgrind-suppressions",
        default=None,
        help="Path to valgrind suppressions file",
    )
    parser.addoption(
        "--server-type",
        action="append",
        default=[],
        help="Server types to test (xvfb, xwayland, xorg). "
        "Can be specified multiple times. Default: xvfb",
    )
    parser.addoption(
        "--server-path", default=None, help="Explicit path to the X server binary"
    )
    parser.addoption(
        "--display",
        default=None,
        help="Connect to an existing X server instead of starting one. "
        "Value is a display number or :N string (e.g. '42' or ':42'). "
        "Optionally combine with --server-type to declare the server type "
        "for marker-based test filtering.",
    )


def pytest_configure(config):
    config.addinivalue_line(
        "markers", "valgrind: mark test as requiring valgrind to detect the issue"
    )
    config.addinivalue_line(
        "markers",
        "asan: mark test as requiring AddressSanitizer to detect the issue",
    )
    config.addinivalue_line(
        "markers", "xwayland_only: mark test as only applicable to Xwayland"
    )
    config.addinivalue_line(
        "markers", "xorg_only: mark test as only applicable to Xorg"
    )
    config.addinivalue_line(
        "markers", "swapped_client: mark test as requiring a byte-swapped client"
    )

    # Validate --display against conflicting options
    display = config.getoption("--display", default=None)
    if display is not None:
        if config.getoption("--valgrind"):
            raise pytest.UsageError("--display and --valgrind are mutually exclusive")
        if config.getoption("--server-path"):
            raise pytest.UsageError(
                "--display and --server-path are mutually exclusive"
            )


def _parse_display(value):
    """Parse a display string like ':42' or '42' into an integer."""
    value = value.strip().lstrip(":")
    try:
        return int(value)
    except ValueError:
        raise pytest.UsageError(f"Invalid display value: {value!r}")


def get_server_types(config) -> list[str]:
    """Get the list of server types to test.

    With ``--display``, defaults to ``["external"]`` if no explicit
    ``--server-type`` is given.  Otherwise defaults to ``["xvfb"]``.
    """
    types = config.getoption("--server-type") or []
    if config.getoption("--display", default=None) is not None:
        return types or ["external"]
    return types or ["xvfb"]


def get_valgrind_suppressions(config) -> Path | None:
    """Find the valgrind suppressions file."""
    explicit = config.getoption("--valgrind-suppressions")
    if explicit:
        return Path(explicit)

    env = os.environ.get("VALGRIND_SUPPRESSIONS")
    if env:
        f = Path(env)
        if f.is_file():
            return f

    # Try next to this file (test/pyxtest/valgrind.suppressions)
    default = Path(__file__).resolve().parent / "valgrind.suppressions"
    if default.is_file():
        return default

    return None


def is_valgrind_available() -> bool:
    """Check if valgrind is available on the system."""
    return shutil.which("valgrind") is not None


def is_asan_build() -> bool:
    """Check if the X server binary was built with AddressSanitizer.

    This is determined by the ``XSERVER_ASAN`` environment variable
    which is set by meson when ``-Db_sanitize=address`` is used.
    It can also be set manually when running pytest directly.
    """
    return os.environ.get("XSERVER_ASAN") == "1"


def pytest_collection_modifyitems(config, items):
    """Skip tests based on markers and configuration."""
    server_types = get_server_types(config)
    asan = is_asan_build()

    for item in items:
        if item.get_closest_marker("xwayland_only") and "xwayland" not in server_types:
            item.add_marker(pytest.mark.skip(reason="Test only applies to Xwayland"))

        if item.get_closest_marker("xorg_only") and "xorg" not in server_types:
            item.add_marker(pytest.mark.skip(reason="Test only applies to Xorg"))

        if item.get_closest_marker("asan") and not asan:
            item.add_marker(
                pytest.mark.skip(
                    reason="Test requires ASAN build (XSERVER_ASAN=1 not set)"
                )
            )

        if item.get_closest_marker("valgrind") and asan:
            item.add_marker(
                pytest.mark.skip(
                    reason="Test requires valgrind, incompatible with ASAN build"
                )
            )


def pytest_generate_tests(metafunc):
    """Parametrize tests that use the xserver fixture over all configured
    server types so each test runs once per type."""
    if "xserver" in metafunc.fixturenames:
        server_types = get_server_types(metafunc.config)
        metafunc.parametrize("xserver", server_types, indirect=True)


def _start_server(request, server_type, log_file=None):
    """Start an X server of the given type for a test.

    Shared implementation for the xvfb, xwayland, xorg, and generic
    xserver fixtures.  Valgrind is enabled if the test is marked with
    ``@pytest.mark.valgrind`` or if ``--valgrind`` is passed on the
    command line.  ASAN is enabled automatically when ``XSERVER_ASAN=1``
    is set in the environment (typically by meson when the server is
    built with ``-Db_sanitize=address``).
    """
    if server_type == "xorg" and os.geteuid() != 0:
        pytest.skip("Xorg requires root to access /dev/tty0 and GPU devices")

    use_valgrind = (
        request.config.getoption("--valgrind")
        or request.node.get_closest_marker("valgrind") is not None
    )
    if use_valgrind and not is_valgrind_available():
        pytest.skip("valgrind not available")
    use_asan = is_asan_build()
    server_path = request.config.getoption("--server-path")
    suppressions = get_valgrind_suppressions(request.config)

    server = XServerProcess(
        server_type=server_type,
        valgrind=use_valgrind,
        valgrind_suppressions=suppressions,
        asan=use_asan,
        server_path=server_path,
        log_file=log_file,
    )

    try:
        server.start(timeout=60 if use_valgrind else 15)
    except (FileNotFoundError, RuntimeError) as e:
        msg = f"Failed to start {server_type} server: {e}"
        if server.log_file:
            msg += f"\nLog file: {server.log_file}"
        pytest.fail(msg)

    yield server

    # Check if the server was killed by ASAN before we stop it
    asan_errors = []
    if use_asan and not server.is_alive:
        asan_errors = server.get_asan_errors()

    valgrind_errors = server.stop()

    if use_asan and asan_errors:
        msg = f"AddressSanitizer found {len(asan_errors)} error(s):\n\n"
        msg += "\n\n".join(str(e) for e in asan_errors)
        pytest.fail(msg)

    if use_valgrind and valgrind_errors:
        serious = [
            e
            for e in valgrind_errors
            if e.kind
            not in (
                "Leak_DefinitelyLost",
                "Leak_PossiblyLost",
                "Leak_StillReachable",
                "Leak_IndirectlyLost",
                "SyscallParam",
            )
        ]
        if serious:
            msg = f"Valgrind found {len(serious)} memory error(s):\n\n"
            msg += "\n\n".join(str(e) for e in serious)
            pytest.fail(msg)


@pytest.fixture
def xserver(request, tmp_path):
    """
    Start an X server for this test.

    Automatically parametrized via ``pytest_generate_tests`` so every
    test that uses this fixture runs once per configured --server-type.
    A fresh server per test, killed afterward.  With --valgrind,
    valgrind memory errors cause test failure during teardown.

    When ``--display`` is given, no server is started; an
    :class:`ExternalXServer` proxy is yielded instead.

    For a fixture that targets a specific server type use the xvfb,
    xwayland, or xorg fixtures instead.
    """
    server_type = request.param

    # Skip server-specific markers
    if request.node.get_closest_marker("xwayland_only") and server_type != "xwayland":
        pytest.skip("Test only applies to Xwayland")
    if request.node.get_closest_marker("xorg_only") and server_type != "xorg":
        pytest.skip("Test only applies to Xorg")

    # External server mode: no server lifecycle management
    display = request.config.getoption("--display")
    if display is not None:
        display_num = _parse_display(display)
        yield ExternalXServer(display_num, server_type=server_type)
        return

    kwargs = {}
    if server_type == "xorg":
        kwargs["log_file"] = tmp_path / f"{server_type}.log"

    yield from _start_server(request, server_type, **kwargs)


@pytest.fixture
def xvfb(request, tmp_path):
    """Start an Xvfb server for this test."""
    display = request.config.getoption("--display")
    if display is not None:
        yield ExternalXServer(_parse_display(display), server_type="xvfb")
        return
    if "xvfb" not in get_server_types(request.config):
        pytest.skip("Xvfb not in --server-type list")
    yield from _start_server(request, "xvfb")


@pytest.fixture
def xwayland(request, tmp_path):
    """Start an Xwayland server for this test."""
    display = request.config.getoption("--display")
    if display is not None:
        yield ExternalXServer(_parse_display(display), server_type="xwayland")
        return
    if "xwayland" not in get_server_types(request.config):
        pytest.skip("Xwayland not in --server-type list")
    yield from _start_server(request, "xwayland")


@pytest.fixture
def xorg(request, tmp_path):
    """Start an Xorg server for this test."""
    display = request.config.getoption("--display")
    if display is not None:
        yield ExternalXServer(_parse_display(display), server_type="xorg")
        return
    if "xorg" not in get_server_types(request.config):
        pytest.skip("Xorg not in --server-type list")
    yield from _start_server(request, "xorg", log_file=tmp_path / "xorg.log")


@pytest.fixture
def xclient(xserver):
    """Create a raw X11 connection to the test server."""
    conn = RawX11Connection(xserver.display_num)
    yield conn
    conn.close()


@pytest.fixture
def xclient_swapped(xserver):
    """Create a big-endian (byte-swapped) X11 connection."""
    try:
        conn = RawX11Connection(xserver.display_num, swapped=True)
    except X11ConnectionError as e:
        if "endian" in str(e).lower():
            pytest.skip("Server does not accept big-endian clients")
        raise
    yield conn
    conn.close()


@pytest.fixture
def xlib_client(xserver):
    """Create a python-xlib connection for higher-level X operations."""
    conn = XlibConnection(xserver.display_num)
    yield conn
    conn.close()
