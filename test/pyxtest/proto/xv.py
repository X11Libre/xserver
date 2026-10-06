# SPDX-License-Identifier: MIT
#
# XVideo extension protocol request builders.

import struct
from dataclasses import dataclass

# Xv minor opcodes (Xvproto.h)
XvQueryExtension = 0
XvQueryAdaptors = 1
XvListImageFormats = 16
XvQueryImageAttributes = 17
XvPutImage = 18
XvShmPutImage = 19

# Adaptor type bits (Xv.h)
XvInputMask = 1 << 0
XvImageMask = 0x00000010

# FourCCs used by glamor Xv
FOURCC_YV12 = 0x32315659
FOURCC_NV12 = 0x3231564E

# Dimensions that overflow int32 NV12 / YV12 size math in glamor
OVERFLOW_WIDTH = 43690
OVERFLOW_HEIGHT = 65534


@dataclass
class QueryExtensionRequest:
    """XvQueryExtension request."""

    opcode: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            XvQueryExtension,
            1,
        )


@dataclass
class QueryAdaptorsRequest:
    """XvQueryAdaptors request."""

    opcode: int
    window: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            XvQueryAdaptors,
            2,
            self.window,
        )


@dataclass
class ListImageFormatsRequest:
    """XvListImageFormats request."""

    opcode: int
    port: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            XvListImageFormats,
            2,
            self.port,
        )


@dataclass
class QueryImageAttributesRequest:
    """XvQueryImageAttributes request."""

    opcode: int
    port: int
    id: int
    width: int
    height: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH II HH",
            self.opcode,
            XvQueryImageAttributes,
            4,
            self.port,
            self.id,
            self.width,
            self.height,
        )


@dataclass
class PutImageRequest:
    """XvPutImage request.

    Wire size is 40 bytes (10 words) plus optional image data.
    ``length_override`` crafts a short request for overflow tests.
    """

    opcode: int
    port: int
    drawable: int
    gc: int
    id: int
    src_x: int = 0
    src_y: int = 0
    src_w: int = 1
    src_h: int = 1
    drw_x: int = 0
    drw_y: int = 0
    drw_w: int = 1
    drw_h: int = 1
    width: int = 1
    height: int = 1
    data: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        header = struct.pack(
            f"{byte_order}BBH IIII hhHHhhHHHH",
            self.opcode,
            XvPutImage,
            0,  # filled below
            self.port,
            self.drawable,
            self.gc,
            self.id,
            self.src_x,
            self.src_y,
            self.src_w,
            self.src_h,
            self.drw_x,
            self.drw_y,
            self.drw_w,
            self.drw_h,
            self.width,
            self.height,
        )
        assert len(header) == 40
        payload = header + self.data
        pad = (4 - (len(payload) % 4)) % 4
        payload += b"\x00" * pad
        length = (
            self.length_override
            if self.length_override is not None
            else len(payload) // 4
        )
        return (
            struct.pack(f"{byte_order}BBH", self.opcode, XvPutImage, length)
            + payload[4:]
        )


@dataclass
class ShmPutImageRequest:
    """XvShmPutImage request (52 bytes / 13 words)."""

    opcode: int
    port: int
    drawable: int
    gc: int
    shmseg: int
    id: int
    offset: int = 0
    src_x: int = 0
    src_y: int = 0
    src_w: int = 1
    src_h: int = 1
    drw_x: int = 0
    drw_y: int = 0
    drw_w: int = 1
    drw_h: int = 1
    width: int = 1
    height: int = 1
    send_event: bool = False

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH IIIIII hhHHhhHHHH BBxx",
            self.opcode,
            XvShmPutImage,
            13,
            self.port,
            self.drawable,
            self.gc,
            self.shmseg,
            self.id,
            self.offset,
            self.src_x,
            self.src_y,
            self.src_w,
            self.src_h,
            self.drw_x,
            self.drw_y,
            self.drw_w,
            self.drw_h,
            self.width,
            self.height,
            1 if self.send_event else 0,
            0,
        )
