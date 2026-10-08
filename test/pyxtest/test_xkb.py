# SPDX-License-Identifier: MIT
#
# Security tests for XKB extension vulnerabilities.

import logging
import struct
import time

import pytest

from proto import xkb
from xclient import BadLength, BadMatch, BadValue, X11Error, X11Reply


@pytest.fixture
def xkb_xclient(xclient):
    """Provide an xclient with XKB initialized."""
    opcode = xclient.xkb_use_extension()
    return xclient, opcode


class TestXkbSetMapOverflows:
    """Tests for XKB SetMap buffer overflow and OOB vulnerabilities."""

    @pytest.mark.asan
    def test_setmap_request_length_check(self, xserver, xkb_xclient):
        """
        Missing length validation for variable-length XkbSetMap sub-fields.
        Multiple present bits set but insufficient wire data.

        Fixed in commit 446ff2d31770 ("Check SetMap request length
        carefully.").
        """
        xclient, opcode = xkb_xclient

        all_flags = (
            xkb.XkbKeyTypesMask
            | xkb.XkbKeySymsMask
            | xkb.XkbModifierMapMask
            | xkb.XkbExplicitComponentsMask
            | xkb.XkbKeyActionsMask
            | xkb.XkbKeyBehaviorsMask
            | xkb.XkbVirtualModsMask
            | xkb.XkbVirtualModMapMask
        )

        req = xkb.SetMapRequest(
            opcode=opcode,
            present=all_flags,
            first_type=0,
            n_types=1,
            first_key_sym=8,
            n_key_syms=1,
            total_syms=1,
            first_key_act=8,
            n_key_acts=1,
            total_acts=1,
            first_key_behavior=8,
            n_key_behaviors=1,
            total_key_behaviors=1,
            first_key_explicit=8,
            n_key_explicit=1,
            total_key_explicit=1,
            first_mod_map_key=8,
            n_mod_map_keys=1,
            total_mod_map_keys=1,
            first_vmod_map_key=8,
            n_vmod_map_keys=1,
            total_vmod_map_keys=1,
            virtual_mods=0xFFFF,
            min_key_code=8,
            max_key_code=255,
            payload=b"\x00" * 8,  # Way too little data
        )
        xclient.send_request(req)
        time.sleep(0.5)

        assert xserver.is_alive, "Server crashed - missing length checks in SetMap"

    def test_setmap_key_actions_totalacts_mismatch(self, xserver, xkb_xclient):
        """
        CheckKeyActions() validated per-key action count bytes against
        symsPerKey but did not verify that the computed total action
        data region fell within the request buffer. The upstream length
        check in _XkbSetMapCheckLength() used req->totalActs from the
        header, not the computed sum.  A crafted request with totalActs=0
        but nonzero per-key counts would pass the length check and then
        advance the wire pointer past the buffer.

        The test queries the server's current key map via XkbGetMap to
        learn the symsPerKey values, then crafts a SetMap with
        XkbKeyActionsMask where the per-key counts match symsPerKey but
        totalActs is set to 0.

        Fixed in commit a439a7340ad9 ("xkb: Add bounds check for action
        data in CheckKeyActions()").
        """
        xclient, opcode = xkb_xclient

        # Step 1: Query the server's key sym map to learn symsPerKey
        get_map = xkb.GetMapRequest(opcode=opcode, full=xkb.XkbKeySymsMask)
        xclient.send_request(get_map)
        resp = xclient.recv_response(timeout=5.0)
        assert isinstance(resp, X11Reply), f"Expected GetMap reply, got {resp}"

        data = resp.data
        min_key = data[10]
        max_key = data[11]
        first_key_sym = data[17]
        n_key_syms = data[20]

        # Parse xkbSymMapWireDesc entries to extract nSyms per key
        offset = 40  # variable data starts after 40-byte reply header
        syms_per_key = {}
        for i in range(n_key_syms):
            n_syms = struct.unpack_from("<H", data, offset + 6)[0]
            syms_per_key[first_key_sym + i] = n_syms
            offset += 8 + n_syms * 4

        # Step 2: Find a range of keys with nonzero symsPerKey
        first_key_act = min_key
        n_key_acts = 5
        act_counts = []
        for kc in sorted(syms_per_key.keys()):
            act_counts = [syms_per_key.get(kc + i, 0) for i in range(n_key_acts)]
            if sum(act_counts) > 0:
                first_key_act = kc
                break

        real_n_acts = sum(act_counts)
        assert real_n_acts > 0, "No keys with nonzero symsPerKey found"

        # Step 3: Build SetMap with totalActs=0 but matching per-key counts
        # The length check expects totalActs*8 bytes of action data,
        # so totalActs=0 means no action data. But CheckKeyActions will
        # compute nActs = real_n_acts and try to advance past the buffer.
        act_bytes = bytes(act_counts)
        pad_len = (4 - len(act_bytes) % 4) % 4
        payload = act_bytes + b"\x00" * pad_len

        req = xkb.SetMapRequest(
            opcode=opcode,
            present=xkb.XkbKeyActionsMask,
            min_key_code=min_key,
            max_key_code=max_key,
            first_key_act=first_key_act,
            n_key_acts=n_key_acts,
            total_acts=0,  # Mismatch: real sum is real_n_acts
            payload=payload,
        )
        xclient.send_request(req)
        resps = xclient.flush_responses(timeout=0.5)
        errors = [r for r in resps if isinstance(r, X11Error)]

        # With the fix, CheckKeyActions returns BadValue (2) with
        # error code 0x25 in the resource_id.  Without the fix, the
        # wire pointer advances past the buffer and a later check
        # returns BadLength (16).
        bad_value_errors = [e for e in errors if e.error_code == BadValue]
        assert bad_value_errors, (
            "SetMap with totalActs=0 but nonzero per-key action counts "
            "was not rejected with BadValue - server is missing the "
            "CheckKeyActions bounds check"
        )

        assert xserver.is_alive, (
            "Server crashed - CheckKeyActions totalActs mismatch OOB"
        )

    @pytest.mark.asan
    def test_resize_key_type_size_syms_overflow(self, xserver, xkb_xclient):
        """
        ZDI-CAN-31834: XkbResizeKeyType numeric truncation in size_syms.

        size_syms is declared as unsigned short (16-bit) in XkbClientMapRec.
        When XkbResizeKeyType() computes (nTotal * 15) / 10 for the new
        size_syms, the 32-bit result is truncated to 16 bits, causing an
        undersized calloc. The subsequent copy loop writes based on the
        actual key count, overflowing the heap buffer.

        Phase 1: Add a custom key type with numLevels=2 and configure all
        keys (248) with 4 groups using that type.
        Phase 2: Resize the type to numLevels=63, causing nTotal to exceed
        65535 and triggering the unsigned short truncation in size_syms.
        """
        xclient, opcode = xkb_xclient

        # Query current keymap to learn key range and type count
        get_map = xkb.GetMapRequest(
            opcode=opcode,
            full=xkb.XkbKeyTypesMask | xkb.XkbKeySymsMask,
        )
        xclient.send_request(get_map)
        resp = xclient.recv_response(timeout=5.0)
        assert isinstance(resp, X11Reply), f"Expected GetMap reply, got {resp}"
        reply = xkb.GetMapReply.from_bytes(resp.data[:32], resp.data[32:])

        min_key = reply.min_key_code
        max_key = reply.max_key_code
        n_keys = max_key - min_key + 1
        n_types = reply.n_types + reply.first_type  # total types
        type_idx = n_types  # index of the new type we'll add
        num_lvls_initial = 2
        groups = 4
        syms_per_key = groups * num_lvls_initial  # 8

        # Phase 1: Add new type (numLevels=2) + set all keys with 4 groups
        # xkbKeyTypeWireDesc: 8 bytes
        type_wire = struct.pack("<BBHBBBB", 0, 0, 0, num_lvls_initial, 0, 0, 0)

        # xkbSymMapWireDesc (8 bytes) + syms per key
        sym_data = b""
        for _ in range(n_keys):
            sym_data += struct.pack(
                "<BBBBBBH",
                type_idx,  # kt_index[0]
                type_idx,  # kt_index[1]
                type_idx,  # kt_index[2]
                type_idx,  # kt_index[3]
                groups,  # groupInfo
                num_lvls_initial,  # width
                syms_per_key,  # nSyms
            )
            # sym data: symsPerKey keysyms
            for s in range(syms_per_key):
                sym_data += struct.pack("<I", 0x61 + s)

        total_syms = n_keys * syms_per_key

        payload1 = type_wire + sym_data
        req1 = xkb.SetMapRequest(
            opcode=opcode,
            present=xkb.XkbKeyTypesMask | xkb.XkbKeySymsMask,
            flags=xkb.XkbSetMapResizeTypes,
            min_key_code=min_key,
            max_key_code=max_key,
            first_type=type_idx,
            n_types=1,
            first_key_sym=min_key,
            n_key_syms=n_keys,
            total_syms=total_syms & 0xFFFF,
            payload=payload1,
        )
        xclient.send_request(req1)
        resp = xclient.recv_response(timeout=5.0)
        if isinstance(resp, X11Error):
            pytest.skip(f"Phase 1 SetMap failed: error {resp.error_code}")

        assert xserver.is_alive, "Server crashed during phase 1 setup"

        # Phase 2: Resize type to numLevels=63 to trigger overflow
        new_levels = 63
        type_wire2 = struct.pack("<BBHBBBB", 0, 0, 0, new_levels, 0, 0, 0)

        req2 = xkb.SetMapRequest(
            opcode=opcode,
            present=xkb.XkbKeyTypesMask,
            flags=0,
            min_key_code=min_key,
            max_key_code=max_key,
            first_type=type_idx,
            n_types=1,
            payload=type_wire2,
        )
        xclient.send_request(req2)
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - XkbResizeKeyType size_syms truncation: "
            "(nTotal * 15) / 10 overflows unsigned short, causing "
            "undersized calloc and heap buffer overflow"
        )


