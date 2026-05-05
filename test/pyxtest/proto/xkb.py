# SPDX-License-Identifier: MIT
#
# XKB protocol request builders
#
# All fields are controllable via keyword arguments so tests can craft
# malformed requests with inconsistent lengths, out-of-range indices, etc.

import struct
from dataclasses import dataclass

# XKB minor opcodes
XkbUseExtension = 0
XkbSelectEvents = 1
XkbBell = 3
XkbGetState = 4
XkbLatchLockState = 5
XkbGetControls = 6
XkbSetControls = 7
XkbGetMap = 8
XkbSetMap = 9
XkbGetCompatMap = 10
XkbSetCompatMap = 11
XkbGetIndicatorState = 12
XkbGetIndicatorMap = 13
XkbSetIndicatorMap = 14
XkbGetNamedIndicator = 15
XkbSetNamedIndicator = 16
XkbGetNames = 17
XkbSetNames = 18
XkbGetGeometry = 19
XkbSetGeometry = 20
XkbPerClientFlags = 21
XkbListComponents = 22
XkbGetKbdByName = 23
XkbGetDeviceInfo = 24
XkbSetDeviceInfo = 25
XkbSetDebuggingFlags = 101

# XKB constants
XkbUseCoreKbd = 0x0100

# SetMap present flags
XkbKeyTypesMask = 0x0001
XkbKeySymsMask = 0x0002
XkbModifierMapMask = 0x0004
XkbExplicitComponentsMask = 0x0008
XkbKeyActionsMask = 0x0010
XkbKeyBehaviorsMask = 0x0020
XkbVirtualModsMask = 0x0040
XkbVirtualModMapMask = 0x0080

# SetMap flags
XkbSetMapResizeTypes = 1
XkbSetMapRecomputeActions = 2

XkbNumRequiredTypes = 4
XkbMaxLegalKeyCode = 255
XkbNoShape = 0xFF


@dataclass
class UseExtensionRequest:
    """XkbUseExtension request (minor opcode 0)."""

    opcode: int
    major: int = 1
    minor: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHHH",
            self.opcode,
            XkbUseExtension,
            2,  # 8 bytes = 2 words
            self.major,
            self.minor,
        )


# Keep as a module-level function for backward compatibility with
# xclient.py which calls xkb.use_extension() directly.
def use_extension(
    opcode: int, *, major: int = 1, minor: int = 0, byte_order: str = "<"
) -> bytes:
    """Build XkbUseExtension request (minor opcode 0)."""
    return UseExtensionRequest(opcode=opcode, major=major, minor=minor).to_bytes(
        byte_order
    )


@dataclass
class GetMapRequest:
    """xkbGetMapReq (minor opcode 8). 28 bytes."""

    opcode: int
    device_spec: int = XkbUseCoreKbd
    full: int = 0
    partial: int = 0
    first_type: int = 0
    n_types: int = 0
    first_key_sym: int = 0
    n_key_syms: int = 0
    first_key_act: int = 0
    n_key_acts: int = 0
    first_key_behavior: int = 0
    n_key_behaviors: int = 0
    virtual_mods: int = 0
    first_key_explicit: int = 0
    n_key_explicit: int = 0
    first_mod_map_key: int = 0
    n_mod_map_keys: int = 0
    first_vmod_map_key: int = 0
    n_vmod_map_keys: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH"  # reqType, xkbReqType, length
            f"HHH"  # deviceSpec, full, partial
            f"BB"  # firstType, nTypes
            f"BB"  # firstKeySym, nKeySyms
            f"BB"  # firstKeyAct, nKeyActs
            f"BB"  # firstKeyBehavior, nKeyBehaviors
            f"H"  # virtualMods
            f"BB"  # firstKeyExplicit, nKeyExplicit
            f"BB"  # firstModMapKey, nModMapKeys
            f"BB"  # firstVModMapKey, nVModMapKeys
            f"H",  # pad
            self.opcode,
            XkbGetMap,
            7,  # 28 bytes = 7 words
            self.device_spec,
            self.full,
            self.partial,
            self.first_type,
            self.n_types,
            self.first_key_sym,
            self.n_key_syms,
            self.first_key_act,
            self.n_key_acts,
            self.first_key_behavior,
            self.n_key_behaviors,
            self.virtual_mods,
            self.first_key_explicit,
            self.n_key_explicit,
            self.first_mod_map_key,
            self.n_mod_map_keys,
            self.first_vmod_map_key,
            self.n_vmod_map_keys,
            0,  # pad
        )


