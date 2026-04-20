# SPDX-License-Identifier: MIT
#
# XI/XI2 (XInput) extension protocol request builders

import struct
from dataclasses import dataclass

# XI2 minor opcodes
XIQueryVersion = 47
XIPassiveGrabDevice = 54
XIPassiveUngrabDevice = 55
XIChangeProperty = 57
XIGetProperty = 59

XI2_MAJOR = 2
XI2_MINOR = 4

# Grab types
XIGrabtypeButton = 0
XIGrabtypeKeycode = 1
XIGrabtypeEnter = 2
XIGrabtypeFocusIn = 3

# Grab modes
XIGrabModeSync = 0
XIGrabModeAsync = 1

# Special values
XIAllDevices = 0
XIAllMasterDevices = 1
XIAnyModifier = 1 << 31

# Grab status codes (returned as X11 error codes when used as
# ProcXIPassiveGrabDevice return values)
XIAlreadyGrabbed = 1

# Property modes
PropModeReplace = 0
PropModePrepend = 1
PropModeAppend = 2

# Virtual core device IDs (always present)
VirtualCorePointer = 2
VirtualCoreKeyboard = 3


@dataclass
class XIQueryVersionRequest:
    """XIQueryVersion request."""

    opcode: int
    major: int = XI2_MAJOR
    minor: int = XI2_MINOR

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHHH",
            self.opcode,
            XIQueryVersion,
            2,  # 8 bytes = 2 words
            self.major,
            self.minor,
        )


@dataclass
class XIPassiveGrabDeviceRequest:
    """XIPassiveGrabDevice request."""

    opcode: int
    grab_window: int
    detail: int
    deviceid: int = XIAllMasterDevices
    grab_type: int = XIGrabtypeButton
    grab_mode: int = XIGrabModeAsync
    paired_device_mode: int = XIGrabModeAsync
    owner_events: bool = False
    mask: bytes = b"\x00" * 4
    modifiers: list[int] | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        mods = self.modifiers if self.modifiers is not None else [XIAnyModifier]
        num_modifiers = len(mods)

        mask_padded = self.mask + b"\x00" * ((4 - len(self.mask) % 4) % 4)
        mask_len = len(mask_padded) // 4

        # Header: 32 bytes, mask: mask_len*4, modifiers: num_modifiers*4
        total = 32 + len(mask_padded) + num_modifiers * 4
        length = total // 4

        header = struct.pack(
            f"{byte_order}BBH IIII HHH BBBB H",
            self.opcode,
            XIPassiveGrabDevice,
            length,
            0,  # time = CurrentTime
            self.grab_window,
            0,  # cursor = None
            self.detail,
            self.deviceid,
            num_modifiers,
            mask_len,
            self.grab_type,
            self.grab_mode,
            self.paired_device_mode,
            1 if self.owner_events else 0,
            0,  # pad
        )

        mod_data = b""
        for mod in mods:
            mod_data += struct.pack(f"{byte_order}I", mod)

        return header + mask_padded + mod_data


@dataclass
class XIPassiveUngrabDeviceRequest:
    """XIPassiveUngrabDevice request."""

    opcode: int
    grab_window: int
    detail: int
    deviceid: int = XIAllMasterDevices
    grab_type: int = XIGrabtypeButton
    modifiers: list[int] | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        mods = self.modifiers if self.modifiers is not None else [XIAnyModifier]
        num_modifiers = len(mods)

        # Header is 20 bytes, followed by num_modifiers * 4 bytes
        length = (20 + num_modifiers * 4) // 4

        header = struct.pack(
            f"{byte_order}BBH II HH Bx H",
            self.opcode,
            XIPassiveUngrabDevice,
            length,
            self.grab_window,
            self.detail,
            self.deviceid,
            num_modifiers,
            self.grab_type,
            # pad0, pad1
            0,
        )

        mod_data = b""
        for mod in mods:
            mod_data += struct.pack(f"{byte_order}I", mod)

        return header + mod_data


@dataclass
class XIChangePropertyRequest:
    """XIChangeProperty request."""

    opcode: int
    deviceid: int
    property_atom: int
    type_atom: int
    format: int = 32
    mode: int = PropModeReplace
    data: bytes = b""
    num_items: int | None = None
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        num_items = (
            self.num_items
            if self.num_items is not None
            else (len(self.data) // (self.format // 8) if self.data else 0)
        )

        total_bytes = 20 + len(self.data)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH HBB II I",
            self.opcode,
            XIChangeProperty,
            length,
            self.deviceid,
            self.mode,
            self.format,
            self.property_atom,
            self.type_atom,
            num_items,
        )
        return header + self.data + b"\x00" * pad_len


@dataclass
class XIGetPropertyRequest:
    """XIGetProperty request."""

    opcode: int
    deviceid: int
    property_atom: int
    type_atom: int = 0  # AnyPropertyType
    offset: int = 0
    length: int = 0xFFFF
    delete: bool = False

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH HBx II II",
            self.opcode,
            XIGetProperty,
            6,  # 24 bytes = 6 words
            self.deviceid,
            1 if self.delete else 0,
            self.property_atom,
            self.type_atom,
            self.offset,
            self.length,
        )
