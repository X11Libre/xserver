# SPDX-License-Identifier: MIT
#
# XI/XI2 (XInput) extension protocol request builders

import struct
from dataclasses import dataclass

# XI (v1) minor opcodes
XGrabDeviceButton = 17
XChangeDeviceControl = 35
XChangeDeviceProperty = 37
XGetDeviceProperty = 39

# XI2 minor opcodes
XIChangeCursor = 42
XIChangeHierarchy = 43
XIQueryVersion = 47
XIQueryDevice = 48
XIPassiveGrabDevice = 54
XIPassiveUngrabDevice = 55
XIChangeProperty = 57
XIGetProperty = 59
XIBarrierReleasePointer = 61

# XIChangeHierarchy change types
XIAddMaster = 1
XIRemoveMaster = 2

# XIRemoveMaster return modes
XIAttachToMaster = 1
XIFloating = 2

# XIQueryDevice special device IDs
# XIAllDevices = 0  (already defined below)
# XIAllMasterDevices = 1  (already defined below)

# XI device use types (from xXIDeviceInfo)
XIMasterPointer = 1
XIMasterKeyboard = 2
XISlavePointer = 3
XISlaveKeyboard = 4
XIFloatingSlave = 5

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

# Device control types (for XChangeDeviceControl)
DEVICE_RESOLUTION = 1
DEVICE_ABS_CALIB = 2
DEVICE_ABS_AREA = 3
DEVICE_CORE = 4
DEVICE_ENABLE = 5


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
class XIChangeCursorRequest:
    """XIChangeCursor request."""

    opcode: int
    window: int
    cursor: int
    deviceid: int = XIAllMasterDevices

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH II HH",
            self.opcode,
            XIChangeCursor,
            4,
            self.window,
            self.cursor,
            self.deviceid,
            0,  # pad
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

    def to_bytes(self, byte_order: str = "<") -> bytes:
        num_items = (
            self.num_items
            if self.num_items is not None
            else (len(self.data) // (self.format // 8) if self.data else 0)
        )

        total_bytes = 20 + len(self.data)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len
        length = total_bytes // 4

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


@dataclass
class XChangeDevicePropertyRequest:
    """XChangeDeviceProperty request (XI v1, minor opcode 37).

    Wire format (xChangeDevicePropertyReq):
        opcode(1) + ReqType(1) + length(2) + property(4) + type(4) +
        deviceid(1) + format(1) + mode(1) + pad(1) + nUnits(4)
    Total header: 20 bytes, followed by property data.
    """

    opcode: int
    deviceid: int
    property_atom: int
    type_atom: int
    format: int = 32
    mode: int = PropModeReplace
    data: bytes = b""
    num_items: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        num_items = (
            self.num_items
            if self.num_items is not None
            else (len(self.data) // (self.format // 8) if self.data else 0)
        )

        total_bytes = 20 + len(self.data)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len
        length = total_bytes // 4

        header = struct.pack(
            f"{byte_order}BBH II BBBx I",
            self.opcode,
            XChangeDeviceProperty,
            length,
            self.property_atom,
            self.type_atom,
            self.deviceid,
            self.format,
            self.mode,
            num_items,
        )
        return header + self.data + b"\x00" * pad_len


@dataclass
class XGetDevicePropertyRequest:
    """XGetDeviceProperty request (XI v1, minor opcode 39).

    Wire format (xGetDevicePropertyReq):
        opcode(1) + ReqType(1) + length(2) + property(4) + type(4) +
        longOffset(4) + longLength(4) + deviceid(1) + delete(1) + pad(2)
    Total: 24 bytes = 6 words.
    """

    opcode: int
    deviceid: int
    property_atom: int
    type_atom: int = 0  # AnyPropertyType
    offset: int = 0
    req_length: int = 0xFFFF
    delete: bool = False

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH II II BBxx",
            self.opcode,
            XGetDeviceProperty,
            6,  # 24 bytes = 6 words
            self.property_atom,
            self.type_atom,
            self.offset,
            self.req_length,
            self.deviceid,
            1 if self.delete else 0,
        )


@dataclass
class XChangeDeviceControlRequest:
    """XChangeDeviceControl request (XI v1, minor opcode 35).

    The request header is 8 bytes (xChangeDeviceControlReq), followed by
    a device control structure that depends on the control type.
    For DEVICE_RESOLUTION, the control is xDeviceResolutionCtl (12 bytes)
    followed by num_valuators CARD32 values.
    """

    opcode: int
    control: int
    deviceid: int
    control_data: bytes = b""

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total = 8 + len(self.control_data)
        pad_len = (4 - total % 4) % 4
        length = (total + pad_len) // 4

        header = struct.pack(
            f"{byte_order}BBH HBx",
            self.opcode,
            XChangeDeviceControl,
            length,
            self.control,
            self.deviceid,
        )
        return header + self.control_data + b"\x00" * pad_len


@dataclass
class DeviceResolutionCtl:
    """xDeviceResolutionCtl structure (8 bytes + valuator values).

    control(2) + length(2) + first_valuator(1) + num_valuators(1) + pad(2)
    followed by num_valuators CARD32 resolution values.
    """

    first_valuator: int = 0
    num_valuators: int = 0
    resolutions: list[int] | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        vals = self.resolutions if self.resolutions is not None else []
        val_data = b""
        for v in vals:
            val_data += struct.pack(f"{byte_order}I", v)

        ctl_length = (8 + len(val_data)) // 4

        header = struct.pack(
            f"{byte_order}HH BB xx",
            DEVICE_RESOLUTION,
            ctl_length,
            self.first_valuator,
            self.num_valuators,
        )
        return header + val_data


@dataclass
class XIAddMasterInfo:
    """XIAddMaster change body for XIChangeHierarchy.

    Wire format (xXIAddMasterInfo):
        CARD16   type           (1 = XIAddMaster)
        CARD16   length         (in 4-byte words)
        CARD16   name_len
        CARD8    send_core
        CARD8    enable
        STRING8  name           (padded to 4 bytes)
    """

    name: str
    send_core: int = 1
    enable: int = 1

    def to_bytes(self, byte_order: str = "<") -> bytes:
        name_bytes = self.name.encode("ascii")
        name_padded = name_bytes + b"\x00" * ((4 - len(name_bytes) % 4) % 4)
        total = 8 + len(name_padded)
        length = total // 4

        header = struct.pack(
            f"{byte_order}HH HBB",
            XIAddMaster,
            length,
            len(name_bytes),
            self.send_core,
            self.enable,
        )
        return header + name_padded


@dataclass
class XIRemoveMasterInfo:
    """XIRemoveMaster change body for XIChangeHierarchy.

    Wire format (xXIRemoveMasterInfo):
        CARD16   type           (2 = XIRemoveMaster)
        CARD16   length         (3 words = 12 bytes)
        CARD16   deviceid
        CARD8    return_mode    (1=AttachToMaster, 2=Floating)
        CARD8    pad
        CARD16   return_pointer (device to attach pointer slaves to)
        CARD16   return_keyboard (device to attach keyboard slaves to)
    """

    deviceid: int
    return_mode: int = XIFloating
    return_pointer: int = VirtualCorePointer
    return_keyboard: int = VirtualCoreKeyboard

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}HH HBx HH",
            XIRemoveMaster,
            3,  # length = 3 words (12 bytes)
            self.deviceid,
            self.return_mode,
            self.return_pointer,
            self.return_keyboard,
        )