@dataclass
class SetMapRequest:
    """
    xkbSetMapReq (minor opcode 9).

    The 36-byte header is followed by variable-length payload data
    whose format depends on the 'present' bitmask. For security testing,
    payload is passed as raw bytes, allowing intentionally malformed data.
    """

    opcode: int
    device_spec: int = XkbUseCoreKbd
    present: int = 0
    flags: int = 0
    min_key_code: int = 8
    max_key_code: int = 255
    first_type: int = 0
    n_types: int = 0
    first_key_sym: int = 0
    n_key_syms: int = 0
    total_syms: int = 0
    first_key_act: int = 0
    n_key_acts: int = 0
    total_acts: int = 0
    first_key_behavior: int = 0
    n_key_behaviors: int = 0
    total_key_behaviors: int = 0
    first_key_explicit: int = 0
    n_key_explicit: int = 0
    total_key_explicit: int = 0
    first_mod_map_key: int = 0
    n_mod_map_keys: int = 0
    total_mod_map_keys: int = 0
    first_vmod_map_key: int = 0
    n_vmod_map_keys: int = 0
    total_vmod_map_keys: int = 0
    virtual_mods: int = 0
    payload: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 36 + len(self.payload)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH"  # reqType, xkbReqType, length
            f"HHH"  # deviceSpec, present, flags
            f"BBB"  # minKeyCode, maxKeyCode, firstType
            f"B"  # nTypes
            f"BB"  # firstKeySym, nKeySyms
            f"H"  # totalSyms
            f"BB"  # firstKeyAct, nKeyActs
            f"H"  # totalActs
            f"BB"  # firstKeyBehavior, nKeyBehaviors
            f"B"  # totalKeyBehaviors
            f"BB"  # firstKeyExplicit, nKeyExplicit
            f"B"  # totalKeyExplicit
            f"BB"  # firstModMapKey, nModMapKeys
            f"B"  # totalModMapKeys
            f"BB"  # firstVModMapKey, nVModMapKeys
            f"B"  # totalVModMapKeys
            f"H",  # virtualMods
            self.opcode,
            XkbSetMap,
            length,
            self.device_spec,
            self.present,
            self.flags,
            self.min_key_code,
            self.max_key_code,
            self.first_type,
            self.n_types,
            self.first_key_sym,
            self.n_key_syms,
            self.total_syms,
            self.first_key_act,
            self.n_key_acts,
            self.total_acts,
            self.first_key_behavior,
            self.n_key_behaviors,
            self.total_key_behaviors,
            self.first_key_explicit,
            self.n_key_explicit,
            self.total_key_explicit,
            self.first_mod_map_key,
            self.n_mod_map_keys,
            self.total_mod_map_keys,
            self.first_vmod_map_key,
            self.n_vmod_map_keys,
            self.total_vmod_map_keys,
            self.virtual_mods,
        )
        return header + self.payload + b"\x00" * pad_len