class TestXkbSetGeometry:
    """Tests for XKB SetGeometry OOB vulnerabilities."""

    def _build_colors(self, n, byte_order="<"):
        """Build n color strings for geometry payload."""
        data = b""
        colors = ["white", "black", "red", "green", "blue", "yellow"]
        for i in range(n):
            data += xkb.build_counted_string(
                colors[i % len(colors)], byte_order=byte_order
            )
        return data

    def test_setgeometry_truncated_sections(self, xserver, xkb_xclient):
        """
        Missing bounds checks on nested structures in XkbSetGeometry.
        Valid-looking headers but truncated wire data for sections,
        rows, and overlays.  The request claims 5 sections but provides
        none, so the server must reject it with BadLength.

        Fixed in commit 6907b6ea2b4c ("xkb: add request length validation
        for XkbSetGeometry").
        """
        xclient, opcode = xkb_xclient

        n_colors = 2
        color_data = self._build_colors(n_colors)
        name_atom = xclient.intern_atom("TestGeom6")

        shape_data = xkb.ShapeWire(n_outlines=1).to_bytes()

        req = xkb.SetGeometryRequest(
            opcode=opcode,
            n_shapes=1,
            n_sections=5,  # Claim 5 sections but provide none
            n_colors=n_colors,
            base_color_ndx=0,
            label_color_ndx=1,
            name_atom=name_atom,
            payload=color_data + shape_data,
        )
        xclient.send_request(req)
        resp = xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, "Server crashed - truncated sections in SetGeometry"
        assert isinstance(resp, X11Error), f"Expected an error, got {resp}"
        assert resp.error_code == BadLength, (
            f"Expected BadLength ({BadLength}), got error code {resp.error_code} - "
            f"missing bounds check in SetGeometry section parsing"
        )

    @pytest.mark.parametrize("which_ndx", ["primaryNdx", "approxNdx"])
    def test_primary_approx_ndx_oob(self, xserver, xkb_xclient, which_ndx):
        """
        _CheckSetShapes stores shape->primary from a client-controlled
        primaryNdx without validating it against nOutlines. A client
        can set primaryNdx to a value >= nOutlines, causing
        shape->primary to point past the outlines array (OOB pointer).

        Fixed in commit 86a321ad9821 ("xkb: Fix out-of-bounds array
        access in _CheckSetShapes()").
        """
        xclient, opcode = xkb_xclient

        n_colors = 2
        color_data = self._build_colors(n_colors)
        name_atom = xclient.intern_atom("TestGeomPrimNdx")
        shape_atom = xclient.intern_atom("TestShapePrim")

        # Create a label font string (counted string)
        label_font = xkb.build_counted_string("")

        # Build a shape with 1 outline but primaryNdx=200 (far OOB)
        shape_data = xkb.ShapeWire(
            name=shape_atom,
            n_outlines=1,
            primary_ndx=200 if which_ndx == "primaryNdx" else xkb.XkbNoShape,
            approx_ndx=200 if which_ndx == "approxNdx" else xkb.XkbNoShape,
        ).to_bytes()

        payload = label_font + color_data + shape_data

        req = xkb.SetGeometryRequest(
            opcode=opcode,
            n_shapes=1,
            n_sections=0,
            n_colors=n_colors,
            base_color_ndx=0,
            label_color_ndx=1,
            name_atom=name_atom,
            payload=payload,
        )
        xclient.send_request(req)
        resps = xclient.flush_responses(timeout=0.5)
        errors = [r for r in resps if isinstance(r, X11Error)]

        bad_value_errors = [e for e in errors if e.error_code == BadValue]
        assert bad_value_errors, (
            f"SetGeometry with {which_ndx}=200 nOutlines=1 was not rejected "
            f"with BadValue - server is missing the {which_ndx} bounds check"
        )

        assert xserver.is_alive, "Server crashed - SetGeometry primaryNdx OOB"

    @pytest.mark.parametrize("which_color", ["Base", "Label"])
    def test_base_color_ndx_off_by_one(self, xserver, xkb_xclient, which_color):
        """
        _CheckSetGeom validated baseColorNdx with '>' instead of '>='
        when comparing against nColors. With nColors=2, baseColorNdx=2
        (which is the count, not a valid 0-based index) would pass the
        check but access one element past the colors array.

        Fixed in commit 6b6e8020b902 ("xkb: Fix off-by-one in color
        index validation in _CheckSetGeom()").
        """
        xclient, opcode = xkb_xclient

        n_colors = 2
        color_data = self._build_colors(n_colors)
        name_atom = xclient.intern_atom(f"TestGeom{which_color}Color")
        shape_atom = xclient.intern_atom("TestShapeBaseClr")

        # Label font must parse successfully to reach the color index checks
        label_font = xkb.build_counted_string("")

        # Need at least one shape (nShapes >= 1) so that _CheckSetShapes
        # doesn't reject the request before the color OOB at line 5682
        # can be detected by valgrind.
        shape_data = xkb.ShapeWire(name=shape_atom, n_outlines=1).to_bytes()

        # baseColorNdx=2 with nColors=2: valid indices are 0,1
        # Old code: 2 > 2 is false, so it passes through to the OOB
        #           read at geom->colors[2] (detected by valgrind)
        # Fixed code: 2 >= 2 is true, so it's rejected with BadMatch
        req = xkb.SetGeometryRequest(
            opcode=opcode,
            n_shapes=1,
            n_sections=0,
            n_colors=n_colors,
            base_color_ndx=n_colors
            if which_color == "Base"
            else 0,  # Off-by-one: == nColors
            label_color_ndx=n_colors if which_color == "Label" else 0,
            name_atom=name_atom,
            payload=label_font + color_data + shape_data,
        )
        xclient.send_request(req)
        resps = xclient.flush_responses(timeout=0.5)
        errors = [r for r in resps if isinstance(r, X11Error)]

        # With the fix, we get BadMatch (8) from the color index check.
        # Without the fix, the OOB access happens silently and the
        # request succeeds (no error), so valgrind catches it.
        match_errors = [e for e in errors if e.error_code == BadMatch]
        assert match_errors, (
            f"SetGeometry with {which_color}ColorNdx=nColors was not rejected with "
            f"BadMatch - server is missing the off-by-one check"
        )

        assert xserver.is_alive, (
            f"Server crashed - SetGeometry {which_color}ColorNdx off-by-one"
        )

    def test_overlay_row_under_off_by_one(self, xserver, xkb_xclient):
        """
        _CheckSetOverlay validated rWire->rowUnder with '>' instead of
        '>='. With a section having 1 row, rowUnder=1 (which is the
        count, not a valid 0-based index) would pass the check but
        reference an out-of-bounds row.

        Fixed in commit ed19312c4bda ("xkb: Fix off-by-one and NULL
        dereferences in _CheckSetOverlay()").
        """
        xclient, opcode = xkb_xclient

        n_colors = 2
        color_data = self._build_colors(n_colors)
        name_atom = xclient.intern_atom("TestGeomOverlay")
        shape_atom = xclient.intern_atom("TestShapeOvl")
        section_atom = xclient.intern_atom("TestSection")
        overlay_atom = xclient.intern_atom("TestOverlay")

        label_font = xkb.build_counted_string("")

        # Build a shape with 1 outline (required: nShapes >= 1)
        shape_data = xkb.ShapeWire(name=shape_atom, n_outlines=1).to_bytes()

        # Build a section with 1 row and 1 overlay.
        # The overlay has rowUnder=1 (off-by-one: valid range is 0..0)
        section_data = xkb.SectionWire(
            name=section_atom, n_rows=1, n_doodads=0, n_overlays=1
        ).to_bytes()
        overlay_data = xkb.OverlayWire(
            name=overlay_atom,
            n_rows=1,
            rows_under=[1],  # 1 is OOB (only row 0 exists)
        ).to_bytes()

        payload = label_font + color_data + shape_data + section_data + overlay_data

        req = xkb.SetGeometryRequest(
            opcode=opcode,
            n_shapes=1,
            n_sections=1,
            n_colors=n_colors,
            base_color_ndx=0,
            label_color_ndx=1,
            name_atom=name_atom,
            payload=payload,
        )
        xclient.send_request(req)
        resps = xclient.flush_responses(timeout=0.5)
        errors = [r for r in resps if isinstance(r, X11Error)]

        # With the fix, we get BadMatch (8) from the rowUnder >= num_rows check.
        # Without the fix, rowUnder == num_rows passes the '>' check and
        # the OOB access happens in XkbAddGeomOverlayRow().
        match_errors = [e for e in errors if e.error_code == BadMatch]
        assert match_errors, (
            "SetGeometry with rowUnder=1 num_rows=1 was not rejected with "
            "BadMatch - server is missing the off-by-one check"
        )

        assert xserver.is_alive, (
            "Server crashed - SetGeometry overlay rowUnder off-by-one"
        )


