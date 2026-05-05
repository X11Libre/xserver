# SPDX-License-Identifier: MIT
#
# Present extension protocol request builders for byteswap testing.

import struct
from dataclasses import dataclass

# Present minor opcodes
PresentQueryVersion = 0
PresentPixmap = 1
PresentNotifyMSC = 2
PresentSelectInput = 3
PresentQueryCapabilities = 4


@dataclass
class QueryVersionRequest:
    """PresentQueryVersion request."""

    opcode: int
    major: int = 1
    minor: int = 2

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHII",
            self.opcode,
            PresentQueryVersion,
            3,
            self.major,
            self.minor,
        )


@dataclass
class SelectInputRequest:
    """PresentSelectInput request."""

    opcode: int
    eid: int
    window: int
    event_mask: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH III",
            self.opcode,
            PresentSelectInput,
            4,
            self.eid,
            self.window,
            self.event_mask,
        )


@dataclass
class QueryCapabilitiesRequest:
    """PresentQueryCapabilities request."""

    opcode: int
    target: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            PresentQueryCapabilities,
            2,
            self.target,
        )


@dataclass
class PresentNotify:
    """xPresentNotify structure (8 bytes): window(4) + serial(4)."""

    window: int
    serial: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(f"{byte_order}II", self.window, self.serial)


@dataclass
class PixmapRequest:
    """PresentPixmap request.

    This is a large request with many fields. All fields are included
    to test that byte-swapping handles them all correctly.
    """

    opcode: int
    window: int
    pixmap: int
    serial: int = 0
    valid: int = 0  # region or None
    update: int = 0  # region or None
    x_off: int = 0
    y_off: int = 0
    target_crtc: int = 0
    wait_fence: int = 0
    idle_fence: int = 0
    options: int = 0
    target_msc: int = 0
    divisor: int = 0
    remainder: int = 0
    notifies: list[PresentNotify] | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        notify_data = b""
        if self.notifies:
            for n in self.notifies:
                notify_data += n.to_bytes(byte_order)

        base_size = 72  # 18 words
        total_size = base_size + len(notify_data)
        length = total_size // 4

        header = struct.pack(
            f"{byte_order}BBH"  # header: opcode, sub-opcode, length
            f"III"  # window, pixmap, serial
            f"II"  # valid, update
            f"hh"  # x_off, y_off
            f"III"  # target_crtc, wait_fence, idle_fence
            f"I"  # options
            f"xxxx"  # pad
            f"Q"  # target_msc (CARD64)
            f"Q"  # divisor (CARD64)
            f"Q",  # remainder (CARD64)
            self.opcode,
            PresentPixmap,
            length,
            self.window,
            self.pixmap,
            self.serial,
            self.valid,
            self.update,
            self.x_off,
            self.y_off,
            self.target_crtc,
            self.wait_fence,
            self.idle_fence,
            self.options,
            self.target_msc,
            self.divisor,
            self.remainder,
        )
        return header + notify_data


@dataclass
class NotifyMSCRequest:
    """PresentNotifyMSC request."""

    opcode: int
    window: int
    serial: int = 0
    target_msc: int = 0
    divisor: int = 0
    remainder: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH"
            f"II"  # window, serial
            f"xxxx"  # pad
            f"Q"  # target_msc
            f"Q"  # divisor
            f"Q",  # remainder
            self.opcode,
            PresentNotifyMSC,
            10,  # 40 bytes = 10 words
            self.window,
            self.serial,
            self.target_msc,
            self.divisor,
            self.remainder,
        )
