# SPDX-License-Identifier: MIT
#
# GLX extension protocol request builders for security testing.

import struct
from dataclasses import dataclass

# GLX minor opcodes
GLXRender = 1
GLXRenderLarge = 2
GLXCreateContext = 3
GLXDestroyContext = 4
GLXMakeCurrent = 5
GLXQueryVersion = 7
GLXMakeContextCurrent = 26
GLXChangeDrawableAttributes = 30

# GLX drawable attribute keys
GLX_EVENT_MASK = 0x801F


@dataclass
class QueryVersionRequest:
    """GLX QueryVersion request (12 bytes).

    Wire format:
        CARD8    reqType         (GLX major opcode)
        CARD8    glxCode         (7)
        CARD16   length          (3)
        CARD32   majorVersion
        CARD32   minorVersion
    """

    opcode: int
    major_version: int = 1
    minor_version: int = 4
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        length = self.length_override if self.length_override is not None else 3
        return struct.pack(
            f"{byte_order}BBH II",
            self.opcode,
            GLXQueryVersion,
            length,
            self.major_version,
            self.minor_version,
        )


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
class DestroyContextRequest:
    """GLX DestroyContext request (8 bytes).

    Wire format:
        CARD8    reqType         (GLX major opcode)
        CARD8    glxCode         (4)
        CARD16   length          (2)
        CARD32   context
    """

    opcode: int
    context: int
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        length = self.length_override if self.length_override is not None else 2
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            GLXDestroyContext,
            length,
            self.context,
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
class MakeContextCurrentRequest:
    """GLX MakeContextCurrent request (20 bytes).

    Wire format:
        CARD8    reqType         (GLX major opcode)
        CARD8    glxCode         (26)
        CARD16   length          (5)
        CARD32   oldContextTag
        CARD32   drawable
        CARD32   readdrawable
        CARD32   context
    """

    opcode: int
    old_context_tag: int
    drawable: int
    readdrawable: int
    context: int
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        length = self.length_override if self.length_override is not None else 5
        return struct.pack(
            f"{byte_order}BBH IIII",
            self.opcode,
            GLXMakeContextCurrent,
            length,
            self.old_context_tag,
            self.drawable,
            self.readdrawable,
            self.context,
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


@dataclass
class RenderLargeRequest:
    """GLX RenderLarge request (X_GLXRenderLarge, minor opcode 2).

    Wire format (xGLXRenderLargeReq):
        CARD8    reqType         (GLX major opcode)
        CARD8    glxCode         (2)
        CARD16   length          (request length in 4-byte words)
        CARD32   contextTag      (from MakeCurrent reply)
        CARD16   requestNumber   (1-based sequence within the large command)
        CARD16   requestTotal    (total number of sub-requests)
        CARD32   dataBytes       (number of data bytes in this sub-request)
        <data>                   (padded to 4 bytes)

    For requestNumber=1, the data begins with a GL rendering command
    header: opcode(2) + cmdlen(2) + data...
    """

    opcode: int
    context_tag: int
    request_number: int
    request_total: int
    data_bytes: int
    data: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        pad_len = (4 - len(self.data) % 4) % 4 if self.data else 0
        total_bytes = 16 + len(self.data) + pad_len
        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH I HH I",
            self.opcode,
            GLXRenderLarge,
            length,
            self.context_tag,
            self.request_number,
            self.request_total,
            self.data_bytes,
        )
        return header + self.data + b"\x00" * pad_len