class TestXkbChangeKeycodeRange:
    """Tests for XKB ChangeKeycodeRange vulnerabilities."""

    @pytest.mark.asan
    def test_names_keys_oob_write(self, xserver, xkb_xclient):
        """
        ZDI-CAN-31941: Incomplete fix regression in XkbChangeKeycodeRange.

        Commit a3171732d ("xkb: Always use MAP_LENGTH keymap size")
        converted most XKB allocations to use MAP_LENGTH (256) but missed
        XkbAllocNames(), which still allocates names->keys to
        max_key_code + 1. XkbChangeKeycodeRange() assumes MAP_LENGTH and
        memsets up to index 255, overflowing the undersized buffer.

        The test loads a keycodes component with max_key_code < 255 via
        GetKbdByName, then sends SetMap with maxKeyCode=255 to trigger the
        grow branch in XkbChangeKeycodeRange.
        """
        xclient, opcode = xkb_xclient

        # XkbGBN_AllComponentsMask
        XkbGBN_AllComponentsMask = 0x00FF

        # Build GetKbdByName request with sun(type6) keycodes
        # Component names: keymap(empty), keycodes=sun(type6), types(empty),
        #                  compat(empty), symbols(empty), geometry(empty)
        kc_name = b"sun(type6)"
        names = (
            b"\x00"  # keymap (empty)
            + bytes([len(kc_name)])
            + kc_name  # keycodes
            + b"\x00"  # types (empty)
            + b"\x00"  # compat (empty)
            + b"\x00"  # symbols (empty)
            + b"\x00"  # geometry (empty)
        )

        req = xkb.GetKbdByNameRequest(
            opcode=opcode,
            need=XkbGBN_AllComponentsMask,
            want=XkbGBN_AllComponentsMask,
            load=1,
            payload=names,
        )
        xclient.send_request(req)
        resp = xclient.recv_response(timeout=5.0)

        if isinstance(resp, X11Error):
            pytest.skip(
                f"GetKbdByName failed with error {resp.error_code} - "
                "sun(type6) keycodes may not be available"
            )

        assert xserver.is_alive, "Server crashed during GetKbdByName"

        # Now send SetMap with maxKeyCode=255 to trigger XkbChangeKeycodeRange
        # present=0 means no actual map data, just triggers the grow
        setmap_body = struct.pack("<HHH BB", xkb.XkbUseCoreKbd, 0, 0, 8, 255)
        setmap_body += b"\x00" * 24  # remaining SetMap header fields
        req2 = xkb.SetMapRequest(
            opcode=opcode,
            present=0,
            min_key_code=8,
            max_key_code=255,
            payload=b"",
        )
        xclient.send_request(req2)
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - XkbChangeKeycodeRange names->keys OOB write: "
            "XkbAllocNames allocates max_key_code+1 but memset uses MAP_LENGTH"
        )


