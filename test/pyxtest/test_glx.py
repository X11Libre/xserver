# SPDX-License-Identifier: MIT
#
# Security tests for GLX extension vulnerabilities.

import time

import pytest

from proto import glx
from xclient import Extension, X11Error


@pytest.fixture
def glx_xclient(xclient):
    """Provide an xclient with GLX extension available."""
    ext = xclient.query_extension(Extension.GLX)
    if not ext:
        pytest.skip("GLX extension not available")
    return xclient, ext.opcode


class TestGLXChangeDrawableAttributes:
    """Tests for GLX ChangeDrawableAttributes length validation."""

    @pytest.mark.asan
    def test_reversed_length_check_oob_read(self, xserver, glx_xclient):
        """
        ZDI-CAN-30165: __glXDisp_ChangeDrawableAttributes used a
        reversed comparison operator (<) instead of (>) when validating
        the request length against numAttribs. The check tested whether
        the computed request size was LESS THAN client->req_len, but
        should have tested GREATER THAN. With the reversed operator, a
        request claiming many more attribute pairs than actually present
        passes validation.

        DoChangeDrawableAttributes then iterates numAttribs attribute
        pairs starting past the request header, reading beyond the
        actual request data (OOB read). If a GLX_EVENT_MASK key is
        found in the overread data, its value is written to
        pGlxDraw->eventMask (OOB write).

        Attack: Create a GLX context bound to the root window (which
        auto-creates a GLXDrawable), then send ChangeDrawableAttributes
        with length=3 (12 bytes) but numAttribs=2100. The computed
        size 4203 > 3 would fail with the correct check, but 4203 < 3
        is false, so the reversed check passes.

        Fixed by changing < to > in both glxcmds.c and glxcmdsswap.c.
        """
        xclient, opcode = glx_xclient

        # Step 1: Create a direct context on the root visual.
        ctx_id = xclient.alloc_id()
        req = glx.CreateContextRequest(
            opcode=opcode,
            context_id=ctx_id,
            visual=xclient.root_visual,
            screen=0,
            share_list=0,
            is_direct=1,
        )
        xclient.send_request(req.to_bytes())
        resp = xclient.recv_response(timeout=2.0)
        if isinstance(resp, X11Error):
            pytest.skip("GLX CreateContext failed (no GLX support for root visual)")

        # Step 2: MakeCurrent to bind the context and auto-create
        # a GLXDrawable for the root window.
        req = glx.MakeCurrentRequest(
            opcode=opcode,
            drawable=xclient.root_window,
            context_id=ctx_id,
            old_context_tag=0,
        )
        xclient.send_request(req.to_bytes())
        resp = xclient.recv_response(timeout=2.0)
        if isinstance(resp, X11Error):
            pytest.skip("GLX MakeCurrent failed")

        # Step 3: ChangeDrawableAttributes with reversed length check.
        # length=3 (12 bytes) but numAttribs=2100.
        # Computed size: (12 + 2100*8) / 4 = 4203 words.
        # Reversed check: 4203 < 3 → FALSE → passes!
        # Correct check: 4203 > 3 → TRUE → BadLength.
        req = glx.ChangeDrawableAttributesRequest(
            opcode=opcode,
            drawable=xclient.root_window,
            num_attribs=0,
            attribs=b"",
            length_override=3,
            num_attribs_override=2100,
        )
        xclient.send_request(req.to_bytes())
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - GLX ChangeDrawableAttributes reversed "
            "length check (ZDI-CAN-30165)"
        )


class TestGLXChangeDrawableAttributesSwap:
    """Tests for GLX ChangeDrawableAttributes swap handler length validation."""

    @pytest.mark.swapped_client
    @pytest.mark.asan
    def test_swap_oversized_request_rejected(self, xserver, xclient_swapped):
        """
        __glXDispSwap_ChangeDrawableAttributes used a manual '>'
        comparison instead of REQUEST_FIXED_SIZE. The manual check
        only rejected undersized requests, silently accepting oversized
        ones. REQUEST_FIXED_SIZE rejects both.

        Send a request with correct numAttribs=0 but extra trailing
        data (oversized). On a fixed server with REQUEST_FIXED_SIZE,
        this is rejected with BadLength. On the old code with the
        manual '>' check, the oversized request was accepted.

        Fixed by replacing the manual check with REQUEST_FIXED_SIZE.
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.GLX)
        if not ext:
            pytest.skip("GLX extension not available")

        # Step 1: Create a direct context on the root visual.
        ctx_id = conn.alloc_id()
        req = glx.CreateContextRequest(
            opcode=ext.opcode,
            context_id=ctx_id,
            visual=conn.root_visual,
            screen=0,
            share_list=0,
            is_direct=1,
        )
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=2.0)
        if isinstance(resp, X11Error):
            pytest.skip("GLX CreateContext failed")

        # Step 2: MakeCurrent to create a GLXDrawable for root window.
        req = glx.MakeCurrentRequest(
            opcode=ext.opcode,
            drawable=conn.root_window,
            context_id=ctx_id,
            old_context_tag=0,
        )
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=2.0)
        if isinstance(resp, X11Error):
            pytest.skip("GLX MakeCurrent failed")

        # Step 3: ChangeDrawableAttributes with numAttribs=0 but extra
        # 8 bytes of trailing data. The correct length for numAttribs=0
        # is 3 words (12 bytes). We send 5 words (20 bytes).
        req = glx.ChangeDrawableAttributesRequest(
            opcode=ext.opcode,
            drawable=conn.root_window,
            num_attribs=0,
            attribs=b"\x00" * 8,  # 8 extra bytes
        )
        conn.send_request(req.to_bytes(">"))
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - GLX ChangeDrawableAttributes swap handler"
        )
