# SPDX-License-Identifier: MIT
#
# Python abstraction for the xf86-input-inputtest driver's binary socket
# protocol. Allows tests to inject input events (motion, gestures, etc.)
# into an Xorg server that has the inputtest driver loaded.
#
# Protocol reference:
#   hw/xfree86/drivers/inputtest/xf86-input-inputtest-protocol.h

import socket
import struct
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Self

from proto.xi import (
    XI_GestureSwipeBegin,
    XI_GestureSwipeEnd,
    XI_GestureSwipeUpdate,
)

XF86IT_PROTOCOL_VERSION_MAJOR = 1
XF86IT_PROTOCOL_VERSION_MINOR = 1

XF86IT_RESPONSE_SERVER_VERSION = 0
XF86IT_RESPONSE_SYNC_FINISHED = 1

XF86IT_EVENT_CLIENT_VERSION = 0
XF86IT_EVENT_WAIT_FOR_SYNC = 1
XF86IT_EVENT_MOTION = 2
XF86IT_EVENT_PROXIMITY = 3
XF86IT_EVENT_BUTTON = 4
XF86IT_EVENT_KEY = 5
XF86IT_EVENT_TOUCH = 6
XF86IT_EVENT_GESTURE_PINCH = 7
XF86IT_EVENT_GESTURE_SWIPE = 8


class Message:
    def pack(self) -> bytes:
        raise NotImplementedError()


class Response:
    SIZE: int

    length: int
    resp_type: int

    @classmethod
    def unpack(cls, data: bytes) -> Self:
        raise NotImplementedError()


@dataclass
class MessageClientVersion(Message):
    """xf86ITEventClientVersion: header(8) + major(2) + minor(2) = 12 bytes."""

    major: int
    minor: int

    def pack(self) -> bytes:
        return struct.pack(
            "=II HH",
            12,  # length
            XF86IT_EVENT_CLIENT_VERSION,
            self.major,
            self.minor,
        )


@dataclass
class ResponseServerVersion(Response):
    """xf86ITResponseServerVersion: header(8) + major(2) + minor(2) = 12 bytes."""

    SIZE = 12

    length: int
    resp_type: int
    major: int
    minor: int

    @classmethod
    def unpack(cls, data: bytes) -> Self:
        length, resp_type, major, minor = struct.unpack("=II HH", data)
        return cls(length, resp_type, major, minor)


@dataclass
class MessageWaitForSync(Message):
    """xf86ITEventWaitForSync: header only (8 bytes)."""

    def pack(self) -> bytes:
        return struct.pack("=II", 8, XF86IT_EVENT_WAIT_FOR_SYNC)


@dataclass
class ResponseSyncFinished(Response):
    """xf86ITResponseSyncFinished: header only (8 bytes)."""

    SIZE = 8

    length: int
    resp_type: int

    @classmethod
    def unpack(cls, data: bytes) -> Self:
        length, resp_type = struct.unpack("=II", data)
        return cls(length, resp_type)


@dataclass
class MessageGestureSwipe(Message):
    """xf86ITEventGestureSwipe: 48 bytes total.

    header(8) + gesture_type(2) + num_touches(2) + flags(4) +
    delta_x(8) + delta_y(8) + delta_unaccel_x(8) + delta_unaccel_y(8)
    """

    gesture_type: int
    num_touches: int
    flags: int
    delta_x: float
    delta_y: float
    delta_unaccel_x: float
    delta_unaccel_y: float

    def pack(self) -> bytes:
        return struct.pack(
            "=II HH I dddd",
            48,  # length
            XF86IT_EVENT_GESTURE_SWIPE,
            self.gesture_type,
            self.num_touches,
            self.flags,
            self.delta_x,
            self.delta_y,
            self.delta_unaccel_x,
            self.delta_unaccel_y,
        )


class InputTestError(Exception):
    """Error communicating with the inputtest driver."""