@dataclass
class KeyTypeWire:
    """
    xkbKeyTypeWireDesc (8 bytes) with optional entries/preserve data.
    """

    num_levels: int = 2
    has_preserve: bool = False
    n_map_entries: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        header = struct.pack(
            f"{byte_order}BBH BBBB",
            0,
            0,
            0,  # mask, realMods, virtualMods
            self.num_levels,
            self.n_map_entries,
            1 if self.has_preserve else 0,  # preserve
            0,  # pad
        )
        # Map entries: 4 bytes each (level(1), realMods(1), virtualMods(2))
        entries = b"\x00" * (4 * self.n_map_entries)
        # Preserve entries: 4 bytes each (realMods(1), pad(1), virtualMods(2))
        preserve = b"\x00" * (4 * self.n_map_entries) if self.has_preserve else b""
        return header + entries + preserve


@dataclass
class SetCompatMapRequest:
    """xkbSetCompatMapReq (minor opcode 11). Header is 16 bytes."""

    opcode: int
    device_spec: int = XkbUseCoreKbd
    recompute_actions: int = 0
    truncate_si: int = 0
    groups: int = 0
    first_si: int = 0
    n_si: int = 0
    payload: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 16 + len(self.payload)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH"  # reqType, xkbReqType, length
            f"H"  # deviceSpec
            f"xB"  # pad, recomputeActions
            f"B"  # truncateSI
            f"B"  # groups
            f"H"  # firstSI
            f"H"  # nSI
            f"xx",  # pad
            self.opcode,
            XkbSetCompatMap,
            length,
            self.device_spec,
            self.recompute_actions,
            self.truncate_si,
            self.groups,
            self.first_si,
            self.n_si,
        )
        return header + self.payload + b"\x00" * pad_len


@dataclass
class SymInterpretWire:
    """A single xkbSymInterpretWireDesc (16 bytes)."""

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}I"  # sym (KeySym)
            f"BBBx"  # mods, match, virtualMod, pad
            f"8s",  # action (8 bytes)
            0,
            0,
            0,
            0,
            b"\x00" * 8,
        )


@dataclass
class SetGeometryRequest:
    """xkbSetGeometryReq (minor opcode 20). Header is 28 bytes."""

    opcode: int
    device_spec: int = XkbUseCoreKbd
    n_shapes: int = 0
    n_sections: int = 0
    name_atom: int = 0
    width_mm: int = 100
    height_mm: int = 100
    n_properties: int = 0
    n_colors: int = 0
    n_doodads: int = 0
    n_key_aliases: int = 0
    base_color_ndx: int = 0
    label_color_ndx: int = 1
    payload: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 28 + len(self.payload)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH"  # reqType, xkbReqType, length
            f"H"  # deviceSpec
            f"BB"  # nShapes, nSections
            f"I"  # name (Atom)
            f"HH"  # widthMM, heightMM
            f"HH"  # nProperties, nColors
            f"HH"  # nDoodads, nKeyAliases
            f"BB"  # baseColorNdx, labelColorNdx
            f"xx",  # pad
            self.opcode,
            XkbSetGeometry,
            length,
            self.device_spec,
            self.n_shapes,
            self.n_sections,
            self.name_atom,
            self.width_mm,
            self.height_mm,
            self.n_properties,
            self.n_colors,
            self.n_doodads,
            self.n_key_aliases,
            self.base_color_ndx,
            self.label_color_ndx,
        )
        return header + self.payload + b"\x00" * pad_len


@dataclass
class CountedString:
    """A counted string: CARD16 length + chars, padded to 4 bytes."""

    value: str | bytes

    def to_bytes(self, byte_order: str = "<") -> bytes:
        s = self.value
        if isinstance(s, str):
            s = s.encode("ascii")
        length = len(s)
        total = 2 + length  # CARD16 header + string bytes
        pad_len = (4 - total % 4) % 4
        return struct.pack(f"{byte_order}H", length) + s + b"\x00" * pad_len


# Keep as a module-level function for convenience since tests use it
# inline in payload construction.
def build_counted_string(s: str | bytes, byte_order: str = "<") -> bytes:
    """Build a counted string: CARD16 length + chars, padded to 4 bytes."""
    return CountedString(value=s).to_bytes(byte_order)


