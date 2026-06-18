# SPDX-License-Identifier: MIT
#
# Security tests for Render extension vulnerabilities.

import time

import pytest
from proto import render
from xclient import Extension, X11Error, X11Reply


@pytest.fixture
def render_xclient(xclient):
    """Provide an xclient with Render initialized."""
    ext = xclient.query_extension(Extension.RENDER)
    if not ext:
        pytest.skip("RENDER extension not available")

    req = render.QueryVersionRequest(opcode=ext.opcode)
    xclient.send_request(req)
    xclient.recv_response(timeout=5.0)

    return xclient, ext.opcode


@pytest.fixture
def render_xclient_swapped(xclient_swapped):
    """Provide a byte-swapped xclient with Render initialized."""
    ext = xclient_swapped.query_extension(Extension.RENDER)
    if not ext:
        pytest.skip("RENDER extension not available")

    req = render.QueryVersionRequest(opcode=ext.opcode)
    xclient_swapped.send_request(req)
    xclient_swapped.recv_response(timeout=5.0)

    return xclient_swapped, ext.opcode


def _find_a8_format(conn, opcode, byte_order="<"):
    """Find the PictFormat ID for A8 (depth=8, type=Direct, alpha=0/0xff).

    Returns the format ID or None if not found.
    """
    req = render.QueryPictFormatsRequest(opcode=opcode)
    conn.send_request(req)
    resp = conn.recv_response(timeout=5.0)

    assert isinstance(resp, X11Reply), "QueryPictFormats failed"
    reply = render.QueryPictFormatsReply.from_reply(resp.data, byte_order)

    for fmt in reply.formats:
        # A8: type=Direct(1), depth=8, alpha=0, alpha_mask=0xff
        if (
            fmt.type == 1
            and fmt.depth == 8
            and fmt.alpha == 0
            and fmt.alpha_mask == 0xFF
        ):
            return fmt.id

    return None


class TestRenderGlyphs:
    """Tests for Render glyph vulnerabilities."""

    def test_duplicate_glyphs_use_after_free(self, xserver, render_xclient):
        """
        Issue #1881: Use-after-free caused by duplicate glyphs in one
        glyphset.

        Two glyphsets GS1 and GS2 share the same A8 format. GS1 has one
        unique glyph U; GS2 has two glyphs A and B with identical pixel
        data (same sha1 hash). When GS2 is registered, AddGlyph inserts
        A into globalGlyphs, then reuses A for B (same hash). FreeGlyph
        on B erroneously removes A's entry from globalGlyphs and
        decrements tableEntries. This off-by-one means freeing GS1
        (which frees U and drops tableEntries to zero) causes
        globalGlyphs for that depth to be freed. Subsequent freeing of
        GS2 (which frees A) accesses the freed globalGlyphs and crashes.
        """
        conn, opcode = render_xclient

        a8_format = _find_a8_format(conn, opcode)
        if a8_format is None:
            pytest.skip("No A8 PictFormat available")

        # Create GS1 with one unique glyph (glyph ID 100)
        gs1 = conn.alloc_id()
        req = render.CreateGlyphSetRequest(
            opcode=opcode, glyph_set_id=gs1, format_id=a8_format
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # 4x1 A8 glyph: 4 bytes of pixel data, unique content
        glyph_info = (4, 1, 0, 0, 4, 0)  # width, height, x, y, xOff, yOff
        unique_data = bytes([0x11, 0x22, 0x33, 0x44])

        req = render.AddGlyphsRequest(
            opcode=opcode,
            glyph_set_id=gs1,
            glyphs=[(100, glyph_info, unique_data)],
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # Create GS2 with two glyphs (IDs 200, 201) that have identical data
        gs2 = conn.alloc_id()
        req = render.CreateGlyphSetRequest(
            opcode=opcode, glyph_set_id=gs2, format_id=a8_format
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        dup_data = bytes([0xAA, 0xBB, 0xCC, 0xDD])
        req = render.AddGlyphsRequest(
            opcode=opcode,
            glyph_set_id=gs2,
            glyphs=[
                (200, glyph_info, dup_data),
                (201, glyph_info, dup_data),
            ],
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # Free GS1 first (drops U, decrements tableEntries to zero
        # before the fix → frees globalGlyphs[depth])
        req = render.FreeGlyphSetRequest(opcode=opcode, glyph_set_id=gs1)
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # Free GS2 (frees A, accesses freed globalGlyphs → crash
        # before the fix)
        req = render.FreeGlyphSetRequest(opcode=opcode, glyph_set_id=gs2)
        conn.send_request(req)

        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - use-after-free from duplicate glyphs "
            "in globalGlyphs (issue #1881)"
        )


class TestRenderSetPictureFilter:
    @pytest.mark.swapped_client
    def test_set_picture_filter_convolution_params_swapped(
        self, xserver, render_xclient_swapped
    ):
        """
        SProcRenderSetPictureFilter was missing SwapLongs() for the
        xFixed filter parameter values.

        Set a 3x1 convolution filter with params [3.0, 1.0, 1.0, 1.0, 1.0]
        (in xFixed: [0x00030000, 0x00010000, ...]).  Without the swap,
        the server sees garbled width/height and rejects with BadMatch.
        With the swap, it succeeds.

        Fixed in commit c98273d0bc00 ("render: add missing byte-swap of
        filter params in SProcRenderSetPictureFilter").
        """
        conn, opcode = render_xclient_swapped

        # Get a PictFormat that matches the root depth.
        req = render.QueryPictFormatsRequest(opcode=opcode)
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)

        assert isinstance(resp, X11Reply), "QueryPictFormats failed"
        reply = render.QueryPictFormatsReply.from_reply(resp.data, ">")

        format_id = 0
        for fmt in reply.formats:
            if fmt.depth == conn.root_depth:
                format_id = fmt.id
                break

        if format_id == 0:
            pytest.skip("No PictFormat matching root depth")

        pix = conn.create_pixmap(width=10, height=10)
        pic = conn.alloc_id()

        req = render.CreatePictureRequest(
            opcode=opcode,
            picture_id=pic,
            drawable=pix,
            format_id=format_id,
        )
        conn.send_request(req)
        errors = conn.flush_responses(timeout=0.5)
        create_errors = [r for r in errors if isinstance(r, X11Error)]
        assert len(create_errors) == 0, f"CreatePicture failed: {create_errors}"

        # Set convolution filter: 3x1 kernel with all weights = 1.0
        # xFixed 3.0 = 0x00030000, xFixed 1.0 = 0x00010000
        # params: [width=3, height=1, k0=1.0, k1=1.0, k2=1.0]
        req = render.SetPictureFilterRequest(
            opcode=opcode,
            picture=pic,
            filter_name="convolution",
            params=[
                0x00030000,
                0x00010000,
                0x00010000,
                0x00010000,
                0x00010000,
            ],
        )
        conn.send_request(req)
        responses = conn.flush_responses(timeout=1.0)

        assert xserver.is_alive, "Server crashed"

        # With the fix: no error (filter set successfully).
        # Without the fix: BadMatch because
        # convolutionFilterValidateParams rejects the garbled params.
        errors = [r for r in responses if isinstance(r, X11Error)]
        assert len(errors) == 0, (
            f"SetPictureFilter returned error(s): {errors} - "
            "filter params not byte-swapped → BadMatch"
        )