@dataclass
class InputTestConnection:
    """Connection to an xf86-input-inputtest driver instance.

    Wraps the Unix socket protocol defined in
    xf86-input-inputtest-protocol.h.  Connect to the socket path
    configured in the Xorg config's ``SocketPath`` option.

    Usage::

        from inputtest import InputTestConnection
        from proto.xi import XI_GestureSwipeBegin, XI_GestureSwipeEnd

        conn = InputTestConnection("/tmp/inputtest.sock")
        conn.send_gesture_swipe(XI_GestureSwipeBegin, num_touches=3)
        conn.sync()
        conn.send_gesture_swipe(XI_GestureSwipeEnd, num_touches=3)
        conn.sync()
        conn.close()
    """

    _sock: socket.socket

    @classmethod
    def connect(cls, socket_path: Path, timeout_secs: float):
        """Connect with retries."""
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.settimeout(timeout_secs)
        deadline = time.monotonic() + timeout_secs
        last_err = None
        while time.monotonic() < deadline:
            try:
                sock.connect(str(socket_path))

                conn = cls(_sock=sock)
                conn._handshake()
                return conn
            except (ConnectionRefusedError, FileNotFoundError) as e:
                last_err = e
                time.sleep(0.1)
        raise InputTestError(
            f"Cannot connect to inputtest socket {socket_path}: {last_err}"
        )

    def _handshake(self):
        """Send client version and receive server version."""
        self.send(
            MessageClientVersion(
                XF86IT_PROTOCOL_VERSION_MAJOR,
                XF86IT_PROTOCOL_VERSION_MINOR,
            )
        )

        resp = self.recv(ResponseServerVersion)
        if resp.resp_type != XF86IT_RESPONSE_SERVER_VERSION:
            raise InputTestError(
                f"Expected ServerVersion response (type 0), got type {resp.resp_type}"
            )

    def sync(self):
        """Send WaitForSync and block until SyncFinished response."""
        self.send(MessageWaitForSync())

        resp = self.recv(ResponseSyncFinished)
        if resp.resp_type != XF86IT_RESPONSE_SYNC_FINISHED:
            raise InputTestError(
                f"Expected SyncFinished response (type 1), got type {resp.resp_type}"
            )

    def send_gesture_swipe(
        self,
        gesture_type: int,
        num_touches: int = 3,
        flags: int = 0,
        delta_x: float = 1.0,
        delta_y: float = 1.0,
        delta_unaccel_x: float = 1.0,
        delta_unaccel_y: float = 1.0,
    ):
        """Send a gesture swipe event.

        Args:
            gesture_type: One of ``XI_GestureSwipeBegin`` (30),
                ``XI_GestureSwipeUpdate`` (31), ``XI_GestureSwipeEnd`` (32)
                from ``proto.xi``.
            num_touches: Number of touches in the gesture (typically 3 or 4).
            flags: Event flags (0 for normal, 1 for cancelled on End).
            delta_x, delta_y: Accelerated relative motion deltas.
            delta_unaccel_x, delta_unaccel_y: Unaccelerated deltas.
        """
        assert gesture_type in (
            XI_GestureSwipeBegin,
            XI_GestureSwipeUpdate,
            XI_GestureSwipeEnd,
        ), f"Invalid gesture_type {gesture_type}"
        self.send(
            MessageGestureSwipe(
                gesture_type,
                num_touches,
                flags,
                delta_x,
                delta_y,
                delta_unaccel_x,
                delta_unaccel_y,
            )
        )

    def close(self):
        """Close the connection."""
        if self._sock:
            try:
                self._sock.close()
            except OSError:
                pass
            self._sock = None

    def _recv_exact(self, nbytes: int) -> bytes:
        """Receive exactly nbytes from the socket."""
        assert self._sock is not None
        data = b""
        while len(data) < nbytes:
            chunk = self._sock.recv(nbytes - len(data))
            if not chunk:
                raise InputTestError(
                    f"Connection closed ({len(data)}/{nbytes} bytes received)"
                )
            data += chunk
        return data

    def send(self, message: Message):
        """Send a message to the inputtest driver."""
        assert self._sock is not None
        self._sock.sendall(message.pack())

    def recv(self, response_class: type[Response]):
        """Receive a response from the inputtest driver."""
        data = self._recv_exact(response_class.SIZE)
        return response_class.unpack(data)

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
        return False

    def __del__(self):
        self.close()
