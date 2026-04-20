# SPDX-License-Identifier: MIT
#
# RandR extension protocol request builders

import struct
from dataclasses import dataclass

# RandR minor opcodes
RRQueryVersion = 0
RRGetScreenResources = 8
RRChangeOutputProperty = 13
RRGetOutputProperty = 15
RRGetScreenResourcesCurrent = 25

RR_MAJOR = 1
RR_MINOR = 6

# Property modes (same as core X11)
PropModeReplace = 0
PropModePrepend = 1
PropModeAppend = 2


@dataclass
class QueryVersionRequest:
    """RRQueryVersion request."""

    opcode: int
    major: int = RR_MAJOR
    minor: int = RR_MINOR

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHII",
            self.opcode,
            RRQueryVersion,
            3,
            self.major,
            self.minor,
        )


@dataclass
class GetScreenResourcesCurrentRequest:
    """RRGetScreenResourcesCurrent request."""

    opcode: int
    window: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHI",
            self.opcode,
            RRGetScreenResourcesCurrent,
            2,
            self.window,
        )


@dataclass
class ChangeOutputPropertyRequest:
    """RRChangeOutputProperty request."""

    opcode: int
    output: int
    property_atom: int
    type_atom: int
    format: int = 32
    mode: int = PropModeReplace
    data: bytes = b""
    num_items: int | None = None
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        num_items = (
            self.num_items
            if self.num_items is not None
            else (len(self.data) // (self.format // 8) if self.data else 0)
        )

        total_bytes = 24 + len(self.data)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH III BB H I",
            self.opcode,
            RRChangeOutputProperty,
            length,
            self.output,
            self.property_atom,
            self.type_atom,
            self.format,
            self.mode,
            0,  # pad
            num_items,
        )
        return header + self.data + b"\x00" * pad_len


@dataclass
class GetOutputPropertyRequest:
    """RRGetOutputProperty request."""

    opcode: int
    output: int
    property_atom: int
    type_atom: int = 0  # AnyPropertyType
    offset: int = 0
    length: int = 0xFFFF
    delete: bool = False
    pending: bool = True

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I II II BB H",
            self.opcode,
            RRGetOutputProperty,
            7,  # 28 bytes = 7 words
            self.output,
            self.property_atom,
            self.type_atom,
            self.offset,
            self.length,
            1 if self.delete else 0,
            1 if self.pending else 0,
            0,  # pad
        )