@dataclass
class ShapeWire:
    """
    xkbShapeWireDesc (8 bytes) + outline data.

    Wire layout:
      name(4 Atom) + nOutlines(1) + primaryNdx(1) + approxNdx(1) + pad(1)
    Each outline: header(4 bytes: nPoints(1) + cornerRadius(1) + pad(2))
                  + nPoints * point(4 bytes: x(2) + y(2)).
    """

    name: int = 0  # Atom
    n_outlines: int = 1
    primary_ndx: int = 0
    approx_ndx: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        header = struct.pack(
            f"{byte_order}IBBBx",
            self.name,
            self.n_outlines,
            self.primary_ndx,
            self.approx_ndx,
        )
        outlines = b""
        for _ in range(self.n_outlines):
            outline_hdr = struct.pack(f"{byte_order}BBxx", 1, 0)  # 1 point
            point = struct.pack(f"{byte_order}hh", 0, 0)
            outlines += outline_hdr + point
        return header + outlines


@dataclass
class SectionWire:
    """xkbSectionWireDesc (20 bytes) + row data."""

    name: int = 0  # Atom
    n_rows: int = 1
    n_doodads: int = 0
    n_overlays: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        header = struct.pack(
            f"{byte_order}I hh HH h BBBBxx",
            self.name,  # name (Atom)
            0,
            0,  # top, left
            100,
            100,  # width, height
            0,  # angle
            0,  # priority
            self.n_rows,
            self.n_doodads,
            self.n_overlays,
        )
        rows = b""
        for _ in range(self.n_rows):
            rows += struct.pack(f"{byte_order}hhBBxx", 0, 0, 0, 0)
        return header + rows


@dataclass
class OverlayWire:
    """xkbOverlayWireDesc + overlay rows."""

    name: int = 0  # Atom
    n_rows: int = 1
    rows_under: list[int] | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        header = struct.pack(f"{byte_order}IBxxx", self.name, self.n_rows)
        rows_under = (
            self.rows_under if self.rows_under is not None else list(range(self.n_rows))
        )
        rows = b""
        for row_under in rows_under:
            rows += struct.pack(f"{byte_order}BBxx", row_under, 0)
        return header + rows


@dataclass
class GetKbdByNameRequest:
    """xkbGetKbdByNameReq (minor opcode 23)."""

    opcode: int
    device_spec: int = XkbUseCoreKbd
    need: int = 0
    want: int = 0
    load: int = 0
    payload: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 12 + len(self.payload)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH HHHBx",
            self.opcode,
            XkbGetKbdByName,
            length,
            self.device_spec,
            self.need,
            self.want,
            self.load,
        )
        return header + self.payload + b"\x00" * pad_len


# XkbSetNames 'which' flags
XkbKeycodesNameMask = 1 << 0
XkbGeometryNameMask = 1 << 1
XkbSymbolsNameMask = 1 << 2
XkbPhysSymbolsNameMask = 1 << 3
XkbTypesNameMask = 1 << 4
XkbCompatNameMask = 1 << 5
XkbKeyTypeNamesMask = 1 << 6
XkbKTLevelNamesMask = 1 << 7
XkbIndicatorNamesMask = 1 << 8
XkbKeyNamesMask = 1 << 9
XkbKeyAliasesMask = 1 << 10
XkbVirtualModNamesMask = 1 << 11
XkbGroupNamesMask = 1 << 12
XkbRGNamesMask = 1 << 13

# XkbSetDeviceInfo 'change' flags
XkbXI_ButtonActionsMask = 1 << 0
XkbXI_IndicatorNamesMask = 1 << 1
XkbXI_IndicatorMapsMask = 1 << 2
XkbXI_IndicatorStateMask = 1 << 3
XkbXI_IndicatorsMask = 0x0E  # Names | Maps | State


