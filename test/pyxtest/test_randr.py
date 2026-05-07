# SPDX-License-Identifier: MIT
#
# Security tests for RandR extension vulnerabilities.

import struct

import pytest

from proto import randr
from xclient import BadIDChoice, BadLength, Extension, X11Error, X11Reply


def _get_first_output(xclient, opcode):
    """Return the first RandR output ID, or skip if none available."""
    req = randr.GetScreenResourcesCurrentRequest(
        opcode=opcode,
        window=xclient.root_window,
    )
    xclient.send_request(req)
    resp = xclient.recv_response(timeout=5.0)
    if not isinstance(resp, X11Reply) or len(resp.data) < 32:
        pytest.skip("Failed to get RandR screen resources")

    n_crtcs = struct.unpack_from("<H", resp.data, 16)[0]
    n_outputs = struct.unpack_from("<H", resp.data, 18)[0]
    if n_outputs == 0:
        pytest.skip("No RandR outputs available")

    offset = 32 + n_crtcs * 4
    return struct.unpack_from("<I", resp.data, offset)[0]


@pytest.fixture
def randr_xclient(xclient):
    """Provide an xclient with RandR initialized, returning (xclient, opcode, output_id)."""
    ext = xclient.query_extension(Extension.RANDR)
    if not ext:
        pytest.skip("RANDR extension not available")

    req = randr.QueryVersionRequest(opcode=ext.opcode)
    xclient.send_request(req)
    xclient.recv_response(timeout=5.0)

    output_id = _get_first_output(xclient, ext.opcode)
    return xclient, ext.opcode, output_id


@pytest.fixture
def randr_xclient_swapped(xclient_swapped):
    """Provide a byte-swapped xclient with RandR initialized."""
    ext = xclient_swapped.query_extension(Extension.RANDR)
    if not ext:
        pytest.skip("RANDR extension not available")

    req = randr.QueryVersionRequest(opcode=ext.opcode)
    xclient_swapped.send_request(req)
    xclient_swapped.recv_response(timeout=5.0)

    return xclient_swapped, ext.opcode


class TestRandROutputProperty:
    """Tests for RRChangeOutputProperty vulnerabilities."""

    def test_prepend_property_size_and_offset(self, xserver, randr_xclient):
        """
        CVE-2023-5367 / ZDI-CAN-22153: Incorrect size and offset
        calculation when prepending to RandR output properties.

        Two bugs in RRChangeOutputProperty (copy-pasted from the XI code):
        1. new_value.size was set to ``len`` instead of ``total_len``
           (new + existing), so the property lost the old data's size.
        2. The old_data offset for PropModePrepend used
           ``prop_value->size`` instead of ``len``, placing old data at
           the wrong position and writing out of bounds.

        This test sets a property, prepends to it, and reads back the
        result.  On a fixed server the property contains all values in
        the correct order.

        Fixed in commit 541ab2ecd41d ("Xi/randr: fix handling of
        PropModeAppend/Prepend").
        """
        xclient, opcode, output_id = randr_xclient

        prop_atom = xclient.intern_atom("_TEST_RR_PREPEND")
        type_atom = xclient.intern_atom("INTEGER")

        # Step 1: Set initial property with values [10, 20, 30]
        initial_data = struct.pack("<III", 10, 20, 30)
        req = randr.ChangeOutputPropertyRequest(
            opcode=opcode,
            output=output_id,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=randr.PropModeReplace,
            data=initial_data,
        )
        xclient.send_request(req)
        xclient.flush_responses(timeout=0.5)

        # Step 2: Prepend values [1, 2]
        prepend_data = struct.pack("<II", 1, 2)
        req = randr.ChangeOutputPropertyRequest(
            opcode=opcode,
            output=output_id,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=randr.PropModePrepend,
            data=prepend_data,
        )
        xclient.send_request(req)
        xclient.flush_responses(timeout=0.5)

        assert xserver.is_alive, (
            "Server crashed - OOB write in RRChangeOutputProperty prepend (CVE-2023-5367)"
        )

        # Step 3: Read back and verify
        req = randr.GetOutputPropertyRequest(
            opcode=opcode,
            output=output_id,
            property_atom=prop_atom,
            type_atom=type_atom,
        )
        xclient.send_request(req)
        resp = xclient.recv_response(timeout=2.0)

        assert isinstance(resp, X11Reply), f"Expected a reply, got {resp}"
        num_items = struct.unpack_from("<I", resp.data, 16)[0]
        assert num_items == 5, (
            f"Expected 5 items (2 prepended + 3 original), got {num_items}"
        )

        values = struct.unpack_from(f"<{num_items}I", resp.data, 32)
        assert values == (1, 2, 10, 20, 30), (
            f"Expected (1, 2, 10, 20, 30), got {values}"
        )

    def test_change_output_property_num_items_overflow(self, xserver, randr_xclient):
        """
        CVE-2023-6478 / ZDI-CAN-22561: Integer truncation in
        ProcRRChangeOutputProperty length check.

        ``totalSize = nUnits * sizeInBytes`` was computed as a 32-bit int.
        With format=32 and nUnits=0x40000000, the multiplication overflows
        to 0, passing the REQUEST_FIXED_SIZE check.

        The fix changed totalSize from ``int`` to ``uint64_t``.

        Fixed in commit 14f480010a93 ("randr: avoid integer truncation in
        length check of ProcRRChange*Property").
        """
        xclient, opcode, output_id = randr_xclient

        prop_atom = xclient.intern_atom("_TEST_RR_OVERFLOW")
        type_atom = xclient.intern_atom("INTEGER")

        req = randr.ChangeOutputPropertyRequest(
            opcode=opcode,
            output=output_id,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=randr.PropModeReplace,
            num_items=0x40000000,
            data=b"",
        )
        xclient.send_request(req)
        resp = xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - integer truncation in RRChangeOutputProperty (CVE-2023-6478)"
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


