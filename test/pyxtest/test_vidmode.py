# SPDX-License-Identifier: MIT
#
# Tests for XFree86-VidModeExtension.

import struct

import pytest

from proto import vidmode
from xclient import Extension, X11Error, X11Reply


class TestVidModeSwitchToMode:
    @pytest.mark.swapped_client
    @pytest.mark.xorg_only
    @pytest.mark.parametrize("vidmode_version", [0, 2])
    def test_switch_to_mode_fields_swapped(
        self, xserver, xclient_swapped, vidmode_version
    ):
        """
        SProcVidModeSwitchToMode previously only swapped stuff->screen.
        All mode-line fields (dotclock, hdisplay, …) were left in
        wire byte order, so ProcVidModeSwitchToMode could never match
        them against any existing mode → BadValue.

        Strategy: query the current mode with GetModeLine, then send
        the same timings back via SwitchToMode.  With the fix the
        values match → success.  Without the fix the garbled timings
        don't match anything → BadValue error.

        Fixed in commit 751e631e1c99 ("Xext/vidmode: fix
        SProcVidModeSwitchToMode swapping only screen field").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.XF86VIDMODE)
        if not ext:
            pytest.skip("XF86-VidModeExtension not available")

        req = vidmode.QueryVersionRequest(opcode=ext.opcode)
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)
        if not isinstance(resp, X11Reply):
            pytest.skip("VidMode QueryVersion failed")

        if vidmode_version >= 2:
            req = vidmode.SetClientVersionRequest(
                opcode=ext.opcode,
                major=vidmode_version,
            )
            conn.send_request(req)

        # Get the current mode line so we can echo it back.
        req = vidmode.GetModeLineRequest(
            opcode=ext.opcode,
            screen=0,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)

        if isinstance(resp, X11Error):
            pytest.skip("GetModeLine not supported (VidMode not initialised?)")
        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"

        if vidmode_version >= 2:
            # v2 xXF86VidModeGetModeLineReply (52 bytes):
            #   [8]  dotclock(4)
            #   [12] hdisplay(2)  hsyncstart(2)
            #   [16] hsyncend(2)  htotal(2)
            #   [20] hskew(2)     vdisplay(2)
            #   [24] vsyncstart(2) vsyncend(2)
            #   [28] vtotal(2)    pad2(2)
            #   [32] flags(4)
            #   [36..48] reserved(12)
            #   [48] privsize(4)
            assert len(resp.data) >= 52, f"Reply too short: {len(resp.data)}"
            dotclock = struct.unpack_from(">I", resp.data, 8)[0]
            hdisplay = struct.unpack_from(">H", resp.data, 12)[0]
            hsyncstart = struct.unpack_from(">H", resp.data, 14)[0]
            hsyncend = struct.unpack_from(">H", resp.data, 16)[0]
            htotal = struct.unpack_from(">H", resp.data, 18)[0]
            hskew = struct.unpack_from(">H", resp.data, 20)[0]
            vdisplay = struct.unpack_from(">H", resp.data, 22)[0]
            vsyncstart = struct.unpack_from(">H", resp.data, 24)[0]
            vsyncend = struct.unpack_from(">H", resp.data, 26)[0]
            vtotal = struct.unpack_from(">H", resp.data, 28)[0]
            flags = struct.unpack_from(">I", resp.data, 32)[0]
        else:
            # v0 xXF86OldVidModeGetModeLineReply (36 bytes, no hskew):
            #   [8]  dotclock(4)
            #   [12] hdisplay(2)  hsyncstart(2)
            #   [16] hsyncend(2)  htotal(2)
            #   [20] vdisplay(2)  vsyncstart(2)
            #   [24] vsyncend(2)  vtotal(2)
            #   [28] flags(4)
            #   [32] privsize(4)
            assert len(resp.data) >= 36, f"Reply too short: {len(resp.data)}"
            dotclock = struct.unpack_from(">I", resp.data, 8)[0]
            hdisplay = struct.unpack_from(">H", resp.data, 12)[0]
            hsyncstart = struct.unpack_from(">H", resp.data, 14)[0]
            hsyncend = struct.unpack_from(">H", resp.data, 16)[0]
            htotal = struct.unpack_from(">H", resp.data, 18)[0]
            vdisplay = struct.unpack_from(">H", resp.data, 20)[0]
            vsyncstart = struct.unpack_from(">H", resp.data, 22)[0]
            vsyncend = struct.unpack_from(">H", resp.data, 24)[0]
            vtotal = struct.unpack_from(">H", resp.data, 26)[0]
            flags = struct.unpack_from(">I", resp.data, 28)[0]

        # Switch to the same mode — should succeed.
        # Use the matching request format for the protocol version.
        if vidmode_version >= 2:
            req = vidmode.SwitchToModeRequest(
                opcode=ext.opcode,
                screen=0,
                dotclock=dotclock,
                hdisplay=hdisplay,
                hsyncstart=hsyncstart,
                hsyncend=hsyncend,
                htotal=htotal,
                hskew=hskew,
                vdisplay=vdisplay,
                vsyncstart=vsyncstart,
                vsyncend=vsyncend,
                vtotal=vtotal,
                flags=flags,
                privsize=0,
            )
        else:
            req = vidmode.OldSwitchToModeRequest(
                opcode=ext.opcode,
                screen=0,
                dotclock=dotclock,
                hdisplay=hdisplay,
                hsyncstart=hsyncstart,
                hsyncend=hsyncend,
                htotal=htotal,
                vdisplay=vdisplay,
                vsyncstart=vsyncstart,
                vsyncend=vsyncend,
                vtotal=vtotal,
                flags=flags,
                privsize=0,
            )
        conn.send_request(req)
        resp = conn.recv_response(timeout=2.0)

        assert xserver.is_alive, "Server crashed"
        # With the fix: no response (void request, success).
        # Without the fix: BadValue error because the garbled
        # timing fields don't match any mode.
        if resp is not None:
            assert not isinstance(resp, X11Error), (
                f"SwitchToMode returned error {resp} - "
                f"mode-line fields not byte-swapped in "
                f"SProcVidModeSwitchToMode"
            )
