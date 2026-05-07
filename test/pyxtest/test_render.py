# SPDX-License-Identifier: MIT
#
# Security tests for Render extension vulnerabilities.

import struct

import pytest

from proto import render
from xclient import Extension, X11Error, X11Reply


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
        # xRenderQueryPictFormatsReply header (32 bytes):
        #   [8]  numFormats(4)
        # followed by numFormats xPictFormInfo entries (28 bytes each):
        #   [0] id(4)  [4] type(1)  [5] depth(1)  ...
        req = render.QueryPictFormatsRequest(opcode=opcode)
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)

        assert isinstance(resp, X11Reply), "QueryPictFormats failed"
        num_formats = struct.unpack_from(">I", resp.data, 8)[0]

        format_id = 0
        for i in range(num_formats):
            off = 32 + i * 28
            if off + 6 > len(resp.data):
                break
            fid = struct.unpack_from(">I", resp.data, off)[0]
            fdepth = resp.data[off + 5]
            if fdepth == conn.root_depth:
                format_id = fid
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
