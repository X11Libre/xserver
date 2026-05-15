# SPDX-License-Identifier: MIT
#
# X11 client connection utilities

import select
import socket
import struct
import time
from dataclasses import dataclass
from enum import StrEnum

from proto.bigrequests import BigRequestsEnableRequest
from proto.x11 import (
    CreatePixmapRequest,
    CreateWindowRequest,
    InternAtomRequest,
    QueryExtensionRequest,
)
from proto import xkb


class X11ConnectionError(Exception):
    """Raised when the X11 connection fails."""

    pass


@dataclass
class XExtensionData:
    """Cached result of a QueryExtension reply."""

    opcode: int
    first_event: int
    first_error: int


class Extension(StrEnum):
    """X11 extension wire names as used in QueryExtension requests."""

    BIG_REQUESTS = "BIG-REQUESTS"
    COMPOSITE = "Composite"
    DAMAGE = "DAMAGE"
    DBE = "DOUBLE-BUFFER"
    DPMS = "DPMS"
    DRI2 = "DRI2"
    DRI3 = "DRI3"
    GENERIC_EVENT = "Generic Event Extension"
    GLX = "GLX"
    MIT_SCREEN_SAVER = "MIT-SCREEN-SAVER"
    MIT_SHM = "MIT-SHM"
    PRESENT = "Present"
    RANDR = "RANDR"
    RECORD = "RECORD"
    RENDER = "RENDER"
    SECURITY = "SECURITY"
    SHAPE = "SHAPE"
    SYNC = "SYNC"
    XC_MISC = "XC-MISC"
    XF86BIGFONT = "XFree86-Bigfont"
    XF86DGA = "XFree86-DGA"
    XF86VIDMODE = "XFree86-VidModeExtension"
    XFIXES = "XFIXES"
    XI = "XInputExtension"
    XRES = "X-Resource"
    XINERAMA = "XINERAMA"
    XKB = "XKEYBOARD"
    XTEST = "XTEST"
    XVIDEO = "XVideo"
    XVIDEO_MC = "XVideo-MotionCompensation"


# X11 core protocol error codes (from X.h)
BadRequest = 1
BadValue = 2
BadWindow = 3
BadPixmap = 4
BadAtom = 5
BadCursor = 6
BadFont = 7
BadMatch = 8
BadDrawable = 9
BadAccess = 10
BadAlloc = 11
BadColor = 12
BadGC = 13
BadIDChoice = 14
BadName = 15
BadLength = 16
BadImplementation = 17


@dataclass
class X11Error:
    """An X11 error reply from the server."""

    response_type: int
    error_code: int
    sequence: int
    resource_id: int
    minor_code: int
    major_code: int

    @classmethod
    def from_data(cls, data: bytes, byte_order: str = "<") -> "X11Error":
        if len(data) < 32:
            data = data + b"\x00" * (32 - len(data))
        response_type, error_code, sequence, resource_id, minor_code, major_code = (
            struct.unpack_from(f"{byte_order}BBHIHB", data)
        )
        return cls(
            response_type, error_code, sequence, resource_id, minor_code, major_code
        )

    def __repr__(self):
        return (
            f"<X11Error code={self.error_code} seq={self.sequence} "
            f"major={self.major_code} minor={self.minor_code}>"
        )


@dataclass
class X11Reply:
    """An X11 reply from the server."""

    data: bytes
    response_type: int
    sequence: int
    length: int

    @classmethod
    def from_data(cls, data: bytes, byte_order: str = "<") -> "X11Reply":
        if len(data) >= 8:
            response_type = data[0]
            sequence = struct.unpack_from(f"{byte_order}H", data, 2)[0]
            length = struct.unpack_from(f"{byte_order}I", data, 4)[0]
        else:
            response_type = sequence = length = 0
        return cls(data, response_type, sequence, length)

    def __repr__(self):
        return (
            f"<X11Reply seq={self.sequence} "
            f"extra_len={self.length} total={len(self.data)}>"
        )


