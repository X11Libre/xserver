# SPDX-License-Identifier: MIT
#
# Security tests for XI/XI2 (XInput) extension vulnerabilities.

import struct

import pytest

from proto import xi
from xclient import BadLength, BadWindow, BadValue, Extension, X11Error, X11Reply


@pytest.fixture
def xi_xclient(xclient):
    """Provide an xclient with XI2 initialized."""
    ext = xclient.query_extension(Extension.XI)
    if not ext:
        pytest.skip("XInput extension not available")

    req = xi.XIQueryVersionRequest(opcode=ext.opcode)
    xclient.send_request(req)
    xclient.recv_response(timeout=5.0)
    return xclient


@pytest.fixture
def xi_xclient_swapped(xclient_swapped):
    """Provide a byte-swapped xclient with XI2 initialized."""
    ext = xclient_swapped.query_extension(Extension.XI)
    if not ext:
        pytest.skip("XInput extension not available")

    req = xi.XIQueryVersionRequest(opcode=ext.opcode)
    xclient_swapped.send_request(req)
    xclient_swapped.recv_response(timeout=5.0)
    return xclient_swapped


class TestXIPassiveGrab:
    """Tests for XIPassiveGrabDevice/UngrabDevice vulnerabilities."""

    def test_passive_grab_detail_above_255(self, xserver, xi_xclient):
        """
        CVE-2022-46341 / ZDI-CAN-19381: OOB write via oversized detail
        in XIPassiveGrabDevice.

        The ``detail`` field is 32 bits on the wire but the server's
        grab mask arrays are only 256 bits wide.  A detail > 255
        causes an OOB array access in the grab handling code.

        The fix pretends that details > 255 are already grabbed,
        returning a reply with XIAlreadyGrabbed status for every
        requested modifier.

        Fixed in commit 51eb63b0ee15 ("Xi: disallow passive grabs with a
        detail > 255").
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        wid = xi_xclient.create_window()

        req = xi.XIPassiveGrabDeviceRequest(
            opcode=opcode,
            grab_window=wid,
            detail=256,  # OOB: valid range is 0-255
            grab_type=xi.XIGrabtypeButton,
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - OOB in XIPassiveGrabDevice (CVE-2022-46341)"
        )
        # The server returns a normal reply with every modifier marked
        # as XIAlreadyGrabbed.
        assert isinstance(resp, X11Reply), f"Expected a reply, got {resp}"
        num_modifiers = struct.unpack_from("<H", resp.data, 8)[0]
        assert num_modifiers > 0, "Expected at least one failed modifier"
        for i in range(num_modifiers):
            status = resp.data[32 + i * 8 + 4]
            assert status == xi.XIAlreadyGrabbed, (
                f"Modifier {i}: expected XIAlreadyGrabbed ({xi.XIAlreadyGrabbed}), "
                f"got status {status}"
            )

    def test_passive_ungrab_detail_oob_write(self, xserver, xi_xclient):
        """
        CVE-2022-46341 / ZDI-CAN-19381: OOB write in
        ProcXIPassiveUngrabDevice via oversized detail value.

        The ``detail`` field is 32 bits on the wire but
        DeleteDetailFromMask() uses it to index into a 256-bit
        (8 x CARD32) mask array.  A detail > 255 writes past the end
        of the allocated mask, corrupting heap memory.

        The fix rejects detail > 255 early, returning BadValue.

        Fixed in commit 51eb63b0ee15 ("Xi: disallow passive grabs with a
        detail > 255").
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        wid = xi_xclient.create_window()

        req = xi.XIPassiveUngrabDeviceRequest(
            opcode=opcode,
            grab_window=wid,
            detail=256,  # OOB: valid range is 0-255
            grab_type=xi.XIGrabtypeButton,
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - OOB write in XIPassiveUngrabDevice (CVE-2022-46341)"
        )
        # The fix returns BadValue (error code 2)
        assert isinstance(resp, X11Error), f"Expected an error reply, got {resp}"
        assert resp.error_code == BadValue, (
            f"Expected BadValue ({BadValue}), got error code {resp.error_code}"
        )


class TestXIChangeProperty:
    """Tests for XIChangeProperty vulnerabilities."""

    def test_num_items_overflow_truncation(self, xserver, xi_xclient):
        """
        CVE-2022-46344 / ZDI-CAN-19405: Integer truncation in
        ProcXIChangeProperty length check.

        ``totalSize = num_items * (format / 8)`` was computed as a 32-bit
        int. With format=32 and num_items=0x40000000, the multiplication
        overflows to 0, passing the REQUEST_FIXED_SIZE check. The server
        would then read num_items elements from beyond the request buffer.

        The fix changed totalSize from ``int`` to ``uint64_t``.

        Fixed in commit 8f454b793e1f ("Xi: avoid integer truncation in
        length check of ProcXIChangeProperty").
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        prop_atom = xi_xclient.intern_atom("_TEST_OVERFLOW")
        type_atom = xi_xclient.intern_atom("INTEGER")

        # format=32 (4 bytes per item), num_items=0x40000000
        # totalSize as int32: 0x40000000 * 4 = 0x100000000 → truncates to 0
        # totalSize as uint64: 0x100000000 → fails REQUEST_FIXED_SIZE
        req = xi.XIChangePropertyRequest(
            opcode=opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=xi.PropModeReplace,
            num_items=0x40000000,
            data=b"",  # no actual data
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - integer truncation in XIChangeProperty (CVE-2022-46344)"
        )
        # The server should reject with BadLength (16).  Without the fix
        # the truncated totalSize (0) passes REQUEST_FIXED_SIZE and the
        # server tries to allocate 4 GB, failing with BadAlloc (11)
        # instead.
        assert isinstance(resp, X11Error), f"Expected an error, got {resp}"
        assert resp.error_code == BadLength, (
            f"Expected BadLength ({BadLength}), got error code {resp.error_code} - "
            f"integer truncation not caught by length check"
        )

    def test_prepend_property_size_and_offset(self, xserver, xi_xclient):
        """
        CVE-2023-5367 / ZDI-CAN-22153: Incorrect size and offset
        calculation when prepending to XI device properties.

        Two bugs in XIChangeDeviceProperty (and the identical RandR code):
        1. new_value.size was set to ``len`` instead of ``total_len``
           (new + existing), so the property lost the old data's size.
        2. The old_data offset for PropModePrepend used
           ``prop_value->size`` instead of ``len``, placing old data at
           the wrong position and writing out of bounds.

        This test sets a property, then prepends to it and reads back
        the result. On a fixed server, the property contains all values
        in the correct order.

        Fixed in commit 541ab2ecd41d ("Xi/randr: fix handling of
        PropModeAppend/Prepend").
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        prop_atom = xi_xclient.intern_atom("_TEST_PREPEND")
        type_atom = xi_xclient.intern_atom("INTEGER")

        # Step 1: Set initial property with values [10, 20, 30]
        initial_data = struct.pack("<III", 10, 20, 30)
        req = xi.XIChangePropertyRequest(
            opcode=opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=xi.PropModeReplace,
            data=initial_data,
        )
        xi_xclient.send_request(req)
        xi_xclient.flush_responses(timeout=0.5)

        # Step 2: Prepend values [1, 2]
        prepend_data = struct.pack("<II", 1, 2)
        req = xi.XIChangePropertyRequest(
            opcode=opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=xi.PropModePrepend,
            data=prepend_data,
        )
        xi_xclient.send_request(req)
        xi_xclient.flush_responses(timeout=0.5)

        assert xserver.is_alive, (
            "Server crashed - OOB write in XIChangeProperty prepend (CVE-2023-5367)"
        )

        # Step 3: Read back and verify
        req = xi.XIGetPropertyRequest(
            opcode=opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)

        assert isinstance(resp, X11Reply), f"Expected a reply, got {resp}"
        num_items = struct.unpack_from("<I", resp.data, 16)[0]
        assert num_items == 5, (
            f"Expected 5 items (2 prepended + 3 original), got {num_items}"
        )

        values = struct.unpack_from(f"<{num_items}I", resp.data, 32)
        assert values == (1, 2, 10, 20, 30), (
            f"Expected (1, 2, 10, 20, 30), got {values}"
        )

    @pytest.mark.swapped_client
    @pytest.mark.parametrize(
        "change_cls,get_cls",
        [
            (xi.XIChangePropertyRequest, xi.XIGetPropertyRequest),
            (xi.XChangeDevicePropertyRequest, xi.XGetDevicePropertyRequest),
        ],
        ids=["xi2", "xi1"],
    )
    def test_change_property_data_format32_swapped(
        self, xserver, xi_xclient_swapped, change_cls, get_cls
    ):
        """
        SProcXIChangeProperty and SProcXChangeDeviceProperty did not
        byte-swap the property data payload for format=32 properties.

        Set a format=32 property from a byte-swapped client with known
        values, then read it back. Without the fix, the stored values
        have the wrong byte order, causing a round-trip mismatch.

        Fixed in commit 243ef9bc2 ("Xi: Swap property data in
        SProcXChangeDeviceProperty/SProcXIChangeProperty").
        """
        conn = xi_xclient_swapped
        ext = conn.query_extension(Extension.XI)

        prop_atom = conn.intern_atom("_TEST_SWAP_FORMAT32")
        type_atom = conn.intern_atom("INTEGER")

        test_values = [0x12345678, 0xDEADBEEF, 42]
        data = b""
        for v in test_values:
            data += struct.pack(">I", v)

        req = change_cls(
            opcode=ext.opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=xi.PropModeReplace,
            data=data,
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        assert xserver.is_alive, "Server crashed during ChangeProperty"

        req = get_cls(
            opcode=ext.opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=2.0)

        assert isinstance(resp, X11Reply), f"Expected a reply, got {resp}"

        # Both XI1 and XI2 GetProperty replies share the same layout
        # for the fields we care about:
        #   bytes 16-19: num_items (CARD32)
        #   byte 20:     format   (CARD8)
        #   bytes 32+:   property data
        num_items = struct.unpack_from(">I", resp.data, 16)[0]
        fmt = resp.data[20]
        assert num_items == len(test_values), (
            f"Expected {len(test_values)} items, got {num_items}"
        )
        assert fmt == 32, f"Expected format 32, got {fmt}"

        values = struct.unpack_from(f">{num_items}I", resp.data, 32)
        assert values == tuple(test_values), (
            f"Property data round-trip failed: expected "
            f"{[hex(v) for v in test_values]}, got "
            f"{[hex(v) for v in values]} - property data not byte-swapped "
            f"in SProc handler for {change_cls.__name__}"
        )


class TestXIChangeDeviceControl:
    @pytest.mark.swapped_client
    @pytest.mark.xorg_only
    def test_change_device_control_resolution_values_swapped(
        self, xserver, xclient_swapped
    ):
        """
        SProcXChangeDeviceControl did not byte-swap the resolution
        values array for DEVICE_RESOLUTION.

        Send a ChangeDeviceControl/DEVICE_RESOLUTION with a resolution
        value that is valid in native byte order but out-of-range when
        byte-reversed (e.g. 1000 = 0x000003E8 → 0xE8030000 reversed).

        Without the fix: the garbled value exceeds max_resolution →
        BadValue error.
        With the fix: the correct value is in range → success reply
        (or BadMatch if the device doesn't support it, but not BadValue).

        Fixed in commit e24bd73e9d6f ("Xi: add missing byte-swap of
        resolution values in SProcXChangeDeviceControl").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.XI)
        if not ext:
            pytest.skip("XInput extension not available")

        req = xi.XIQueryVersionRequest(opcode=ext.opcode)
        conn.send_request(req)
        conn.recv_response(timeout=5.0)

        ctl = xi.DeviceResolutionCtl(
            first_valuator=0,
            num_valuators=1,
            resolutions=[1000],
        )
        ctl_bytes = ctl.to_bytes(">")

        req = xi.XChangeDeviceControlRequest(
            opcode=ext.opcode,
            control=xi.DEVICE_RESOLUTION,
            deviceid=xi.VirtualCorePointer,
            control_data=ctl_bytes,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=2.0)

        assert xserver.is_alive, "Server crashed"

        # Without the fix: BadValue (error code 2) because the
        # byte-reversed resolution 0xE8030000 exceeds max_resolution.
        # With the fix: either a reply (success) or BadMatch (device
        # doesn't support resolution control), but NOT BadValue.
        if isinstance(resp, X11Error):
            assert resp.error_code != BadValue, (
                "ChangeDeviceControl returned BadValue - "
                "resolution values not byte-swapped"
            )


class TestXIChangeCursor:
    def test_change_cursor_null_window(self, xserver, xi_xclient):
        """
        XIChangeCursor dereferences pWin even if it's not set
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode
        req = xi.XIChangeCursorRequest(
            opcode=opcode,
            window=0,
            cursor=0,
            deviceid=xi.VirtualCorePointer,
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"

        # Without the fix: SegFault on a NULL WindowPtr
        # With the fix: BadWindow
        assert isinstance(resp, X11Error), f"Expected an error, got {resp}"
        assert resp.error_code == BadWindow, (
            "ChangeCursor didn't return BadWindow for Window 0"
        )
