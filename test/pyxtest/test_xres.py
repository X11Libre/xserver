# SPDX-License-Identifier: MIT
#
# Tests for X-Resource extension.

import struct

import pytest

from proto import xres
from xclient import Extension, X11Reply


@pytest.fixture
def xres_xclient_swapped(xclient_swapped):
    """Provide a byte-swapped xclient with X-Resource initialized."""
    ext = xclient_swapped.query_extension(Extension.XRES)
    if not ext:
        pytest.skip("X-Resource extension not available")

    req = xres.QueryVersionRequest(opcode=ext.opcode)
    xclient_swapped.send_request(req.to_bytes(">"))
    resp = xclient_swapped.recv_response(timeout=5.0)
    if not isinstance(resp, X11Reply):
        pytest.skip("XRes QueryVersion failed")

    return xclient_swapped, ext.opcode


class TestXResQueryClientIds:
    @pytest.mark.swapped_client
    def test_query_client_ids_spec_entries_swapped(self, xserver, xres_xclient_swapped):
        """
        SProcXResQueryClientIds was missing byte-swaps for the
        xXResClientIdSpec entries.  Without the swaps, the mask field
        (e.g. XResClientXIDMask=1) is seen as 0x01000000, which
        doesn't match any known mask → the reply contains zero results.

        With the fix, the reply should contain at least one result.

        Fixed in commit f7b574931544 ("Xext/xres: add missing byte-swap
        of spec entries in SProcXResQueryClientIds").
        """
        conn, opcode = xres_xclient_swapped

        # X_XResClientXIDMask = 1.
        # client=0 means "all clients" (the wildcard).
        req = xres.QueryClientIdsRequest(
            opcode=opcode,
            specs=[(0, 1)],  # mask=1 (XResClientXIDMask)
        )
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"
        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"

        # xXResQueryClientIdsReply:
        #   [8] numIds(4)
        num_ids = struct.unpack_from(">I", resp.data, 8)[0]
        assert num_ids > 0, (
            "QueryClientIds returned numIds=0, expected >0. "
            "The spec entry mask was not byte-swapped, so the "
            "server didn't recognize XResClientXIDMask."
        )

    @pytest.mark.swapped_client
    def test_construct_client_id_value_swap_check(
        self, xserver, xres_xclient_swapped, xclient
    ):
        """
        ConstructClientIdValue used client->swapped instead of
        sendClient->swapped.  When a big-endian client queries
        a native client's ID, the spec.client field in the reply
        is not swapped → garbled value.

        We create an additional native (little-endian) connection.
        The big-endian connection queries the native client's XID.
        With the fix, the returned spec.client should match the
        native client's resource base.  Without the fix, it's
        byte-swapped garbage.

        Fixed in commit d2d4fb35e798 ("Xext/xres: fix wrong swap
        check").
        """
        conn, opcode = xres_xclient_swapped
        conn_native = xclient

        # Get the list of clients
        req = xres.QueryClientsRequest(opcode=opcode)
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=5.0)
        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"

        num_clients = struct.unpack_from(">I", resp.data, 8)[0]
        assert num_clients >= 2, f"Expected at least 2 clients, got {num_clients}"

        # Collect all client XIDs from the reply.
        # Each xXResClient is 8 bytes: resource_base(4) + resource_mask(4)
        client_xids = []
        for i in range(num_clients):
            offset = 32 + i * 8
            if offset + 4 <= len(resp.data):
                xid = struct.unpack_from(">I", resp.data, offset)[0]
                client_xids.append(xid)

        # Find the native client's resource base
        native_base = conn_native._resource_id_base
        target_xid = None
        for xid in client_xids:
            if (xid & 0xFFE00000) == (native_base & 0xFFE00000):
                target_xid = xid
                break

        if target_xid is None:
            pytest.skip("Could not find native client in XRes client list")

        # Query the native client's XID using XResClientXIDMask
        req = xres.QueryClientIdsRequest(
            opcode=opcode,
            specs=[(target_xid, 1)],  # XResClientXIDMask
        )
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=5.0)

        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"

        num_ids = struct.unpack_from(">I", resp.data, 8)[0]
        assert num_ids > 0, f"Expected >0 IDs, got {num_ids}"

        # xXResClientIdValue:
        #   spec.client(4) + spec.mask(4) + length(4) + value(4*length)
        spec_client = struct.unpack_from(">I", resp.data, 32)[0]

        assert spec_client == target_xid, (
            f"spec.client = {spec_client:#010x}, "
            f"expected {target_xid:#010x}. "
            f"The swap check used client->swapped instead of "
            f"sendClient->swapped."
        )