class TestXkbSetCompatMap:
    """Tests for XKB SetCompatMap vulnerabilities."""

    @pytest.mark.asan
    def test_setcompatmap_size_vs_num_overflow(self, xserver, xkb_xclient):
        """
        _XkbSetCompatMap compared firstSI + nSI against compat->num_si
        (logical count) instead of compat->size_si (allocated size).
        After a truncation that lowered num_si below size_si, a
        subsequent non-truncating request with firstSI + nSI between
        num_si and size_si would skip the realloc and write past the
        allocated buffer.

        Fixed in commit 4cd853321094 ("xkb: Fix buffer overflow in
        _XkbSetCompatMap()").
        """
        xclient, opcode = xkb_xclient

        # Step 1: Allocate 10 entries (sets size_si=10, num_si=10)
        payload1 = b"".join(xkb.SymInterpretWire().to_bytes() for _ in range(10))
        req1 = xkb.SetCompatMapRequest(
            opcode=opcode,
            first_si=0,
            n_si=10,
            truncate_si=1,
            payload=payload1,
        )
        xclient.send_request(req1)
        xclient.flush_responses(timeout=0.5)

        # Step 2: Truncate to 2 (sets num_si=2, size_si stays 10)
        payload2 = b"".join(xkb.SymInterpretWire().to_bytes() for _ in range(2))
        req2 = xkb.SetCompatMapRequest(
            opcode=opcode,
            first_si=0,
            n_si=2,
            truncate_si=1,
            payload=payload2,
        )
        xclient.send_request(req2)
        xclient.flush_responses(timeout=0.5)

        # Step 3: Write at index 8-11 without truncation.
        # num_si=2, size_si=10. Old code checks firstSI+nSI > num_si
        # and reallocates, but should check size_si.
        payload3 = b"".join(xkb.SymInterpretWire().to_bytes() for _ in range(4))
        req3 = xkb.SetCompatMapRequest(
            opcode=opcode,
            first_si=8,
            n_si=4,  # 8+4=12 > size_si=10
            truncate_si=0,
            payload=payload3,
        )
        xclient.send_request(req3)
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - SetCompatMap size_si vs num_si overflow"
        )


