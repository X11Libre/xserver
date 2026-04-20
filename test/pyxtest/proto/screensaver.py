# SPDX-License-Identifier: MIT
#
# Screen Saver extension protocol request builders

import struct
from dataclasses import dataclass

# ScreenSaver minor opcodes
ScreenSaverQueryVersion = 0
ScreenSaverSelectInput = 1
ScreenSaverSetAttributes = 3
ScreenSaverUnsetAttributes = 4
ScreenSaverSuspend = 5


@dataclass
class SuspendRequest:
    """ScreenSaverSuspend request (8 bytes)."""

    opcode: int
    suspend: int = 1
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        length = self.length_override if self.length_override is not None else 2
        return struct.pack(
            f"{byte_order}BBHI",
            self.opcode,
            ScreenSaverSuspend,
            length,
            self.suspend,
        )


@dataclass
class SetAttributesRequest:
    """ScreenSaverSetAttributes request (28 bytes = 7 words, mask=0).

    xScreenSaverSetAttributesReq:
      reqType(1) + saverReqType(1) + length(2)
      drawable(4)
      x(2) + y(2)
      width(2) + height(2)
      borderWidth(2) + c_class(1) + depth(1)
      visualID(4)
      mask(4)
      [values...]
    """

    opcode: int
    drawable: int
    x: int = 0
    y: int = 0
    width: int = 100
    height: int = 100
    border_width: int = 0
    c_class: int = 0  # CopyFromParent
    depth: int = 0  # CopyFromParent
    visual_id: int = 0  # CopyFromParent
    mask: int = 0
    values: bytes = b""

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 28 + len(self.values)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len
        length = total_bytes // 4

        header = struct.pack(
            f"{byte_order}BBHIhhHHHBBII",
            self.opcode,
            ScreenSaverSetAttributes,
            length,
            self.drawable,
            self.x,
            self.y,
            self.width,
            self.height,
            self.border_width,
            self.c_class,
            self.depth,
            self.visual_id,
            self.mask,
        )
        return header + self.values + b"\x00" * pad_len


@dataclass
class UnsetAttributesRequest:
    """ScreenSaverUnsetAttributes request (8 bytes = 2 words)."""

    opcode: int
    drawable: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            ScreenSaverUnsetAttributes,
            2,  # length = 2 words
            self.drawable,
        )