class TestRandRSetScreenConfig:
    @pytest.mark.swapped_client
    def test_set_screen_config_config_timestamp_swapped(
        self, xserver, randr_xclient_swapped
    ):
        """
        SProcRRSetScreenConfig was missing swapl(&stuff->configTimestamp).

        First query the server's configTimestamp via GetScreenResources,
        then send it back in SetScreenConfig.  Without the fix, the
        unswapped configTimestamp won't match → the reply has
        status=RRSetConfigInvalidConfigTime (1) instead of
        RRSetConfigSuccess (0).

        Fixed in commit ac45f9b29e3a ("randr: add missing byte swapping
        for various fields").
        """
        conn, opcode = randr_xclient_swapped

        # Get screen resources to obtain the configTimestamp
        req = randr.GetScreenResourcesCurrentRequest(
            opcode=opcode,
            window=conn.root_window,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)

        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"
        assert len(resp.data) >= 32, "Reply too short"

        # xRRGetScreenResourcesReply:
        #   [8]  timestamp(4)
        #   [12] configTimestamp(4)
        config_ts = struct.unpack_from(">I", resp.data, 12)[0]

        req = randr.SetScreenConfigRequest(
            opcode=opcode,
            drawable=conn.root_window,
            timestamp=0,  # CurrentTime
            config_timestamp=config_ts,
            size_id=0,
            rotation=1,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"
        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"

        # xRRSetScreenConfigReply:
        #   [1] status
        # RRSetConfigSuccess = 0
        # RRSetConfigInvalidConfigTime = 1
        # RRSetConfigInvalidTime = 2
        # RRSetConfigFailed = 3
        #
        # Without the fix, the unswapped configTimestamp fails
        # the equality check → status 1 (RRSetConfigInvalidConfigTime).
        status = resp.data[1]
        assert status != 1, (
            "SetScreenConfig status = 1 "
            "(RRSetConfigInvalidConfigTime) - configTimestamp "
            "was not byte-swapped correctly."
        )


class TestRandRCreateLease:
    @pytest.mark.swapped_client
    def test_create_lease_lid_swapped(self, xserver, randr_xclient_swapped):
        """
        SProcRRCreateLease was missing swapl(&stuff->lid).
        Without the swap, the garbled lid fails LEGAL_NEW_RESOURCE
        → BadIDChoice error (error code 14).

        With the fix, the request should succeed far enough to reach
        the crtc/output validation (possibly returning BadValue for
        the empty lists, but NOT BadIDChoice).

        Fixed in commit ac45f9b29e3a ("randr: add missing byte swapping
        for various fields").
        """
        conn, opcode = randr_xclient_swapped

        lid = conn.alloc_id()

        # Get screen resources to find a crtc and output
        req = randr.GetScreenResourcesCurrentRequest(
            opcode=opcode,
            window=conn.root_window,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)
        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"

        n_crtcs = struct.unpack_from(">H", resp.data, 16)[0]
        n_outputs = struct.unpack_from(">H", resp.data, 18)[0]

        crtcs = []
        outputs = []
        if n_crtcs > 0:
            crtcs = [struct.unpack_from(">I", resp.data, 32)[0]]
        if n_outputs > 0:
            offset = 32 + n_crtcs * 4
            outputs = [struct.unpack_from(">I", resp.data, offset)[0]]

        req = randr.CreateLeaseRequest(
            opcode=opcode,
            window=conn.root_window,
            lid=lid,
            crtcs=crtcs,
            outputs=outputs,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"

        # Without the fix: BadIDChoice (error code 14).
        # With the fix: either success or some other error.
        if isinstance(resp, X11Error):
            assert resp.error_code != BadIDChoice, (
                "CreateLease returned BadIDChoice - lid not byte-swapped"
            )