class TestXkbSetNames:
    """Tests for XKB SetNames vulnerabilities."""

    @pytest.mark.asan
    def test_setnames_truncated_atoms(self, xserver, xkb_xclient):
        """
        XkbSetNames with multiple 'which' bits set but truncated atom
        data extends reads past the request buffer.

        Fixed in commit f7cd1276bbd4 ("Correct bounds checking in
        XkbSetNames()").
        """
        xclient, opcode = xkb_xclient

        # Set several name categories but provide too little data
        which = (
            xkb.XkbKeycodesNameMask
            | xkb.XkbTypesNameMask
            | xkb.XkbCompatNameMask
            | xkb.XkbVirtualModNamesMask
            | xkb.XkbRGNamesMask
        )

        req = xkb.SetNamesRequest(
            opcode=opcode,
            which=which,
            virtual_mods=0xFFFF,  # 16 virtual mod names expected
            n_radio_groups=10,  # 10 radio group names expected
            payload=b"\x00" * 12,  # Way too little for all those atoms
        )
        xclient.send_request(req)
        time.sleep(0.5)

        assert xserver.is_alive, "Server crashed - truncated atoms in SetNames"


class TestXkbSetMapNumLevels:
    """Tests for XKB SetMap num_levels validation."""

    @pytest.mark.asan
    def test_num_levels_exceeds_max_shift_level(self, xserver, xkb_xclient):
        """
        ZDI-CAN-30160: CheckKeyTypes did not enforce an upper bound on
        numLevels for non-canonical key types. A client could set
        numLevels up to 255 via XkbSetMap. When ChangeKeyboardMapping
        later triggers XkbUpdateKeyTypesFromCore, the function
        XkbKeyTypesForCoreSymbols uses num_levels as groupsWidth and
        indexes into tsyms[], a stack buffer of XkbMaxSymsPerKey (252)
        entries. With num_levels=255, indices reach up to 1019,
        overflowing the 252-element stack buffer.

        Fixed by rejecting numLevels > XkbMaxShiftLevel (63) in
        CheckKeyTypes alongside the existing check for numLevels < 1.
        """
        EVIL_NUM_LEVELS = 255

        xclient, opcode = xkb_xclient

        # Step 1: Get the full XKB map to discover types and key mappings.
        map_reply = xclient.xkb_get_map(opcode, full=xkb.XkbAllMapComponentsMask)
        assert map_reply is not None, "XkbGetMap failed"

        types = map_reply.types
        sym_maps = map_reply.sym_maps
        explicit_map = map_reply.explicit_map

        logging.debug(
            f"We have {len(types)} types, keycodes are {map_reply.min_key_code}-{map_reply.max_key_code}"
        )
        logging.debug("Types:")
        for idx, t in enumerate(types):
            logging.debug(
                f"type[{idx:02d}]: num_levels={t.num_levels} {'canonical' if idx < 4 else ''}"
            )

        # Step 2: Find a key with a non-canonical type (kt_index >= 4).
        # Prefer one that already has the explicit flag set.
        target_key = -1
        target_type = -1
        target_group = -1
        has_explicit = False

        logging.debug("Scanning keys for non-canonical type with explicit flag")
        for i, sm in enumerate(sym_maps):
            keycode = map_reply.first_key_sym + i
            n_groups = sm.group_info & 0x0F
            expl = explicit_map.get(keycode, 0)

            for g in range(min(n_groups, 4)):
                kt = sm.kt_index[g]
                if kt >= 4 and (expl & (1 << g)):
                    logging.debug(
                        f"FOUND: key={keycode} group={g} kt_index={kt} num_levels={types[kt].num_levels} explicit=0x{expl:02x}"
                    )
                    # Found a key with explicit flag already set.
                    if not has_explicit:
                        target_key = keycode
                        target_type = kt
                        target_group = g
                        has_explicit = True

        # If none found with explicit flag, find any key using type >= 4.
        if target_key < 0:
            logging.debug(
                "No key with explicit + non-canonical type found. Scanning for any type >=4"
            )
            for i, sm in enumerate(sym_maps):
                keycode = map_reply.first_key_sym + i
                n_groups = sm.group_info & 0x0F
                for g in range(min(n_groups, 4)):
                    kt = sm.kt_index[g]
                    if kt >= 4:
                        logging.debug(
                            f"FOUND: key={keycode} group={g} type={kt} num_levels={types[kt].num_levels}"
                        )
                        target_key = keycode
                        target_type = kt
                        target_group = g
                        break
                if target_key >= 0:
                    break

        if target_key < 0:
            pytest.skip("No key using non-canonical type found")

        logging.debug(
            f"Target: key={target_key} group={target_group} type={target_type}"
        )

        # Step 2b: Set the explicit flag on the target key if needed.
        if not has_explicit:
            expl_flags = explicit_map.get(target_key, 0)
            expl_flags |= 1 << target_group
            # Build explicit component payload: (keycode, flags) pairs
            # for all keys that have non-zero explicit, plus our target.
            explicit_map[target_key] = expl_flags
            expl_payload = b""
            for kc in sorted(explicit_map):
                expl_payload += bytes([kc, explicit_map[kc]])
            pad_len = (4 - len(expl_payload) % 4) % 4
            expl_payload += b"\x00" * pad_len

            req = xkb.SetMapRequest(
                opcode=opcode,
                present=xkb.XkbExplicitComponentsMask,
                first_key_explicit=map_reply.first_key_explicit,
                n_key_explicit=map_reply.n_key_explicit,
                total_key_explicit=len(explicit_map),
                min_key_code=map_reply.min_key_code,
                max_key_code=map_reply.max_key_code,
                payload=expl_payload,
            )
            xclient.send_request(req.to_bytes())
            resps = xclient.flush_responses(timeout=0.5)
            errors = [r for r in resps if isinstance(r, X11Error)]
            assert not errors, f"SetMap(explicit) failed: {errors}"

        # Step 3: Modify the target type's num_levels to 255 via
        # XkbSetMap with XkbKeyTypesMask.  Send all types (like Xlib
        # does), with the target type's num_levels changed.
        # Note: GetMap and SetMap use different wire formats for map entries
        # (8-byte xkbKTMapEntryWireDesc vs 4-byte xkbKTSetMapEntryWireDesc),
        # so we must convert via to_set_map_wire().
        types_payload = b""
        for i, t in enumerate(types):
            if i == target_type:
                logging.debug(
                    f"Modifying key type {i} to have num_levels {EVIL_NUM_LEVELS}"
                )
                types_payload += t.to_set_map_wire(num_levels=EVIL_NUM_LEVELS)
            else:
                types_payload += t.to_set_map_wire()

        req = xkb.SetMapRequest(
            opcode=opcode,
            present=xkb.XkbKeyTypesMask,
            first_type=0,
            n_types=len(types),
            min_key_code=map_reply.min_key_code,
            max_key_code=map_reply.max_key_code,
            payload=types_payload,
        )
        xclient.send_request(req.to_bytes())
        resps = xclient.flush_responses(timeout=0.5)
        errors = [r for r in resps if isinstance(r, X11Error)]

        # On a patched server, CheckKeyTypes rejects numLevels > XkbMaxShiftLevel (63).
        # The SetMap must fail with an error.
        assert errors, (
            f"SetMap with num_levels={EVIL_NUM_LEVELS} was accepted - "
            "server is missing the numLevels upper bound check (ZDI-CAN-30160)"
        )
        logging.debug(f"SetMap correctly rejected: {errors}")

        # Step 4: Trigger via ChangeKeyboardMapping on the target key.
        # On an unpatched server where the evil num_levels was accepted,
        # XkbUpdateKeyTypesFromCore would use it as groupsWidth,
        # overflowing the stack-allocated tsyms[252] buffer.
        # On a patched server the SetMap was rejected above, so this
        # is harmless — but we send it anyway to confirm the server
        # stays alive regardless.
        keysyms = [0x41414141 + i for i in range(8)]
        xclient.change_keyboard_mapping(
            first_keycode=target_key,
            keysyms_per_keycode=8,
            keycodes=1,
            keysyms=keysyms,
        )
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - numLevels > XkbMaxShiftLevel (ZDI-CAN-30160)"
        )


