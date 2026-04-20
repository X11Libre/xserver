# SPDX-License-Identifier: MIT
#
# Screen Saver extension protocol request builders

import struct
from dataclasses import dataclass

# ScreenSaver minor opcodes
ScreenSaverQueryVersion = 0
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