@dataclass
class SetNamesRequest:
    """xkbSetNamesReq (minor opcode 18). Header is 28 bytes."""

    opcode: int
    device_spec: int = XkbUseCoreKbd
    virtual_mods: int = 0
    which: int = 0
    first_type: int = 0
    n_types: int = 0
    first_kt_level: int = 0
    n_kt_levels: int = 0
    indicators: int = 0
    group_names: int = 0
    n_radio_groups: int = 0
    first_key: int = 8
    n_keys: int = 0
    n_key_aliases: int = 0
    total_kt_level_names: int = 0
    payload: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 28 + len(self.payload)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH"  # reqType, xkbReqType, length
            f"HH"  # deviceSpec, virtualMods
            f"I"  # which
            f"BBBB"  # firstType, nTypes, firstKTLevel, nKTLevels
            f"I"  # indicators
            f"BB"  # groupNames, nRadioGroups
            f"BB"  # firstKey, nKeys
            f"Bx"  # nKeyAliases, pad
            f"H",  # totalKTLevelNames
            self.opcode,
            XkbSetNames,
            length,
            self.device_spec,
            self.virtual_mods,
            self.which,
            self.first_type,
            self.n_types,
            self.first_kt_level,
            self.n_kt_levels,
            self.indicators,
            self.group_names,
            self.n_radio_groups,
            self.first_key,
            self.n_keys,
            self.n_key_aliases,
            self.total_kt_level_names,
        )
        return header + self.payload + b"\x00" * pad_len


@dataclass
class SetDeviceInfoRequest:
    """xkbSetDeviceInfoReq (minor opcode 25). Header is 12 bytes."""

    opcode: int
    device_spec: int = XkbUseCoreKbd
    first_btn: int = 0
    n_btns: int = 0
    change: int = 0
    n_device_led_fbs: int = 0
    payload: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 12 + len(self.payload)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH HBB HH",
            self.opcode,
            XkbSetDeviceInfo,
            length,
            self.device_spec,
            self.first_btn,
            self.n_btns,
            self.change,
            self.n_device_led_fbs,
        )
        return header + self.payload + b"\x00" * pad_len


# XkbSelectEvents event mask bits
XkbNewKeyboardNotifyMask = 1 << 0
XkbMapNotifyMask = 1 << 1
XkbStateNotifyMask = 1 << 2
XkbControlsNotifyMask = 1 << 3
XkbIndicatorStateNotifyMask = 1 << 4
XkbIndicatorMapNotifyMask = 1 << 5
XkbNamesNotifyMask = 1 << 6
XkbCompatMapNotifyMask = 1 << 7
XkbBellNotifyMask = 1 << 8
XkbActionMessageMask = 1 << 9
XkbAccessXNotifyMask = 1 << 10
XkbExtensionDeviceNotifyMask = 1 << 11


@dataclass
class SelectEventsRequest:
    """xkbSelectEventsReq (minor opcode 1). Header is 16 bytes.

    The header is followed by per-event affect/detail masks for any
    event types set in affectWhich that are NOT in selectAll or clear.
    """

    opcode: int
    device_spec: int = XkbUseCoreKbd
    affect_which: int = 0
    clear: int = 0
    select_all: int = 0
    affect_map: int = 0
    map: int = 0
    payload: bytes = b""
    length_override: int | None = None

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total_bytes = 16 + len(self.payload)
        pad_len = (4 - total_bytes % 4) % 4
        total_bytes += pad_len

        length = (
            self.length_override
            if self.length_override is not None
            else total_bytes // 4
        )

        header = struct.pack(
            f"{byte_order}BBH HH HH HH",
            self.opcode,
            XkbSelectEvents,
            length,
            self.device_spec,
            self.affect_which,
            self.clear,
            self.select_all,
            self.affect_map,
            self.map,
        )
        return header + self.payload + b"\x00" * pad_len
