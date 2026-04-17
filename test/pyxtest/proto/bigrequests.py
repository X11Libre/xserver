# SPDX-License-Identifier: MIT
#
# BIG-REQUESTS extension protocol request builders

import struct
from dataclasses import dataclass

# BIG-REQUESTS minor opcodes
BigRequestsEnable = 0


@dataclass
class BigRequestsEnableRequest:
    """BIG-REQUESTS Enable request."""

    opcode: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            BigRequestsEnable,  # sub-opcode
            1,  # request length
        )
