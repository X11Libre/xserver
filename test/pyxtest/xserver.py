# SPDX-License-Identifier: MIT
#
# X server lifecycle manager
#
# Handles starting and stopping Xvfb, Xwayland, and Xorg servers,
# optionally under valgrind or with AddressSanitizer (ASAN) support,
# with automatic display number allocation via the -displayfd mechanism.

from typing import Iterator

import os
import select
import shutil
import subprocess
import tempfile
import time
import warnings
from pathlib import Path

from asan import AsanError
from valgrind import ValgrindError


class XServerProcess:
    """
    Manages an X server subprocess for testing.

    Supports Xvfb, Xwayland, and Xorg. Uses the -displayfd pipe mechanism
    for automatic display number allocation. Optionally wraps the server
    in valgrind for memory error detection, or detects AddressSanitizer
    (ASAN) errors when running an ASAN-instrumented binary.
    """

    VALGRIND_ERROR_EXIT = 99

    def __init__(
        self,
        server_type="xvfb",
        valgrind=False,
        valgrind_suppressions=None,
        asan=False,
        server_path=None,
        extra_args=None,
        log_file=None,
    ):
        self.server_type = server_type
        self.asan = asan
        if asan and valgrind:
            warnings.warn(
                "ASAN and valgrind are incompatible; disabling valgrind "
                "in favour of ASAN.",
                stacklevel=2,
            )
            valgrind = False
        self.valgrind = valgrind
        if valgrind_suppressions is None:
            default_supp = Path(__file__).resolve().parent / "valgrind.suppressions"
            self.valgrind_suppressions = (
                default_supp if default_supp.is_file() else None
            )
        else:
            self.valgrind_suppressions = valgrind_suppressions
        self._builddir = None
        self.server_path = server_path or self._find_server()
        self.extra_args = extra_args or []
        self.log_file = log_file

        self._process = None
        self._display_num = None
        self._valgrind_xml_file = None
        self._asan_log_path = None
        self._stderr_file = None
        self._weston_process = None

    def _find_server(self):
        """Discover the server binary from environment or build directory."""
        env_map = {
            "xvfb": "XVFB_PATH",
            "xwayland": "XWAYLAND_PATH",
            "xorg": "XORG_PATH",
        }
        env_var = env_map.get(self.server_type)
        if env_var and os.environ.get(env_var):
            path = Path(os.environ[env_var])
            if path.is_file() and os.access(path, os.X_OK):
                return path

        def find_meson_builddir(source_root: Path) -> Iterator[Path]:
            for d in source_root.iterdir():
                if d.is_dir() and (d / "meson-private").exists():
                    yield d

        # Try XSERVER_BUILDDIR env var or fall back to the first
        # build directory relative to this file's location
        # test/pyxtest/xserver.py -> ../../build/hw/...
        builddir = os.environ.get("XSERVER_BUILDDIR")
        if builddir is None:
            try:
                builddir = next(
                    find_meson_builddir(Path(__file__).resolve().parent.parent.parent)
                )
            except StopIteration:
                pass

        if builddir:
            build_paths = {
                "xvfb": Path(builddir, "hw", "vfb", "Xvfb"),
                "xwayland": Path(builddir, "hw", "xwayland", "Xwayland"),
                "xorg": Path(builddir, "hw", "xfree86", "Xorg"),
            }
            path = build_paths.get(self.server_type)
            if path and path.is_file() and os.access(path, os.X_OK):
                self._builddir = Path(builddir)
                return path

        # Fall back to system PATH
        binary_names = {
            "xvfb": "Xvfb",
            "xwayland": "Xwayland",
            "xorg": "Xorg",
        }
        name = binary_names.get(self.server_type)
        server_in_path = shutil.which(name)
        if server_in_path:
            if builddir:
                msg = (
                    f"Using system {self.server_type} server from PATH: {server_in_path}. "
                    f"{name} was not found in XSERVER_BUILDDIR ({builddir})."
                )
            else:
                msg = (
                    f"Using system {self.server_type} server from PATH: {server_in_path}. "
                    f"Set {env_var} or XSERVER_BUILDDIR to use a specific build."
                )
            warnings.warn(msg, stacklevel=2)
            return server_in_path

        raise FileNotFoundError(
            f"Failed to find {self.server_type} server binary. "
            f"Set {env_var} environment variable or build the server first."
        )

    @property
    def display_num(self):
        return self._display_num

    @property
    def display(self):
        if self._display_num is not None:
            return f":{self._display_num}"
        return None

    def start(self, timeout=10):
        """Start the X server and wait for it to be ready."""
        read_fd, write_fd = os.pipe()

        cmd = self._build_command(write_fd)

        self._stderr_file = tempfile.NamedTemporaryFile(
            prefix=f"xserver-{self.server_type}-stderr-",
            suffix=".log",
            delete=False,
            mode="w",
        )

        if self.server_type == "xwayland":
            self._start_wayland_compositor()

        env = os.environ.copy()
        if self._weston_process:
            env.setdefault("WAYLAND_DISPLAY", "wayland-security-test")

        if self.asan:
            self._setup_asan_env(env)

        try:
            self._process = subprocess.Popen(
                cmd,
                stderr=self._stderr_file,
                pass_fds=(write_fd,),
                env=env,
            )
        except Exception:
            os.close(write_fd)
            os.close(read_fd)
            raise

        os.close(write_fd)

        try:
            display_bytes = b""
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                if self._process.poll() is not None:
                    display_bytes = None
                    break

                ready, _, _ = select.select([read_fd], [], [], 1.0)
                if ready:
                    chunk = os.read(read_fd, 64)
                    if not chunk:
                        break
                    display_bytes += chunk
                    if b"\n" in display_bytes:
                        break

            if not display_bytes:
                if self._process.poll() is not None:
                    stderr_content = self._read_stderr()
                    raise RuntimeError(
                        f"X server exited with code {self._process.returncode} "
                        f"before sending display number (waited {timeout}s).\n"
                        f"Command: {' '.join(str(s) for s in cmd)}\n"
                        f"Stderr:\n{stderr_content}"
                    )
                self.stop()
                raise RuntimeError(
                    f"Timed out waiting for X server display number (waited {timeout}s)"
                )

            self._display_num = int(display_bytes.strip())

        finally:
            os.close(read_fd)

        return self._display_num

    def _setup_xorg_modules(self):
        """Create a module directory layout for Xorg from a meson build tree.

        Xorg's module loader expects drivers in a ``drivers/``
        subdirectory, extensions in ``extensions/``, etc.  The meson
        build tree puts them in separate build directories, so we
        create a temporary directory with the right structure using
        symlinks.

        Returns the path to the module directory, or None if the build
        directory doesn't contain the expected modules.
        """
        xfree86 = self._builddir / "hw" / "xfree86"
        if not xfree86.is_dir():
            return None

        moddir = Path(tempfile.mkdtemp(prefix="xorg-modules-"))

        # Map of build subdirectories to module layout subdirectories.
        # Entries are (build_subdir, module_subdir, glob_pattern).
        module_dirs = [
            ("drivers/modesetting", "drivers", "*_drv.so"),
            ("drivers/inputtest", "input", "*_drv.so"),
            ("dixmods", "extensions", "libglx.so"),
        ]
        for build_sub, mod_sub, pattern in module_dirs:
            src = xfree86 / build_sub
            if not src.is_dir():
                continue
            dst = moddir / mod_sub
            dst.mkdir(exist_ok=True)
            for f in src.glob(pattern):
                (dst / f.name).symlink_to(f.resolve())

        # Other modules go at the top level
        top_level_modules = [
            "dixmods/libwfb.so",
            "dixmods/libshadow.so",
            "glamor_egl/libglamoregl.so",
            "exa/libexa.so",
            "fbdevhw/libfbdevhw.so",
            "int10/libint10.so",
            "shadowfb/libshadowfb.so",
            "vgahw/libvgahw.so",
        ]
        for rel in top_level_modules:
            src = xfree86 / rel
            if src.is_file():
                (moddir / src.name).symlink_to(src.resolve())

        return moddir

    def _build_command(self, displayfd):
        """Build the full command line including optional valgrind wrapper."""
        server_args = [self.server_path]

        if self.server_type == "xvfb":
            server_args.extend(
                [
                    "-screen",
                    "scrn",
                    "1280x1024x24",
                ]
            )
        elif self.server_type == "xwayland":
            server_args.extend(["-nokeymap"])
        elif self.server_type == "xorg":
            # -logfile is only permitted if we're running as root
            if self.log_file and os.geteuid() == 0:
                server_args.extend(["-logfile", str(self.log_file)])

            # When running Xorg from a build directory, set up the
            # module path and use an empty config so it auto-detects
            # the GPU via modesetting.
            if self._builddir:
                modpath = self._setup_xorg_modules()
                if modpath:
                    server_args.extend(["-modulepath", str(modpath)])
                server_args.extend(["-config", "/dev/null", "-configdir", "/dev/null"])

        server_args.extend(["-noreset", "+byteswappedclients"])

        # Auto-detect xkb directory if the compiled-in default doesn't exist
        xkb_dir = os.environ.get("XKB_CONFIG_ROOT")
        if not xkb_dir:
            for candidate in ["/usr/share/X11/xkb", "/usr/local/share/X11/xkb"]:
                if os.path.isdir(candidate):
                    xkb_dir = candidate
                    break
        if xkb_dir:
            server_args.extend(["-xkbdir", xkb_dir])

        server_args.extend(["-displayfd", str(displayfd)])
        server_args.extend(self.extra_args)

        if not self.valgrind:
            return server_args

        self._valgrind_xml_file = tempfile.NamedTemporaryFile(
            prefix=f"xserver-{self.server_type}-valgrind-", suffix=".xml", delete=False
        )
        self._valgrind_xml_file.close()

        valgrind_cmd = [
            "valgrind",
            "--tool=memcheck",
            "--leak-check=full",
            "--track-origins=yes",
            "--show-reachable=no",
            "--gen-suppressions=all",
            f"--error-exitcode={self.VALGRIND_ERROR_EXIT}",
            "--xml=yes",
            f"--xml-file={self._valgrind_xml_file.name}",
        ]

        if self.valgrind_suppressions:
            valgrind_cmd.append(f"--suppressions={self.valgrind_suppressions}")

        return valgrind_cmd + server_args

    def _setup_asan_env(self, env):
        """Configure ASAN_OPTIONS for the server subprocess.

        Sets up a log file for ASAN output and configures ASAN to not
        detect leaks (too noisy for these tests).  Any existing
        ASAN_OPTIONS from the environment are preserved.
        """
        asan_log_file = tempfile.NamedTemporaryFile(
            prefix=f"xserver-{self.server_type}-asan-",
            suffix=".log",
            delete=False,
        )
        self._asan_log_path = Path(asan_log_file.name)
        asan_log_file.close()

        asan_opts = {
            "log_path": str(self._asan_log_path),
            "detect_leaks": "0",
        }

        # Merge with any existing ASAN_OPTIONS from the environment
        existing = env.get("ASAN_OPTIONS", "")
        for part in existing.split(":"):
            part = part.strip()
            if "=" in part:
                key, val = part.split("=", 1)
                # Don't override our log_path
                if key not in asan_opts:
                    asan_opts[key] = val

        env["ASAN_OPTIONS"] = ":".join(f"{k}={v}" for k, v in asan_opts.items())

    def _start_wayland_compositor(self):
        """Start weston as a headless compositor for Xwayland testing."""
        if not shutil.which("weston"):
            raise FileNotFoundError(
                "weston is required for Xwayland testing but was not found"
            )

        self._weston_process = subprocess.Popen(
            ["weston", "--no-config", "--backend=headless-backend.so"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        time.sleep(1)
        if self._weston_process.poll() is not None:
            raise RuntimeError(
                f"weston exited with code {self._weston_process.returncode}"
            )

    @property
    def is_alive(self) -> bool:
        """Check if the server process is still running."""
        if self._process is None:
            return False
        return self._process.poll() is None

    @property
    def crash_signal(self) -> int | None:
        """If the server crashed, return the signal number. Otherwise None."""
        if self._process is None:
            return None
        ret = self._process.poll()
        if ret is not None and ret < 0:
            return -ret
        return None

    def stop(self):
        """Stop the server and return any valgrind/ASAN errors."""
        errors = []

        if self._process and self._process.poll() is None:
            self._process.terminate()
            try:
                self._process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self._process.kill()
                self._process.wait(timeout=5)

        if self._weston_process and self._weston_process.poll() is None:
            self._weston_process.terminate()
            try:
                self._weston_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._weston_process.kill()
                self._weston_process.wait()

        if self.valgrind and self._valgrind_xml_file:
            errors = ValgrindError.from_xml(Path(self._valgrind_xml_file.name))

        if self._stderr_file:
            self._stderr_file.close()

        return errors

    def _read_stderr(self):
        """Read the captured stderr content."""
        if self._stderr_file:
            self._stderr_file.flush()
            try:
                return Path(self._stderr_file.name).read_text()
            except OSError:
                pass
        return ""

    def check_valgrind_errors(self):
        """Parse and assert no valgrind errors occurred."""
        if self._valgrind_xml_file is None:
            return []
        errors = ValgrindError.from_xml(Path(self._valgrind_xml_file.name))
        if errors:
            msg = f"Valgrind found {len(errors)} error(s):\n\n"
            msg += "\n\n".join(str(e) for e in errors)
            raise AssertionError(msg)
        return errors

    def get_asan_errors(self) -> list[AsanError]:
        """Parse ASAN errors from log file and/or stderr."""
        errors: list[AsanError] = []

        # Check the ASAN log file(s) first (ASAN appends .<pid> to log_path)
        if self._asan_log_path:
            errors.extend(AsanError.from_log(self._asan_log_path))

        # Also check stderr in case ASAN wrote there
        stderr_text = self._read_stderr()
        if stderr_text and "AddressSanitizer" in stderr_text:
            stderr_errors = AsanError.from_text(stderr_text)
            # Avoid duplicates: only add stderr errors if log file had none
            if not errors:
                errors.extend(stderr_errors)

        return errors

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()
        return False

    def __repr__(self):
        state = "running" if self.is_alive else "stopped"
        display = self.display or "N/A"
        flags = ""
        if self.valgrind:
            flags += " +valgrind"
        if self.asan:
            flags += " +asan"
        return f"<XServerProcess {self.server_type} {display} [{state}]{flags}>"


class ExternalXServer:
    """Proxy for an externally-managed X server (``--display`` mode).

    Used when the user passes ``--display`` to connect to an already-running
    server instead of launching one per test.  The server's PID is discovered
    via ``SO_PEERCRED`` on the Unix socket so that :attr:`is_alive` can
    report whether the process is still running.
    """

    def __init__(self, display_num, server_type="external"):
        self._display_num = display_num
        self.server_type = server_type
        self._pid = self._get_server_pid()

    def _get_server_pid(self):
        """Get the PID of the X server via SO_PEERCRED on the Unix socket."""
        import socket as _socket
        import struct as _struct

        path = f"/tmp/.X11-unix/X{self._display_num}"
        try:
            sock = _socket.socket(_socket.AF_UNIX, _socket.SOCK_STREAM)
            sock.connect(path)
            cred = sock.getsockopt(
                _socket.SOL_SOCKET, _socket.SO_PEERCRED, _struct.calcsize("iii")
            )
            pid, _, _ = _struct.unpack("iii", cred)
            sock.close()
            return pid
        except (OSError, _struct.error):
            return None

    @property
    def display_num(self):
        return self._display_num

    @property
    def display(self):
        return f":{self._display_num}"

    @property
    def is_alive(self) -> bool:
        """Check if the external server process is still running."""
        if self._pid is None:
            return True  # can't check, assume alive
        try:
            os.kill(self._pid, 0)
            return True
        except ProcessLookupError:
            return False
        except PermissionError:
            return True  # process exists but we can't signal it

    @property
    def crash_signal(self) -> int | None:
        """Always None -- we cannot determine this for external servers."""
        return None

    def stop(self):
        """No-op -- never stop an external server."""
        return []

    def get_asan_errors(self):
        """No ASAN log access for external servers."""
        return []

    def check_valgrind_errors(self):
        """No valgrind access for external servers."""
        return []

    def __repr__(self):
        state = "running" if self.is_alive else "stopped"
        return f"<ExternalXServer :{self._display_num} [{state}] pid={self._pid}>"
