# SPDX-License-Identifier: MIT
#
# Security tests for XI/XI2 (XInput) extension vulnerabilities.

import struct

import pytest

from proto import xi
from xclient import Extension, X11Error, X11Reply


@pytest.fixture
def xi_xclient(xclient):
    """Provide an xclient with XI2 initialized."""
    ext = xclient.query_extension(Extension.XI)
    if not ext:
        pytest.skip("XInput extension not available")

    req = xi.XIQueryVersionRequest(opcode=ext.opcode)
    xclient.send_request(req.to_bytes())
    xclient.recv_response(timeout=5.0)
    return xclient


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
        xi_xclient.send_request(req.to_bytes())
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
        xi_xclient.send_request(req.to_bytes())
        resp = xi_xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - OOB write in XIPassiveUngrabDevice (CVE-2022-46341)"
        )
        # The fix returns BadValue (error code 2)
        assert isinstance(resp, X11Error), f"Expected an error reply, got {resp}"
        assert resp.error_code == 2, (
            f"Expected BadValue (2), got error code {resp.error_code}"
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
        xi_xclient.send_request(req.to_bytes())
        resp = xi_xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - integer truncation in XIChangeProperty (CVE-2022-46344)"
        )
        # The server should reject with BadLength (16).  Without the fix
        # the truncated totalSize (0) passes REQUEST_FIXED_SIZE and the
        # server tries to allocate 4 GB, failing with BadAlloc (11)
        # instead.
        assert isinstance(resp, X11Error), f"Expected an error, got {resp}"
        assert resp.error_code == 16, (
            f"Expected BadLength (16), got error code {resp.error_code} - "
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
        xi_xclient.send_request(req.to_bytes())
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
        xi_xclient.send_request(req.to_bytes())
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
        xi_xclient.send_request(req.to_bytes())
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
