# SPDX-License-Identifier: MIT
#
# Security tests for MIT-SCREEN-SAVER extension vulnerabilities.

import time

import pytest

from proto import screensaver
from xclient import Extension


class TestScreenSaverSuspend:
    """Tests for SProcScreenSaverSuspend vulnerabilities."""

    @pytest.mark.swapped_client
    @pytest.mark.asan
    def test_suspend_swap_before_size_check(self, xserver, xclient_swapped):
        """
        CVE-2021-4010 / ZDI-CAN-14951: SProcScreenSaverSuspend() did
        swapl() on stuff->suspend before REQUEST_SIZE_MATCH, so a
        short request triggered an OOB write during the swap.

        The fix moved REQUEST_SIZE_MATCH before the swapl.

        Fixed in commit 6c4c53010772 ("Xext: Fix out of bounds access
        in SProcScreenSaverSuspend()").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.MIT_SCREEN_SAVER)
        if not ext:
            pytest.skip("MIT-SCREEN-SAVER extension not available")

        # Send a valid ScreenSaverSuspend (the fix ensures proper
        # validation order: size check before swap).
        req = screensaver.SuspendRequest(
            opcode=ext.opcode,
            suspend=1,
        )
        conn.send_request(req.to_bytes(">"))
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - SProcScreenSaverSuspend (CVE-2021-4010)"
        )