@dataclass
class XIChangeHierarchyRequest:
    """XIChangeHierarchy request (XI2 minor opcode 43).

    Wire format (xXIChangeHierarchyReq):
        CARD8    reqType         (XI major opcode)
        CARD8    ReqType         (43)
        CARD16   length
        CARD8    num_changes
        CARD8    pad0
        CARD16   pad1
        <changes>               (variable-length change bodies)
    """

    opcode: int
    num_changes: int
    changes_data: bytes

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total = 8 + len(self.changes_data)
        length = total // 4

        header = struct.pack(
            f"{byte_order}BBH Bx H",
            self.opcode,
            XIChangeHierarchy,
            length,
            self.num_changes,
            0,  # pad1
        )
        return header + self.changes_data


@dataclass
class XIQueryDeviceRequest:
    """XIQueryDevice request (XI2 minor opcode 48).

    Wire format (xXIQueryDeviceReq):
        CARD8    reqType         (XI major opcode)
        CARD8    ReqType         (48)
        CARD16   length          (2)
        CARD16   deviceid
        CARD16   pad
    """

    opcode: int
    deviceid: int = XIAllDevices

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH HH",
            self.opcode,
            XIQueryDevice,
            2,  # 8 bytes = 2 words
            self.deviceid,
            0,  # pad
        )


@dataclass
class XGrabDeviceButtonRequest:
    """XGrabDeviceButton request (XI v1, minor opcode 17).

    Wire format (xGrabDeviceButtonReq):
        CARD8    reqType         (XI major opcode)
        CARD8    ReqType         (17)
        CARD16   length
        CARD32   grabWindow
        CARD8    grabbed_device
        CARD8    modifier_device
        CARD16   event_count
        CARD16   modifiers
        CARD8    this_device_mode
        CARD8    other_devices_mode
        CARD8    button
        CARD8    ownerEvents
        CARD16   pad
        <event classes>          (event_count * CARD32)
    """

    opcode: int
    grab_window: int
    grabbed_device: int
    modifier_device: int
    button: int = 0  # AnyButton
    modifiers: int = 0x8000  # AnyModifier
    this_device_mode: int = 1  # Async
    other_devices_mode: int = 1  # Async
    owner_events: int = 0
    event_classes: list[int] | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        classes = self.event_classes or []
        event_count = len(classes)
        total = 20 + event_count * 4
        length = total // 4

        header = struct.pack(
            f"{byte_order}BBH I BB H HBB BB H",
            self.opcode,
            XGrabDeviceButton,
            length,
            self.grab_window,
            self.grabbed_device,
            self.modifier_device,
            event_count,
            self.modifiers,
            self.this_device_mode,
            self.other_devices_mode,
            self.button,
            self.owner_events,
            0,  # pad
        )

        class_data = b""
        for c in classes:
            class_data += struct.pack(f"{byte_order}I", c)

        return header + class_data