class TestXkbSetMapMapWidths:
    """Tests for XKB SetMap mapWidths stack buffer overflow."""

    @pytest.mark.asan
    def test_mapwidths_stack_oob_write(self, xserver, xkb_xclient):
        """
        ZDI-CAN-30161: CheckKeyTypes computes nMaps = firstType + nTypes
        from client-controlled fields when XkbSetMapResizeTypes is set.
        mapWidths[] is a stack-allocated CARD8 array of 256 elements.
        No upper bound was enforced on nMaps.

        Attack: First, SetMap(firstType=0, nTypes=255, ResizeTypes)
        sets num_types to 255. Then SetMap(firstType=255, nTypes=10,
        ResizeTypes) passes the check firstType > num_types (255 > 255
        is false) and computes nMaps=265. The loop then writes
        mapWidths[255..264], overflowing 9 bytes past the stack buffer.

        Fixed by rejecting requests where firstType + nTypes exceeds
        the mapWidths buffer size (XkbMaxLegalKeyCode + 1 = 256).
        """
        xclient, opcode = xkb_xclient

        # Step 1: SetMap with firstType=0, nTypes=255 to set num_types=255.
        # All types must have valid numLevels.
        types_payload = b""
        # Type 0: numLevels=1
        types_payload += xkb.KeyTypeWire(num_levels=1).to_bytes()
        # Types 1-3: numLevels=2
        for _ in range(3):
            types_payload += xkb.KeyTypeWire(num_levels=2).to_bytes()
        # Types 4-254: numLevels=1
        for _ in range(251):
            types_payload += xkb.KeyTypeWire(num_levels=1).to_bytes()

        req = xkb.SetMapRequest(
            opcode=opcode,
            present=xkb.XkbKeyTypesMask,
            flags=xkb.XkbSetMapResizeTypes,
            first_type=0,
            n_types=255,
            min_key_code=8,
            max_key_code=255,
            payload=types_payload,
        )
        xclient.send_request(req.to_bytes())
        xclient.flush_responses(timeout=0.5)

        # Step 2: SetMap with firstType=255, nTypes=10, ResizeTypes.
        # nMaps = 255 + 10 = 265, which exceeds mapWidths[256].
        # Without the fix, this writes past the stack buffer.
        types_payload2 = b""
        for _ in range(10):
            types_payload2 += xkb.KeyTypeWire(num_levels=1).to_bytes()

        req = xkb.SetMapRequest(
            opcode=opcode,
            present=xkb.XkbKeyTypesMask,
            flags=xkb.XkbSetMapResizeTypes,
            first_type=255,
            n_types=10,
            min_key_code=8,
            max_key_code=255,
            payload=types_payload2,
        )
        xclient.send_request(req.to_bytes())
        resps = xclient.flush_responses(timeout=0.5)
        errors = [r for r in resps if isinstance(r, X11Error)]

        # On a patched server, CheckKeyTypes rejects nMaps > XkbMaxLegalKeyCode + 1.
        assert errors, (
            "SetMap with firstType=255 nTypes=10 was accepted - "
            "server is missing the nMaps upper bound check (ZDI-CAN-30161)"
        )

        assert xserver.is_alive, (
            "Server crashed - mapWidths stack OOB write (ZDI-CAN-30161)"
        )