class RawX11Connection:
    """
    Minimal X11 connection for sending raw (possibly malformed) requests.

    Set swapped=True for testing byte-swap code paths (SProcXxx).
    """

    def __init__(self, display_num, swapped=False):
        self.display_num = display_num
        self.swapped = swapped
        self._byte_order = ">" if swapped else "<"
        self.sock = None
        self.seq = 0
        self.root_window = 0
        self.root_visual = 0
        self.root_depth = 0
        self._resource_id_base = 0
        self._resource_id_mask = 0
        self._next_resource_id = 0
        self._extensions = {}
        self._connect()

    def _connect(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        path = f"/tmp/.X11-unix/X{self.display_num}"
        try:
            self.sock.connect(path)
        except (ConnectionRefusedError, FileNotFoundError) as e:
            self.sock.close()
            self.sock = None
            raise X11ConnectionError(f"Cannot connect to {path}: {e}")
        try:
            self._handshake()
        except Exception:
            self.sock.close()
            self.sock = None
            raise

    def _handshake(self):
        byte_order = 0x42 if self.swapped else 0x6C
        bo = self._byte_order
        setup = struct.pack(f"{bo}BxHHHHxx", byte_order, 11, 0, 0, 0)
        assert self.sock
        self.sock.sendall(setup)

        header = self._recv_exact(8)
        status = header[0]

        if status == 0:
            reason_len = header[1]
            extra_len = struct.unpack_from(f"{bo}H", header, 6)[0]
            extra = self._recv_exact(extra_len * 4)
            reason = extra[:reason_len].decode("ascii", errors="replace")
            raise X11ConnectionError(f"X server refused connection: {reason}")

        if status == 2:
            raise X11ConnectionError("X server requires authentication")
        if status != 1:
            raise X11ConnectionError(f"Unexpected setup status: {status}")

        extra_len = struct.unpack_from(f"{bo}H", header, 6)[0]
        data = self._recv_exact(extra_len * 4)

        (_, res_base, res_mask, _, vendor_len, _, num_screens, num_formats) = (
            struct.unpack_from(f"{bo}IIIIHH BB", data, 0)
        )

        self._resource_id_base = res_base
        self._resource_id_mask = res_mask
        self._next_resource_id = 1

        offset = 32 + ((vendor_len + 3) & ~3) + num_formats * 8

        if num_screens > 0 and offset + 40 <= len(data):
            (
                self.root_window,
                _,
                _,
                _,
                _,
                _,
                _,
                _,
                _,
                _,
                _,
                self.root_visual,
                _,
                _,
                self.root_depth,
            ) = struct.unpack_from(f"{bo}IIIIIHHHHHHI BBB", data, offset)

    # --- Core protocol helpers ---

    def alloc_id(self) -> int:
        xid = self._resource_id_base | self._next_resource_id
        self._next_resource_id += 1
        return xid

    def send_request(self, data: bytes) -> int:
        self.seq += 1
        assert self.sock
        self.sock.sendall(data)
        return self.seq

    def recv_response(self, timeout: float = 5.0) -> X11Error | X11Reply | None:
        ready = select.select([self.sock], [], [], timeout)
        if not ready[0]:
            return None
        try:
            header = self._recv_exact(32, timeout=timeout)
        except (ConnectionResetError, BrokenPipeError, OSError, X11ConnectionError):
            return None
        if not header:
            return None

        rtype = header[0]
        bo = self._byte_order
        if rtype == 0:
            return X11Error.from_data(header, bo)
        elif rtype == 1:
            extra_len = struct.unpack_from(f"{bo}I", header, 4)[0]
            if extra_len > 0:
                try:
                    extra = self._recv_exact(extra_len * 4, timeout=timeout)
                    return X11Reply.from_data(header + extra, bo)
                except (
                    ConnectionResetError,
                    BrokenPipeError,
                    OSError,
                    X11ConnectionError,
                ):
                    return X11Reply.from_data(header, bo)
            return X11Reply.from_data(header, bo)
        else:
            return X11Reply.from_data(header, bo)

    def flush_responses(self, timeout: float = 0.5) -> list[X11Error | X11Reply]:
        responses: list[X11Error | X11Reply] = []
        while True:
            resp = self.recv_response(timeout=timeout)
            if resp is None:
                break
            responses.append(resp)
        return responses

    def is_connected(self) -> bool:
        assert self.sock
        try:
            ready = select.select([self.sock], [], [], 0)
            if ready[0]:
                data = self.sock.recv(1, socket.MSG_PEEK)
                return len(data) > 0
            return True
        except (ConnectionResetError, BrokenPipeError, OSError):
            return False

    def wait_for_disconnect(self, timeout: float = 5.0) -> bool:
        assert self.sock
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            ready = select.select([self.sock], [], [], min(remaining, 0.5))
            if ready[0]:
                try:
                    data = self.sock.recv(4096)
                    if not data:
                        return True
                except (ConnectionResetError, BrokenPipeError, OSError):
                    return True
        return False

    # --- Extension negotiation ---

    def query_extension(self, name: str) -> XExtensionData | None:
        if name in self._extensions:
            return self._extensions[name]

        req = QueryExtensionRequest(name=name)
        self.send_request(req.to_bytes(self._byte_order))
        resp = self.recv_response(timeout=5.0)
        if resp is None or isinstance(resp, X11Error):
            return None

        if len(resp.data) >= 12:
            present, major, first_event, first_error = struct.unpack_from(
                "BBBB", resp.data, 8
            )
            if present:
                result = XExtensionData(major, first_event, first_error)
                self._extensions[name] = result
                return result
        return None

    def xkb_use_extension(self, major_version: int = 1, minor_version: int = 0) -> int:
        ext = self.query_extension(Extension.XKB)
        if not ext:
            raise X11ConnectionError("XKB extension not available")

        req = xkb.use_extension(
            ext.opcode,
            major=major_version,
            minor=minor_version,
            byte_order=self._byte_order,
        )
        self.send_request(req)

        resp = self.recv_response(timeout=5.0)
        if isinstance(resp, X11Error):
            raise X11ConnectionError(f"XkbUseExtension failed: error {resp.error_code}")
        return ext.opcode

    def enable_big_requests(self) -> int:
        ext = self.query_extension(Extension.BIG_REQUESTS)
        if not ext:
            raise X11ConnectionError("BIG-REQUESTS not available")

        req = BigRequestsEnableRequest(opcode=ext.opcode)
        self.send_request(req.to_bytes(self._byte_order))

        resp = self.recv_response(timeout=5.0)
        if isinstance(resp, X11Error):
            raise X11ConnectionError("BigRequestsEnable failed")
        if resp and len(resp.data) >= 12:
            return struct.unpack_from(f"{self._byte_order}I", resp.data, 8)[0]
        return 0

    # --- Resource creation helpers ---

    def create_window(
        self,
        width: int = 100,
        height: int = 100,
        depth: int | None = None,
        parent: int | None = None,
        x: int = 0,
        y: int = 0,
    ) -> int:
        wid = self.alloc_id()
        if parent is None:
            parent = self.root_window
        if depth is None:
            depth = self.root_depth

        assert parent is not None
        assert depth is not None

        req = CreateWindowRequest(
            wid=wid,
            parent=parent,
            x=x,
            y=y,
            width=width,
            height=height,
            depth=depth,
        )
        self.send_request(req.to_bytes(self._byte_order))
        self.flush_responses(timeout=0.2)
        return wid

    def create_pixmap(
        self,
        width: int = 100,
        height: int = 100,
        depth: int | None = None,
        drawable: int | None = None,
    ) -> int:
        pid = self.alloc_id()
        if drawable is None:
            drawable = self.root_window
        if depth is None:
            depth = self.root_depth

        assert drawable is not None
        assert depth is not None

        req = CreatePixmapRequest(
            pid=pid,
            drawable=drawable,
            width=width,
            height=height,
            depth=depth,
        )
        self.send_request(req.to_bytes(self._byte_order))
        self.flush_responses(timeout=0.2)
        return pid

    def intern_atom(self, name: str, only_if_exists: bool = False) -> int:
        req = InternAtomRequest(name=name, only_if_exists=only_if_exists)
        self.send_request(req.to_bytes(self._byte_order))

        resp = self.recv_response(timeout=5.0)
        if isinstance(resp, X11Error) or resp is None:
            return 0
        if len(resp.data) >= 12:
            return struct.unpack_from(f"{self._byte_order}I", resp.data, 8)[0]
        return 0

    def get_fd(self) -> int:
        assert self.sock
        return self.sock.fileno()

    def close(self) -> None:
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
            self.sock = None

    def _recv_exact(self, nbytes: int, timeout: float = 10.0) -> bytes:
        assert self.sock
        data = b""
        deadline = time.monotonic() + timeout
        while len(data) < nbytes:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise X11ConnectionError(
                    f"Timeout reading ({len(data)}/{nbytes} bytes)"
                )
            ready = select.select([self.sock], [], [], min(remaining, 1.0))
            if ready[0]:
                chunk = self.sock.recv(nbytes - len(data))
                if not chunk:
                    raise X11ConnectionError(
                        f"Connection closed ({len(data)}/{nbytes} bytes)"
                    )
                data += chunk
        return data

    def __enter__(self) -> "RawX11Connection":
        return self

    def __exit__(self, *args) -> bool:
        self.close()
        return False

    def __del__(self) -> None:
        self.close()


class XlibConnection:
    """python-xlib based connection for higher-level X operations."""

    def __init__(self, display_num):
        from Xlib import display as xlib_display

        self.display_num = display_num
        self.display = xlib_display.Display(f":{display_num}")
        self.screen = self.display.screen()
        self.root = self.screen.root

    def get_fd(self):
        return self.display.fileno()

    def create_window(self, width=100, height=100, x=0, y=0):
        from Xlib import X

        window = self.root.create_window(
            x,
            y,
            width,
            height,
            0,
            self.screen.root_depth,
            X.InputOutput,
            X.CopyFromParent,
        )
        self.display.sync()
        return window

    def flush(self):
        self.display.flush()

    def sync(self):
        self.display.sync()

    def close(self):
        try:
            self.display.close()
        except Exception:
            pass

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
        return False
