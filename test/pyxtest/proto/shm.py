# SPDX-License-Identifier: MIT
#
# MIT-SHM extension protocol request builders for byteswap testing.

import struct
from dataclasses import dataclass

# SHM minor opcodes
ShmQueryVersion = 0
ShmAttach = 1
ShmDetach = 2
ShmPutImage = 3
ShmGetImage = 4
ShmCreatePixmap = 5
ShmAttachFd = 6
ShmCreateSegment = 7


@dataclass
class QueryVersionRequest:
    """ShmQueryVersion request."""

    opcode: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            ShmQueryVersion,
            1,
        )


@dataclass
class AttachRequest:
    """ShmAttach request (SysV shared memory)."""

    opcode: int
    shmseg: int
    shmid: int
    read_only: bool = False

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH II Bxxx",
            self.opcode,
            ShmAttach,
            4,
            self.shmseg,
            self.shmid,
            1 if self.read_only else 0,
        )


@dataclass
class DetachRequest:
    """ShmDetach request."""

    opcode: int
    shmseg: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            ShmDetach,
            2,
            self.shmseg,
        )


@dataclass
class CreateSegmentRequest:
    """ShmCreateSegment request.

    Creates a shared memory segment owned by the server.
    Returns a file descriptor and whether the segment is read-only.
    """

    opcode: int
    shmseg: int
    size: int
    read_only: bool = False

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH II Bxxx",
            self.opcode,
            ShmCreateSegment,
            4,  # 16 bytes = 4 words
            self.shmseg,
            self.size,
            1 if self.read_only else 0,
        )
