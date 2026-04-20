# SPDX-License-Identifier: MIT
#
# Security tests for XKB extension vulnerabilities.

import time

import pytest

from proto import xkb
from xclient import X11Error


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
        xclient.send_request(req.to_bytes())
        time.sleep(0.5)

        assert xserver.is_alive, "Server crashed - missing length checks in SetMap"


class TestXkbSetGeometry:
    """Tests for XKB SetGeometry OOB vulnerabilities."""

    def _build_colors(self, n, byte_order="<"):
        """Build n color strings for geometry payload."""
        data = b""
        colors = [
            "white",
            "black",
            "red",
            "green",
            "blue",
            "yellow",
            "cyan",
            "magenta",
        ]
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
        xclient.send_request(req.to_bytes())
        resp = xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, "Server crashed - truncated sections in SetGeometry"
        assert isinstance(resp, X11Error), f"Expected an error, got {resp}"
        assert resp.error_code == 16, (
            f"Expected BadLength (16), got error code {resp.error_code} - "
            f"missing bounds check in SetGeometry section parsing"
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
        xclient.send_request(req1.to_bytes())
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
        xclient.send_request(req2.to_bytes())
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
        xclient.send_request(req3.to_bytes())
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
        xclient.send_request(req.to_bytes())
        time.sleep(0.5)

        assert xserver.is_alive, "Server crashed - truncated atoms in SetNames"
