# SPDX-License-Identifier: MIT
#
# Render extension protocol request builders

import struct
from dataclasses import dataclass, field

# Render minor opcodes
RenderQueryVersion = 0
RenderQueryPictFormats = 1
RenderCreatePicture = 4
RenderCreateGlyphSet = 17
RenderCompositeGlyphs8 = 23
RenderCompositeGlyphs16 = 24
RenderCompositeGlyphs32 = 25
RenderSetPictureFilter = 30


@dataclass
class QueryVersionRequest:
    """RenderQueryVersion request."""

    opcode: int
    major: int = 0
    minor: int = 11

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHII",
            self.opcode,
            RenderQueryVersion,
            3,
            self.major,
            self.minor,
        )


@dataclass
class QueryPictFormatsRequest:
    """RenderQueryPictFormats request."""

    opcode: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            RenderQueryPictFormats,
            1,
        )


@dataclass
class CreatePictureRequest:
    """RenderCreatePicture request."""

    opcode: int
    picture_id: int
    drawable: int
    format_id: int
    value_mask: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHIIII",
            self.opcode,
            RenderCreatePicture,
            5,
            self.picture_id,
            self.drawable,
            self.format_id,
            self.value_mask,
        )


@dataclass
class CreateGlyphSetRequest:
    """RenderCreateGlyphSet request."""

    opcode: int
    glyph_set_id: int
    format_id: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHII",
            self.opcode,
            RenderCreateGlyphSet,
            3,
            self.glyph_set_id,
            self.format_id,
        )


@dataclass
class _CompositeGlyphsRequestBase:
    """Base class for RenderCompositeGlyphs{8,16,32} requests.

    glyph_elts is raw xGlyphElt + glyph ID data.
    Subclasses set _minor_opcode for the glyph size (8/16/32 bit IDs).
    """

    _minor_opcode: int = field(default=0, init=False, repr=False)

    opcode: int = 0
    src_picture: int = 0
    dst_picture: int = 0
    glyph_set: int = 0
    mask_format: int = 0
    op: int = 3  # PictOpOver
    src_x: int = 0
    src_y: int = 0
    glyph_elts: bytes = b""

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total = 28 + len(self.glyph_elts)
        pad_len = (4 - total % 4) % 4
        length = (total + pad_len) // 4

        header = struct.pack(
            f"{byte_order}BBH BxH IIII hh",
            self.opcode,
            self._minor_opcode,
            length,
            self.op,
            0,  # pad2
            self.src_picture,
            self.dst_picture,
            self.mask_format,
            self.glyph_set,
            self.src_x,
            self.src_y,
        )
        return header + self.glyph_elts + b"\x00" * pad_len


@dataclass
class CompositeGlyphs8Request(_CompositeGlyphsRequestBase):
    """RenderCompositeGlyphs8 request (minor opcode 23)."""

    _minor_opcode: int = field(default=RenderCompositeGlyphs8, init=False, repr=False)


@dataclass
class CompositeGlyphs16Request(_CompositeGlyphsRequestBase):
    """RenderCompositeGlyphs16 request (minor opcode 24)."""

    _minor_opcode: int = field(default=RenderCompositeGlyphs16, init=False, repr=False)


@dataclass
class CompositeGlyphs32Request(_CompositeGlyphsRequestBase):
    """RenderCompositeGlyphs32 request (minor opcode 25)."""

    _minor_opcode: int = field(default=RenderCompositeGlyphs32, init=False, repr=False)


@dataclass
class SetPictureFilterRequest:
    """RenderSetPictureFilter request.

    The filter name is a variable-length string followed by padding,
    then xFixed (CARD32) filter parameter values.
    """

    opcode: int
    picture: int
    filter_name: str = "nearest"
    params: list[int] | None = None  # xFixed values (CARD32)

    def to_bytes(self, byte_order: str = "<") -> bytes:
        name_bytes = self.filter_name.encode("ascii")
        name_len = len(name_bytes)
        name_padded = name_bytes + b"\x00" * ((4 - name_len % 4) % 4)

        param_data = b""
        if self.params:
            for p in self.params:
                param_data += struct.pack(f"{byte_order}I", p)

        total = 12 + len(name_padded) + len(param_data)
        pad_len = (4 - total % 4) % 4
        length = (total + pad_len) // 4

        header = struct.pack(
            f"{byte_order}BBH I H xx",
            self.opcode,
            RenderSetPictureFilter,
            length,
            self.picture,
            name_len,
        )
        return header + name_padded + param_data + b"\x00" * pad_len
