# SPDX-License-Identifier: MIT
#
# X-Resource extension protocol request builders for byteswap testing.

import struct
from dataclasses import dataclass, field

# XRes minor opcodes
XResQueryVersion = 0
XResQueryClients = 1
XResQueryClientResources = 2
XResQueryClientPixmapBytes = 3
XResQueryClientIds = 4
XResQueryResourceBytes = 5


@dataclass
class QueryVersionRequest:
    """XResQueryVersion request."""

    opcode: int
    major: int = 1
    minor: int = 2

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHBB xx",
            self.opcode,
            XResQueryVersion,
            2,
            self.major,
            self.minor,
        )


@dataclass
class QueryClientsRequest:
    """XResQueryClients request."""

    opcode: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            XResQueryClients,
            1,
        )


@dataclass
class QueryClientIdsRequest:
    """XResQueryClientIds request.

    Followed by numSpecs xXResClientIdSpec entries (8 bytes each:
    client CARD32 + mask CARD32).
    """

    opcode: int
    specs: list[tuple[int, int]] = field(default_factory=list)  # (client, mask) pairs
    num_specs_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        num_specs = (
            self.num_specs_override
            if self.num_specs_override is not None
            else len(self.specs)
        )

        spec_data = b""
        for client, mask in self.specs:
            spec_data += struct.pack(f"{byte_order}II", client, mask)

        total = 8 + len(spec_data)
        pad_len = (4 - total % 4) % 4
        length = (total + pad_len) // 4

        header = struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            XResQueryClientIds,
            length,
            num_specs,
        )
        return header + spec_data + b"\x00" * pad_len
