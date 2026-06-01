# SPDX-License-Identifier: MIT
#
# Security tests for fb (software) font rendering vulnerabilities.

import os
import tempfile
import time

import pytest

from proto import pcf, x11
from xclient import X11Error


class TestFbGlyphBlt:
    """Tests for fbPolyGlyphBlt/fbImageGlyphBlt with malicious fonts.

    The GLYPHWIDTHPIXELS and GLYPHHEIGHTPIXELS macros compute glyph
    dimensions from signed INT16 fields.  A crafted PCF font can
    produce negative results (e.g. rightSideBearing < leftSideBearing
    or descent negative enough to underflow ascent + descent).

    Negative gWidth/gHeight passes the ``if (gWidth && gHeight)``
    check (any non-zero value is truthy in C), then:
    - fbGlyph8/16/32: ``while (height--)`` with negative height
      wraps through all negative values before reaching zero,
      running for ~2 billion iterations and writing out of bounds.
    - memcpy-based paths: negative width cast to size_t wraps to
      SIZE_MAX.

    The fix changes the check to ``if (gWidth > 0 && gHeight > 0)``
    to explicitly reject negative dimensions.
    """

    @pytest.mark.asan
    @pytest.mark.parametrize(
        "max_lsb, max_rsb, max_ascent, max_descent, "
        "glyph_lsb, glyph_rsb, glyph_ascent, glyph_descent",
        [
            pytest.param(
                0,
                8,
                8,
                0,
                0,
                8,
                10,
                -20,
                id="negative_height",
            ),
            pytest.param(
                0,
                8,
                8,
                0,
                10,
                0,
                8,
                0,
                id="negative_width",
            ),
        ],
    )
    def test_negative_glyph_metrics(
        self,
        xserver,
        xclient,
        max_lsb,
        max_rsb,
        max_ascent,
        max_descent,
        glyph_lsb,
        glyph_rsb,
        glyph_ascent,
        glyph_descent,
    ):
        """
        Crafted PCF font with negative per-glyph dimensions exercising
        the fb software rendering path (Xvfb, no glamor).

        Without the fix, negative gWidth or gHeight passes the
        ``if (gWidth && gHeight)`` truthiness check in fbPolyGlyphBlt
        and fbImageGlyphBlt, leading to integer wrapping in
        downstream loops and out-of-bounds memory access.
        """
        font_data, xlfd = pcf.build_evil_pcf(
            max_lsb=max_lsb,
            max_rsb=max_rsb,
            max_ascent=max_ascent,
            max_descent=max_descent,
            glyph_lsb=glyph_lsb,
            glyph_rsb=glyph_rsb,
            glyph_ascent=glyph_ascent,
            glyph_descent=glyph_descent,
            char_code=0x21,
        )

        with tempfile.TemporaryDirectory(prefix="pyxtest-font-") as font_dir:
            font_path = os.path.join(font_dir, "evil.pcf")
            with open(font_path, "wb") as f:
                f.write(font_data)

            fonts_dir_path = os.path.join(font_dir, "fonts.dir")
            with open(fonts_dir_path, "w") as f:
                f.write("1\n")
                f.write(f"evil.pcf {xlfd}\n")

            xclient.send_request(x11.SetFontPathRequest(paths=[font_dir, "built-ins"]))
            time.sleep(0.5)
            for r in xclient.flush_responses(timeout=2.0):
                if isinstance(r, X11Error):
                    pytest.skip(f"SetFontPath failed (error {r.error_code})")

            fid = xclient.alloc_id()
            xclient.send_request(x11.OpenFontRequest(fid=fid, name=xlfd))
            time.sleep(0.5)
            for r in xclient.flush_responses(timeout=2.0):
                if isinstance(r, X11Error):
                    pytest.skip(f"OpenFont failed (error {r.error_code})")

            pix_id = xclient.alloc_id()
            xclient.send_request(
                x11.CreatePixmapRequest(
                    pid=pix_id,
                    drawable=xclient.root_window,
                    width=64,
                    height=64,
                    depth=xclient.root_depth,
                )
            )

            gc_id = xclient.alloc_id()
            xclient.send_request(
                x11.CreateGCRequest(
                    cid=gc_id,
                    drawable=pix_id,
                    values={x11.CreateGCRequest.GCFont: fid},
                )
            )
            time.sleep(0.5)
            for r in xclient.flush_responses(timeout=2.0):
                if isinstance(r, X11Error):
                    pytest.skip(f"CreatePixmap/CreateGC failed (error {r.error_code})")

            # PolyText8 exercises fbPolyGlyphBlt
            xclient.send_request(
                x11.PolyText8Request(
                    drawable=pix_id,
                    gc=gc_id,
                    x=10,
                    y=30,
                    string="!",
                )
            )
            time.sleep(0.5)

            assert xserver.is_alive, (
                "Server crashed - fb glyph rendering with negative per-glyph metrics"
            )

            # ImageText8 exercises fbImageGlyphBlt
            xclient.send_request(
                x11.ImageText8Request(
                    drawable=pix_id,
                    gc=gc_id,
                    x=10,
                    y=30,
                    string="!",
                )
            )
            time.sleep(0.5)

            assert xserver.is_alive, (
                "Server crashed - fb image glyph rendering with "
                "negative per-glyph metrics"
            )
