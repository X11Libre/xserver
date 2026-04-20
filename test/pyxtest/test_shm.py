# SPDX-License-Identifier: MIT
#
# Tests for MIT-SHM extension.

import struct

import pytest

from proto import shm
from xclient import Extension, X11Reply


class TestShmCreateSegment:
    @pytest.mark.swapped_client
    def test_create_segment_reply_sequence_swapped(self, xserver, xclient_swapped):
        """
        ProcShmCreateSegment was missing swaps(&rep.sequenceNumber) and
        swapl(&rep.length) for byte-swapped clients.

        Without the fix the sequence number in the reply is
        byte-reversed.  We send the request and verify that the
        reply's sequence number matches the expected value.

        ShmCreateSegment requires SHM fd-passing (SHM 1.2+) and a
        local connection (Unix socket).  The reply also carries an fd
        via SCM_RIGHTS which our simple recv() silently drops — that's
        fine, we only need to check the header fields.

        Fixed in commit c49c150dcfb7 ("Xext/shm: add missing reply
        byte-swap in ProcShmCreateSegment").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.MIT_SHM)
        if not ext:
            pytest.skip("MIT-SHM extension not available")

        # Query SHM version
        req = shm.QueryVersionRequest(opcode=ext.opcode)
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=5.0)

        if not isinstance(resp, X11Reply):
            pytest.skip("SHM QueryVersion failed")

        # Check for SHM 1.2+ (fd-passing / CreateSegment support)
        # Reply: type(1) + sharedPixmaps(1) + seq(2) + length(4) +
        #        majorVersion(2) + minorVersion(2) + ...
        if len(resp.data) < 12:
            pytest.skip("SHM QueryVersion reply too short")

        major = struct.unpack_from(">H", resp.data, 8)[0]
        minor = struct.unpack_from(">H", resp.data, 10)[0]
        if major < 1 or (major == 1 and minor < 2):
            pytest.skip(f"SHM {major}.{minor} does not support CreateSegment")

        shmseg_id = conn.alloc_id()
        seq = conn.send_request(
            shm.CreateSegmentRequest(
                opcode=ext.opcode,
                shmseg=shmseg_id,
                size=4096,
                read_only=False,
            ).to_bytes(">")
        )

        # Read the 32-byte reply header.  The fd arrives as
        # ancillary data which plain recv() drops, but the data
        # bytes still arrive on the stream.
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"
        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"

        # Verify the sequence number was swapped correctly.
        reply_seq = struct.unpack_from(">H", resp.data, 2)[0]
        assert reply_seq == (seq & 0xFFFF), (
            f"Reply sequence = {reply_seq:#06x}, "
            f"expected {seq & 0xFFFF:#06x}. "
            f"sequenceNumber not byte-swapped in "
            f"ProcShmCreateSegment reply."
        )