@dataclass
class XIDeviceInfo:
    """Parsed xXIDeviceInfo structure from XIQueryDevice reply.

    Wire format (xXIDeviceInfo):
        CARD16   deviceid
        CARD16   use               (MasterPointer, MasterKeyboard, etc.)
        CARD16   attachment
        CARD16   num_classes
        CARD16   name_len
        CARD8    enabled
        CARD8    pad
        STRING8  name              (name_len bytes, padded to 4)
        <class info structs>       (num_classes * variable-length)
    """

    deviceid: int
    use: int
    attachment: int
    name: str
    enabled: bool

    @classmethod
    def parse_device_list(
        cls, data: bytes, offset: int, num_devices: int, byte_order: str = "<"
    ) -> list["XIDeviceInfo"]:
        """Parse a list of XIDeviceInfo structures from reply data.

        Returns a list of XIDeviceInfo objects and updates offset to point
        past the last device.
        """
        devices = []
        current_offset = offset

        for _ in range(num_devices):
            if current_offset + 12 > len(data):
                break

            deviceid, use, attachment, num_classes, name_len, enabled = (
                struct.unpack_from(f"{byte_order}HHH HHB x", data, current_offset)
            )
            current_offset += 12

            # Read device name
            if current_offset + name_len > len(data):
                break
            device_name = data[current_offset : current_offset + name_len].decode(
                "ascii", errors="replace"
            )
            name_padded = (name_len + 3) & ~3
            current_offset += name_padded

            # Skip class info structs
            for _ in range(num_classes):
                if current_offset + 4 > len(data):
                    break
                _cls_type, cls_len = struct.unpack_from(
                    f"{byte_order}HH", data, current_offset
                )
                # cls_len is in 4-byte words
                current_offset += cls_len * 4

            devices.append(
                cls(
                    deviceid=deviceid,
                    use=use,
                    attachment=attachment,
                    name=device_name,
                    enabled=bool(enabled),
                )
            )

        return devices


@dataclass
class XIQueryDeviceReply:
    """Parsed XIQueryDevice reply.

    Wire format (xXIQueryDeviceReply):
        type(1) + pad(1) + sequenceNumber(2) + length(4)
        num_devices(2) + pad(22)
        <device info list> (num_devices * variable-length xXIDeviceInfo)
    """

    devices: list[XIDeviceInfo]

    @classmethod
    def from_reply(cls, data: bytes, byte_order: str = "<") -> "XIQueryDeviceReply":
        """Parse an X11Reply's raw data into an XIQueryDeviceReply."""
        if len(data) < 32:
            return cls(devices=[])

        num_devices = struct.unpack_from(f"{byte_order}H", data, 8)[0]
        devices = XIDeviceInfo.parse_device_list(
            data, offset=32, num_devices=num_devices, byte_order=byte_order
        )

        return cls(devices=devices)


@dataclass
class XIBarrierReleasePointerRequest:
    """XIBarrierReleasePointer request (XI2 minor opcode 61).

    Wire format (xXIBarrierReleasePointerReq):
        CARD8    reqType         (XI major opcode)
        CARD8    ReqType         (61)
        CARD16   length
        CARD32   num_barriers
        <barriers>               (num_barriers * xXIBarrierReleasePointerInfo)

    Each xXIBarrierReleasePointerInfo (12 bytes):
        CARD16   deviceid
        CARD16   pad
        CARD32   barrier
        CARD32   eventid
    """

    opcode: int
    barriers: list[tuple[int, int, int]]
    """List of (deviceid, barrier_id, eventid)"""

    def to_bytes(self, byte_order: str = "<") -> bytes:
        num_barriers = len(self.barriers)
        total = 8 + num_barriers * 12
        length = total // 4

        header = struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            XIBarrierReleasePointer,
            length,
            num_barriers,
        )

        payload = b""
        for deviceid, barrier_id, eventid in self.barriers:
            payload += struct.pack(
                f"{byte_order}HH II",
                deviceid,
                0,  # pad
                barrier_id,
                eventid,
            )

        return header + payload
