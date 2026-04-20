# SPDX-License-Identifier: MIT
#
# RECORD extension protocol request builders

import struct
from dataclasses import dataclass, field

# RECORD minor opcodes
RecordQueryVersion = 0
RecordCreateContext = 1
RecordRegisterClients = 2

RECORD_MAJOR = 1
RECORD_MINOR = 13


@dataclass
class QueryVersionRequest:
    """RecordQueryVersion request."""

    opcode: int
    major: int = RECORD_MAJOR
    minor: int = RECORD_MINOR

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHHH",
            self.opcode,
            RecordQueryVersion,
            2,
            self.major,
            self.minor,
        )


@dataclass
class CreateContextRequest:
    """RecordCreateContext request.

    Header is 20 bytes, followed by nClients CARD32 client IDs,
    then nRanges xRecordRange structs (32 bytes each).
    """

    opcode: int
    context_id: int
    element_header: int = 0
    client_ids: list[int] = field(default_factory=list)
    ranges_data: bytes = b""
    n_clients_override: int | None = None
    n_ranges_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        n_clients = (
            self.n_clients_override
            if self.n_clients_override is not None
            else len(self.client_ids)
        )
        n_ranges = (
            self.n_ranges_override
            if self.n_ranges_override is not None
            else len(self.ranges_data) // 32
        )

        client_data = b""
        for cid in self.client_ids:
            client_data += struct.pack(f"{byte_order}I", cid)

        total_bytes = 20 + len(client_data) + len(self.ranges_data)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        header = struct.pack(
            f"{byte_order}BBH I B xxx I I",
            self.opcode,
            RecordCreateContext,
            total_bytes // 4,
            self.context_id,
            self.element_header,
            n_clients,
            n_ranges,
        )
        return header + client_data + self.ranges_data + b"\x00" * pad_len
