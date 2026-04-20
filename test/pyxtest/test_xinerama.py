# SPDX-License-Identifier: MIT
#
# Tests for XINERAMA (PanoramiX / pseudoramiX) extension.

import pytest

from proto import xinerama
from xclient import Extension, X11Reply


class TestPseudoramiXGetState:
    @pytest.mark.swapped_client
    def test_xinerama_get_state_window_swapped(self, xserver, xclient_swapped):
        """
        SProcPseudoramiXGetState was missing swapl(&stuff->window).
        Without it, dixLookupWindow fails → BadWindow error.

        Fixed in commit 6c51a0f9053c ("pseudoramiX: add missing byte
        swapping in various fields").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.XINERAMA)
        if not ext:
            pytest.skip("XINERAMA extension not available")

        req = xinerama.GetStateRequest(
            opcode=ext.opcode,
            window=conn.root_window,
        )
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"
        assert isinstance(resp, X11Reply), (
            f"Expected reply from GetState, got {resp} - "
            "window field not swapped → BadWindow"
        )


class TestPseudoramiXGetScreenCount:
    @pytest.mark.swapped_client
    def test_xinerama_get_screen_count_window_swapped(self, xserver, xclient_swapped):
        """
        SProcPseudoramiXGetScreenCount was missing swapl(&stuff->window).
        Without it, dixLookupWindow fails → BadWindow error.

        Fixed in commit 6c51a0f9053c ("pseudoramiX: add missing byte
        swapping in various fields").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.XINERAMA)
        if not ext:
            pytest.skip("XINERAMA extension not available")

        req = xinerama.GetScreenCountRequest(
            opcode=ext.opcode,
            window=conn.root_window,
        )
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"
        assert isinstance(resp, X11Reply), (
            f"Expected reply from GetScreenCount, got {resp} - "
            "window field not swapped → BadWindow"
        )


class TestPseudoramiXGetScreenSize:
    @pytest.mark.swapped_client
    def test_xinerama_get_screen_size_fields_swapped(self, xserver, xclient_swapped):
        """
        SProcPseudoramiXGetScreenSize was missing swapl(&stuff->window)
        and swapl(&stuff->screen).  Without window swap,
        dixLookupWindow fails → BadWindow error.

        Fixed in commit 6c51a0f9053c ("pseudoramiX: add missing byte
        swapping in various fields").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.XINERAMA)
        if not ext:
            pytest.skip("XINERAMA extension not available")

        req = xinerama.GetScreenSizeRequest(
            opcode=ext.opcode,
            window=conn.root_window,
            screen=0,
        )
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"
        assert isinstance(resp, X11Reply), (
            f"Expected reply from GetScreenSize, got {resp} - "
            "window/screen fields not swapped"
        )
