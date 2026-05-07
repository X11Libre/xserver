# SPDX-License-Identifier: MIT
#
# Security tests for RECORD extension vulnerabilities.

import time

import pytest

from proto import record
from xclient import Extension


@pytest.fixture
def record_xclient_swapped(xclient_swapped):
    """Provide a byte-swapped xclient with RECORD initialized."""
    ext = xclient_swapped.query_extension(Extension.RECORD)
    if not ext:
        pytest.skip("RECORD extension not available")

    req = record.QueryVersionRequest(opcode=ext.opcode)
    xclient_swapped.send_request(req)
    xclient_swapped.recv_response(timeout=5.0)

    return xclient_swapped, ext.opcode


class TestRecordCreateContext:
    """Tests for RECORD CreateContext/RegisterClients vulnerabilities."""

    @pytest.mark.swapped_client
    @pytest.mark.asan
    def test_swap_create_register_integer_underflow(
        self, xserver, record_xclient_swapped
    ):
        """
        CVE-2020-14362 / ZDI-CAN-11574: SwapCreateRegister() used
        stuff->length (attacker-controlled 16-bit wire field) instead
        of client->req_len for bounds checking nClients.

        With big requests or a carefully crafted length field, the
        subtraction ``stuff->length - header_size`` can underflow,
        allowing nClients to pass the check and causing OOB reads
        during client ID swapping.

        Fixed in commit 2902b78535ec ("Fix XRecordRegisterClients() Integer
        underflow").
        """
        conn, opcode = record_xclient_swapped

        ctx_id = conn.alloc_id()

        # Send CreateContext claiming many clients but with
        # minimal actual data.
        req = record.CreateContextRequest(
            opcode=opcode,
            context_id=ctx_id,
            client_ids=[0],  # One client ID
            ranges_data=b"\x00" * 32,  # One range (32 bytes)
            n_clients_override=100,  # Claim 100 clients
            n_ranges_override=1,
        )
        conn.send_request(req)
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - integer underflow in SwapCreateRegister (CVE-2020-14362)"
        )
