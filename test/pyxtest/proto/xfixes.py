# SPDX-License-Identifier: MIT
#
# XFIXES extension protocol request builders for security testing.

import struct
from dataclasses import dataclass

# XFixes minor opcodes
XFixesQueryVersion = 0
XFixesCreatePointerBarrier = 31
XFixesDestroyPointerBarrier = 32


@dataclass
class XFixesQueryVersionRequest:
    """XFixesQueryVersion request (12 bytes).

    Wire format:
        CARD8    reqType         (XFIXES major opcode)
        CARD8    xfixesReqType   (0)
        CARD16   length          (3)
        CARD32   majorVersion
        CARD32   minorVersion
    """

    opcode: int
    major_version: int = 5
    minor_version: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH II",
            self.opcode,
            XFixesQueryVersion,
            3,  # 12 bytes = 3 words
            self.major_version,
            self.minor_version,
        )


@dataclass
class XFixesCreatePointerBarrierRequest:
    """XFixesCreatePointerBarrier request.

    Wire format (xXFixesCreatePointerBarrierReq):
        CARD8    reqType         (XFIXES major opcode)
        CARD8    xfixesReqType   (31)
        CARD16   length
        CARD32   barrier         (XID)
        CARD32   window
        INT16    x1
        INT16    y1
        INT16    x2
        INT16    y2
        CARD32   directions
        CARD16   pad
        CARD16   num_devices
        <devices>                (num_devices * CARD16, padded)
    """

    opcode: int
    barrier: int
    window: int
    x1: int
    y1: int
    x2: int
    y2: int
    directions: int = 0
    num_devices: int = 0
    devices: list[int] | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        devs = self.devices or []
        dev_data = b""
        for d in devs:
            dev_data += struct.pack(f"{byte_order}H", d)
        # Pad device list to 4-byte boundary
        pad_len = (4 - len(dev_data) % 4) % 4 if dev_data else 0
        dev_data += b"\x00" * pad_len

        total = 28 + len(dev_data)
        length = total // 4

        header = struct.pack(
            f"{byte_order}BBH II hh hh I HH",
            self.opcode,
            XFixesCreatePointerBarrier,
            length,
            self.barrier,
            self.window,
            self.x1,
            self.y1,
            self.x2,
            self.y2,
            self.directions,
            0,  # pad
            self.num_devices,
        )
        return header + dev_data
