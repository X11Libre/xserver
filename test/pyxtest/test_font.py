# SPDX-License-Identifier: MIT
#
# Security tests for font alias handling vulnerabilities.

import os
import time

import pytest

from proto import x11
from xclient import X11Reply


class TestFontAliasOverflow:
    """Tests for doListFontsAndAliases stack buffer overflow via long alias."""

    @pytest.mark.asan
    def test_list_fonts_long_alias_overflow(self, xserver, xclient, tmp_path):
        """
        ZDI-CAN-30136: doListFontsAndAliases copies the resolved alias
        target from libXfont2 into tmp_pattern[] and c->current.pattern[],
        both sized XLFDMAXFONTNAMELEN. The server defined
        XLFDMAXFONTNAMELEN as 256, but libXfont2 allows alias targets up
        to MAXFONTNAMELEN (1024) bytes in fonts.alias files. A
        fonts.alias with a target name between 257 and 1023 bytes caused
        a stack buffer overflow when the alias was resolved via
        ListFonts.

        Attack:
        1. Create a font directory with fonts.alias containing an alias
           whose target name exceeds the old 256-byte buffer (but stays
           under 1024 to pass libXfont2 validation).
        2. SetFontPath to include this directory.
        3. ListFonts with a pattern matching the alias name.
        4. Server copies oversized resolved name into the undersized
           stack and struct buffers -- stack buffer overflow.

        Fixed by increasing XLFDMAXFONTNAMELEN to 1024 to match
        libXfont2's MAXFONTNAMELEN.
        """
        # The old XLFDMAXFONTNAMELEN was 256, now 1024
        # MAXFONTNAMELEN in libXfont2 is 1024
        # Use a target length > 256 but < 1024 to trigger the old bug.
        # The overflow must be large enough to clobber the saved return
        # address on the stack; 256 + 400 = 656 bytes overflows 400
        # bytes past the tmp_pattern[256] buffer which reliably reaches
        # the saved RIP and crashes the server.
        target_len = 656
        alias_name = "pwn"

        # Step 1: Create evil font directory with long alias target
        evil_dir = str(tmp_path / "evilfonts")
        os.makedirs(evil_dir)

        # fonts.dir (empty -- 0 fonts, required for FPE init)
        with open(os.path.join(evil_dir, "fonts.dir"), "w") as f:
            f.write("0\n")

        # fonts.alias with oversized target name
        # Use XLFD-like format starting with '-' so the FPE recognizes it
        long_target = "-" + "A" * (target_len - 1)
        with open(os.path.join(evil_dir, "fonts.alias"), "w") as f:
            f.write(f"{alias_name} {long_target}\n")

        # Step 2: Get current font path so we can restore it later
        req = x11.GetFontPathRequest()
        xclient.send_request(req.to_bytes())
        resp = xclient.recv_response(timeout=5.0)
        assert isinstance(resp, X11Reply), "GetFontPath failed"
        original_paths = x11.GetFontPathReply.from_reply(resp.data).paths

        # Step 3: Set font path to include evil directory first
        new_paths = [evil_dir] + original_paths
        req = x11.SetFontPathRequest(paths=new_paths)
        xclient.send_request(req.to_bytes())
        xclient.flush_responses(timeout=1.0)

        # Step 4: ListFonts with pattern matching the alias name.
        # This triggers doListFontsAndAliases which resolves the alias
        # and copies the oversized target into the stack buffer.
        req = x11.ListFontsRequest(pattern=alias_name, max_names=10)
        xclient.send_request(req.to_bytes())
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - font alias stack buffer overflow (ZDI-CAN-30136)"
        )

        # Step 5: Restore original font path
        req = x11.SetFontPathRequest(paths=original_paths)
        xclient.send_request(req.to_bytes())
        xclient.flush_responses(timeout=1.0)
