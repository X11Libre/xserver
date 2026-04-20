# SPDX-License-Identifier: MIT
#
# Xinerama (PanoramiX / pseudoramiX) extension protocol request builders
# for byteswap testing.

import struct
from dataclasses import dataclass

# Xinerama minor opcodes
PanoramiXQueryVersion = 0
PanoramiXGetState = 1
PanoramiXGetScreenCount = 2
PanoramiXGetScreenSize = 3
XineramaIsActive = 4
XineramaQueryScreens = 5


@dataclass
class QueryVersionRequest:
    """PanoramiXQueryVersion request."""

    opcode: int
    major: int = 1
    minor: int = 1

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHBB xx",
            self.opcode,
            PanoramiXQueryVersion,
            2,
            self.major,
            self.minor,
        )


@dataclass
class GetStateRequest:
    """PanoramiXGetState request."""

    opcode: int
    window: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            PanoramiXGetState,
            2,
            self.window,
        )


@dataclass
class GetScreenCountRequest:
    """PanoramiXGetScreenCount request."""

    opcode: int
    window: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            PanoramiXGetScreenCount,
            2,
            self.window,
        )


@dataclass
class GetScreenSizeRequest:
    """PanoramiXGetScreenSize request."""

    opcode: int
    window: int
    screen: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH II",
            self.opcode,
            PanoramiXGetScreenSize,
            3,
            self.window,
            self.screen,
        )


@dataclass
class IsActiveRequest:
    """XineramaIsActive request."""

    opcode: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            XineramaIsActive,
            1,
        )
