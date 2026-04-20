# SPDX-License-Identifier: MIT
#
# XFree86-VidModeExtension protocol request builders for byteswap testing.

import struct
from dataclasses import dataclass

# VidMode minor opcodes
VidModeQueryVersion = 0
VidModeGetModeLine = 1
VidModeModModeLine = 2
VidModeSwitchMode = 3
VidModeGetMonitor = 4
VidModeLockModeSwitch = 5
VidModeGetAllModeLines = 6
VidModeAddModeLine = 7
VidModeDeleteModeLine = 8
VidModeValidateModeLine = 9
VidModeSwitchToMode = 10
VidModeGetViewPort = 11
VidModeSetViewPort = 12


@dataclass
class QueryVersionRequest:
    """VidModeQueryVersion request."""

    opcode: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            VidModeQueryVersion,
            1,
        )


@dataclass
class GetModeLineRequest:
    """VidModeGetModeLine request."""

    opcode: int
    screen: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH H xx",
            self.opcode,
            VidModeGetModeLine,
            2,
            self.screen,
        )


@dataclass
class SwitchModeRequest:
    """VidModeSwitchMode request."""

    opcode: int
    screen: int = 0
    zoom: int = 1

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH HH",
            self.opcode,
            VidModeSwitchMode,
            2,
            self.screen,
            self.zoom,
        )


@dataclass
class SwitchToModeRequest:
    """VidModeSwitchToMode request (v2 format, 52 bytes).

    xXF86VidModeSwitchToModeReq:
      reqType(1) + xf86vidmodeReqType(1) + length(2) +
      screen(4) + dotclock(4) +
      hdisplay(2) + hsyncstart(2) + hsyncend(2) + htotal(2) + hskew(2) +
      vdisplay(2) + vsyncstart(2) + vsyncend(2) + vtotal(2) + pad1(2) +
      flags(4) + reserved1(4) + reserved2(4) + reserved3(4) + privsize(4)
    """

    opcode: int
    screen: int = 0
    dotclock: int = 0
    hdisplay: int = 0
    hsyncstart: int = 0
    hsyncend: int = 0
    htotal: int = 0
    hskew: int = 0
    vdisplay: int = 0
    vsyncstart: int = 0
    vsyncend: int = 0
    vtotal: int = 0
    flags: int = 0
    privsize: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH"  # header (4)
            f"I"  # screen (4)
            f"I"  # dotclock (4)
            f"HHHHH"  # hdisplay, hsyncstart, hsyncend, htotal, hskew (10)
            f"HHHH"  # vdisplay, vsyncstart, vsyncend, vtotal (8)
            f"xx"  # pad1 (2)
            f"I"  # flags (4)
            f"I"  # reserved1 (4)
            f"I"  # reserved2 (4)
            f"I"  # reserved3 (4)
            f"I",  # privsize (4)
            self.opcode,
            VidModeSwitchToMode,
            13,  # 52 bytes = 13 words
            self.screen,
            self.dotclock,
            self.hdisplay,
            self.hsyncstart,
            self.hsyncend,
            self.htotal,
            self.hskew,
            self.vdisplay,
            self.vsyncstart,
            self.vsyncend,
            self.vtotal,
            self.flags,
            0,  # reserved1
            0,  # reserved2
            0,  # reserved3
            self.privsize,
        )


@dataclass
class GetAllModeLinesRequest:
    """VidModeGetAllModeLines request."""

    opcode: int
    screen: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH H xx",
            self.opcode,
            VidModeGetAllModeLines,
            2,
            self.screen,
        )


@dataclass
class ValidateModeLineRequest:
    """VidModeValidateModeLine request (v2 format, 52 bytes).

    Same layout as SwitchToModeRequest.
    """

    opcode: int
    screen: int = 0
    dotclock: int = 0
    hdisplay: int = 0
    hsyncstart: int = 0
    hsyncend: int = 0
    htotal: int = 0
    hskew: int = 0
    vdisplay: int = 0
    vsyncstart: int = 0
    vsyncend: int = 0
    vtotal: int = 0
    flags: int = 0
    privsize: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHIIHHHHHHHHHxxIIIII",
            self.opcode,
            VidModeValidateModeLine,
            13,  # 52 bytes = 13 words
            self.screen,
            self.dotclock,
            self.hdisplay,
            self.hsyncstart,
            self.hsyncend,
            self.htotal,
            self.hskew,
            self.vdisplay,
            self.vsyncstart,
            self.vsyncend,
            self.vtotal,
            self.flags,
            0,  # reserved1
            0,  # reserved2
            0,  # reserved3
            self.privsize,
        )