class TestXkbSetMapDesync:
    """Tests for XKB SetMap key-width/action-count desync vulnerabilities."""

    @pytest.mark.asan
    def test_setmap_width_action_count_desync(self, xserver, xkb_xclient):
        """
        ZDI-CAN-32408: XKB SetMap key-width/action-count desync OOB read.

        CheckKeySyms populates symsPerKey[] with new widths for keys in the
        request range, then continues for keys beyond the range starting
        at index i = nKeySyms (not firstKeySym + nKeySyms). When firstKeySym
        is large and nKeySyms is 1, the second loop overwrites
        symsPerKey[target] with the old width from the existing map.

        CheckKeyActions then validates the wire action count against the
        stale old width and accepts it. In _XkbSetMap, SetKeySyms widens
        the key (resizing actions to nGroups * newWidth), but SetKeyActions
        then resizes again to the old count. A subsequent XkbGetMap reads
        XkbKeyNumActions (derived from the new width) entries from the
        shorter allocation, causing an OOB heap read. The leaked bytes
        are sent back to the client in the GetMap reply.
        """
        xclient, opcode = xkb_xclient

        # Step 1: Get current keymap
        reply = xclient.xkb_get_map(
            opcode,
            full=xkb.XkbKeyTypesMask | xkb.XkbKeySymsMask,
        )
        assert reply is not None, "Initial GetMap failed"

        min_key = reply.min_key_code
        max_key = reply.max_key_code
        n_types = reply.total_types
        first_key_sym = reply.first_key_sym

        # Find a target key near max_key_code that has nonzero syms.
        # We pick a key with small n_syms so the desync between old
        # and new width is large.
        target_key = None
        target_sym = None
        for i in range(len(reply.sym_maps) - 1, -1, -1):
            sm = reply.sym_maps[i]
            if sm.n_syms > 0 and sm.n_syms < 63:
                target_key = first_key_sym + i
                target_sym = sm
                break
        if target_key is None or target_sym is None:
            pytest.skip("Could not find a suitable target key")

        old_n_syms = target_sym.n_syms

        # Step 2: Fill action slots on earlier keys so our target's
        # action allocation is tightly bounded by the heap.
        fillers = []
        for i, sm in enumerate(reply.sym_maps):
            kc = first_key_sym + i
            if kc >= target_key:
                break
            if sm.n_syms > 0:
                fillers.append((kc, sm.n_syms))
        fillers = fillers[:96]

        if fillers:
            first_filler = fillers[0][0]
            last_filler = fillers[-1][0]
            filler_range = last_filler - first_filler + 1
            filler_by_key = {kc: ns for kc, ns in fillers}
            counts = bytearray()
            total_filler_acts = 0
            for kc in range(first_filler, last_filler + 1):
                ns = filler_by_key.get(kc, 0)
                counts.append(ns)
                total_filler_acts += ns
            filler_payload = bytes(counts) + b"\x00" * ((4 - len(counts) % 4) % 4)
            filler_payload += b"\x00" * (total_filler_acts * 8)

            filler_req = xkb.SetMapRequest(
                opcode=opcode,
                present=xkb.XkbKeyActionsMask,
                min_key_code=min_key,
                max_key_code=max_key,
                first_key_act=first_filler,
                n_key_acts=filler_range,
                total_acts=total_filler_acts,
                payload=filler_payload,
            )
            xclient.send_request(filler_req.to_bytes())
            time.sleep(0.05)

        # Step 3: Send the vulnerable SetMap that widens the target key
        # to 63 levels while providing old_n_syms actions.
        # The desync: CheckKeySyms writes the new width to symsPerKey[target],
        # but its second loop (starting at i=nKeySyms=1, not firstKeySym+1)
        # overwrites symsPerKey[target] with the old width from the existing
        # map. CheckKeyActions then accepts old_n_syms as valid.
        wide_levels = 63
        new_type_idx = n_types

        # New key type: 63 levels, no map entries, no preserve
        type_wire = struct.pack("<BBHBBBB", 0, 0, 0, wide_levels, 0, 0, 0)

        # Sym map for the target key using the new wide type
        sym_wire = struct.pack(
            "<BBBBBBH",
            new_type_idx,  # kt_index[0]
            0,  # kt_index[1]
            0,  # kt_index[2]
            0,  # kt_index[3]
            1,  # groupInfo (1 group)
            wide_levels,  # width
            wide_levels,  # nSyms
        )
        # Keysym data
        sym_wire += b"\x00\x00\x00\x00" * wide_levels

        # Action count: use the old width (this is the desync)
        action_counts = bytes([old_n_syms]) + b"\x00" * 3  # padded to 4
        action_data = b"\x00" * (old_n_syms * 8)

        payload = type_wire + sym_wire + action_counts + action_data

        vuln_req = xkb.SetMapRequest(
            opcode=opcode,
            present=xkb.XkbKeyTypesMask | xkb.XkbKeySymsMask | xkb.XkbKeyActionsMask,
            flags=xkb.XkbSetMapResizeTypes,
            min_key_code=min_key,
            max_key_code=max_key,
            first_type=new_type_idx,
            n_types=1,
            first_key_sym=target_key,
            n_key_syms=1,
            total_syms=wide_levels,
            first_key_act=target_key,
            n_key_acts=1,
            total_acts=old_n_syms,
            payload=payload,
        )
        xclient.send_request(vuln_req.to_bytes())
        time.sleep(0.1)

        assert xserver.is_alive, "Server crashed during SetMap - unexpected"

        # Step 4: Trigger the OOB read via GetMap for the target key's actions
        get_req = xkb.GetMapRequest(
            opcode=opcode,
            partial=xkb.XkbKeyActionsMask,
            first_key_act=target_key,
            n_key_acts=1,
        )
        xclient.send_request(get_req.to_bytes())
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - XKB SetMap width/action desync caused OOB read "
            "in XkbWriteKeyActions when XkbKeyNumActions derived action count "
            "from widened key_sym_map.width but the action array was sized to "
            "the old count (ZDI-CAN-32408)"
        )
