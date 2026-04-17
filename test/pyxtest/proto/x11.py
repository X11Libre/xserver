# SPDX-License-Identifier: MIT
#
# Core X11 protocol request builders

import struct
from dataclasses import dataclass

# Core protocol opcodes
CreateWindow = 1
CreatePixmap = 53
InternAtom = 16
QueryExtension = 98


def _pad(data: bytes) -> bytes:
    """Pad data to a 4-byte boundary."""
    return data + b"\x00" * ((4 - len(data) % 4) % 4)


@dataclass
class CreateWindowRequest:
    """X11 CreateWindow request."""

    wid: int
    parent: int
    x: int
    y: int
    width: int
    height: int
    depth: int
    border_width: int = 0
    window_class: int = 1  # InputOutput
    visual: int = 0
    value_mask: int = 0x0800  # override-redirect
    override_redirect: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH II hhHHHH II I",
            CreateWindow,  # opcode
            self.depth,
            9,  # request length in 4-byte units
            self.wid,
            self.parent,
            self.x,
            self.y,
            self.width,
            self.height,
            self.border_width,
            self.window_class,
            self.visual,
            self.value_mask,
            self.override_redirect,
        )


@dataclass
class CreatePixmapRequest:
    """X11 CreatePixmap request."""

    pid: int
    drawable: int
    width: int
    height: int
    depth: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHIIHH",
            CreatePixmap,  # opcode
            self.depth,
            4,  # request length
            self.pid,
            self.drawable,
            self.width,
            self.height,
        )


@dataclass
class InternAtomRequest:
    """X11 InternAtom request."""

    name: str
    only_if_exists: bool = False

    def to_bytes(self, byte_order: str = "<") -> bytes:
        name_bytes = self.name.encode("ascii")
        padded = _pad(name_bytes)
        req_len = (8 + len(padded)) // 4
        return (
            struct.pack(
                f"{byte_order}BBHHxx",
                InternAtom,  # opcode
                1 if self.only_if_exists else 0,
                req_len,
                len(name_bytes),
            )
            + padded
        )


@dataclass
class QueryExtensionRequest:
    """X11 QueryExtension request."""

    name: str

    def to_bytes(self, byte_order: str = "<") -> bytes:
        name_bytes = self.name.encode("ascii")
        padded = _pad(name_bytes)
        req_len = (8 + len(padded)) // 4
        return (
            struct.pack(
                f"{byte_order}BBHHxx", QueryExtension, 0, req_len, len(name_bytes)
            )
            + padded
        )
