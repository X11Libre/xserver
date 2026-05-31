# SPDX-License-Identifier: MIT
#
# Security tests for glamor font rendering vulnerabilities.

import os
import tempfile
import time

import pytest

from proto import pcf, x11
from xclient import X11Error


class TestGlamorFontAtlas:
    """Tests for glamor_font_get atlas overflow vulnerabilities.

    ZDI-CAN-30498: glamor_font_get() computes the atlas slot size from
    the font's declared maxbounds, but copies each glyph's bitmap
    using the per-glyph metrics (GLYPHHEIGHTPIXELS/GLYPHWIDTHBYTES).

    The PCF parser in libXfont2 does not recompute maxbounds from
    per-glyph data (only the BDF parser does), so the file's declared
    maxbounds values are trusted as-is.  A crafted PCF font can set
    per-glyph metrics that exceed or underflow maxbounds, causing
    heap buffer overflows or integer wrapping in the atlas memcpy
    loop.

    The fix rejects fonts where any per-glyph metric exceeds maxbounds
    (or is negative), falling back to software rendering.
    """

    @pytest.mark.xorg_only
    @pytest.mark.asan
    @pytest.mark.parametrize(
        "max_lsb, max_rsb, max_ascent, max_descent, "
        "glyph_lsb, glyph_rsb, glyph_ascent, glyph_descent",
        [
            pytest.param(
                0,
                4,
                4,
                0,
                0,
                100,
                100,
                0,
                id="exceeds_maxbounds",
            ),
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
    def test_malicious_glyph_metrics(
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
        Crafted PCF font with per-glyph metrics that violate the
        maxbounds invariant.  Without the fix, glamor_font_get()
        overflows the atlas buffer or wraps unsigned loop counters.

        Three variants:
        - exceeds_maxbounds: per-glyph 100x100 vs maxbounds 4x4,
          memcpy writes past the atlas buffer.
        - negative_height: ascent + descent < 0, loop counter wraps
          to ~4 billion via ``(unsigned) gh``.
        - negative_width: rsb - lsb < 0, GLYPHWIDTHBYTES is negative,
          memcpy size wraps to SIZE_MAX.

        Fixed in commit bb9b4f2333 ("glamor: clamp per-glyph metrics
        to maxbounds in font atlas").
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

            # Prepend our malicious font directory to the font path,
            # keeping built-ins so the server can still find cursor
            # fonts etc.
            xclient.send_request(x11.SetFontPathRequest(paths=[font_dir, "built-ins"]))
            time.sleep(0.5)
            for r in xclient.flush_responses(timeout=2.0):
                if isinstance(r, X11Error):
                    pytest.skip(f"SetFontPath failed (error {r.error_code})")

            # Open the malicious font
            fid = xclient.alloc_id()
            xclient.send_request(x11.OpenFontRequest(fid=fid, name=xlfd))
            time.sleep(0.5)
            for r in xclient.flush_responses(timeout=2.0):
                if isinstance(r, X11Error):
                    pytest.skip(f"OpenFont failed (error {r.error_code})")

            # Create a pixmap on the default screen (glamor-backed).
            # Drawing on a pixmap ensures the glamor acceleration path
            # is used rather than a potential fallback.
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

            # Create a GC with the malicious font
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

            # Draw text using the malicious font on the pixmap.
            # This triggers glamor_font_get() which builds the font
            # atlas.  Character '!' is at code point 0x21 which
            # matches our font's single encoded glyph.
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
                "Server crashed - glamor_font_get heap corruption "
                "due to malicious per-glyph metrics (ZDI-CAN-30498)"
            )
