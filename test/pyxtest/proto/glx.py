# SPDX-License-Identifier: MIT
#
# GLX extension protocol request builders for security testing.

import struct
from dataclasses import dataclass

# GLX minor opcodes
GLXCreateContext = 3
GLXMakeCurrent = 5
GLXChangeDrawableAttributes = 30

# GLX drawable attribute keys
GLX_EVENT_MASK = 0x801F


@dataclass
class CreateContextRequest:
    """glxCreateContext request (24 bytes = 6 words)."""

    opcode: int
    context_id: int
    visual: int
    screen: int = 0
    share_list: int = 0
    is_direct: int = 1

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH IIII B xxx",
            self.opcode,
            GLXCreateContext,
            6,  # length = 6 words
            self.context_id,
            self.visual,
            self.screen,
            self.share_list,
            self.is_direct,
        )


@dataclass
class MakeCurrentRequest:
    """glxMakeCurrent request (16 bytes = 4 words)."""

    opcode: int
    drawable: int
    context_id: int
    old_context_tag: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH III",
            self.opcode,
            GLXMakeCurrent,
            4,  # length = 4 words
            self.drawable,
            self.context_id,
            self.old_context_tag,
        )


@dataclass
class ChangeDrawableAttributesRequest:
    """glxChangeDrawableAttributes request.

    Header is 12 bytes (3 words): opcode, glxCode, length, drawable, numAttribs.
    Followed by numAttribs * 2 CARD32 values (key/value pairs).

    The length_override and num_attribs_override fields allow crafting
    intentionally malformed requests for security testing.
    """

    opcode: int
    drawable: int
    num_attribs: int = 0
    attribs: bytes = b""
    length_override: int | None = None
    num_attribs_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 12 + len(self.attribs)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        num_attribs = (
            self.num_attribs_override
            if self.num_attribs_override is not None
            else self.num_attribs
        )

        header = struct.pack(
            f"{byte_order}BBH II",
            self.opcode,
            GLXChangeDrawableAttributes,
            length,
            self.drawable,
            num_attribs,
        )
        return header + self.attribs + b"\x00" * pad_len
