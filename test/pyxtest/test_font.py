# SPDX-License-Identifier: MIT
#
# Security tests for font alias handling vulnerabilities and
# tests for the font server connections option.

import os
import socket
import struct
import threading

import pytest
from proto import x11
from xclient import X11Error, X11Reply


class _FakeFontServer:
    """Minimal font server that completes the xfs protocol handshake.

    Listens on a random TCP port and speaks just enough of the xfs
    protocol for libXfont2's fserve FPE to successfully initialize.
    The handshake consists of:
    1. Read fsConnClientPrefix (8 bytes) from the client
    2. Send fsConnSetup (12 bytes) with status=AuthSuccess
    3. Send fsConnSetupAccept (12 bytes)
    4. Keep the connection open (the client may send further requests
       like SetResolution which we silently ignore)

    Note: this only works when libXfont2 is built with font server
    support (--enable-fc, the upstream default).  Some distributions
    (e.g. Debian) build libXfont2 with --disable-fc, in which case
    the fserve FPE is not registered and the entry will be rejected
    with BadValue regardless of the +fontserverconnections setting.
    """

    def __init__(self):
        self._socks = []
        self._threads = []
        self._clients = []
        self._lock = threading.Lock()

        self.port = None
        for family, bind_addr, opts in [
            (socket.AF_INET, "0.0.0.0", []),
            (socket.AF_INET6, "::", [(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)]),
        ]:
            try:
                sock = socket.socket(family, socket.SOCK_STREAM)
            except OSError:
                continue
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            for opt in opts:
                sock.setsockopt(*opt)

            bind_port = self.port if self.port is not None else 0
            try:
                sock.bind((bind_addr, bind_port))
            except OSError:
                sock.close()
                continue
            sock.listen(5)

            if self.port is None:
                self.port = sock.getsockname()[1]

            self._socks.append(sock)

        if not self._socks:
            raise RuntimeError("Could not bind fake font server to any address")

    def start(self):
        for sock in self._socks:
            t = threading.Thread(target=self._serve, args=(sock,), daemon=True)
            t.start()
            self._threads.append(t)

    def _serve(self, sock):
        while True:
            try:
                client, _ = sock.accept()
            except OSError:
                break
            with self._lock:
                self._clients.append(client)
            threading.Thread(
                target=self._handle_client, args=(client,), daemon=True
            ).start()

    def _handle_client(self, client):
        client.settimeout(10)
        try:
            data = b""
            while len(data) < 8:
                chunk = client.recv(8 - len(data))
                if not chunk:
                    return
                data += chunk

            setup = struct.pack(
                "<HHH BB HH",
                0,  # status = AuthSuccess
                2,  # major_version = FS_PROTOCOL
                0,  # minor_version = FS_PROTOCOL_MINOR
                0,  # num_alternates
                0,  # auth_index
                0,  # alternate_len
                0,  # auth_len
            )
            accept = struct.pack(
                "<IHHI",
                3,  # length in 4-byte words
                8192,  # max_request_len
                0,  # vendor_len
                1,  # release_number
            )
            client.sendall(setup + accept)

            while True:
                if not client.recv(4096):
                    break
        except OSError:
            pass
        finally:
            try:
                client.close()
            except OSError:
                pass

    def stop(self):
        for c in self._clients:
            try:
                c.close()
            except OSError:
                pass
        for s in self._socks:
            try:
                s.close()
            except OSError:
                pass
        for t in self._threads:
            t.join(timeout=2)


@pytest.fixture
def fake_font_server():
    """Start a fake font server that speaks minimal xfs protocol."""
    fs = _FakeFontServer()
    fs.start()
    yield fs
    fs.stop()


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
        xclient.flush_responses(timeout=1.0)

        assert xserver.is_alive, (
            "Server crashed - font alias stack buffer overflow (ZDI-CAN-30136)"
        )

        # Step 5: Restore original font path
        req = x11.SetFontPathRequest(paths=original_paths)
        xclient.send_request(req.to_bytes())
        xclient.flush_responses(timeout=1.0)


class TestFontServerConnections:
    """Tests for the +/-fontserverconnections option."""

    @pytest.fixture
    def fs_entry(self, fake_font_server):
        return f"tcp/localhost:{fake_font_server.port}"

    @pytest.fixture(
        params=[
            pytest.param("+fontserverconnections", id="enabled"),
            pytest.param("-fontserverconnections", id="disabled"),
            pytest.param("", id="default"),
        ]
    )
    def xserver_args(self, request, fs_entry):
        args = ["-fp", f"built-ins,{fs_entry}"]
        if request.param:
            args.insert(0, request.param)
        return args

    def test_fontserver_in_default_fp(self, xserver, xclient, fs_entry):
        """
        When a font server entry is passed via -fp on the command line,
        it should be silently dropped from the font path at startup
        when font server connections are disabled.

        Our xserver fixture will start with the fontserver path added.

        Note: when +fontserverconnections is set, the font server
        entry may still be absent if libXfont2 was built without
        font server client support (--disable-fc, as done by Debian).
        In that case no FPE recognizes the ``tcp/...`` path entry and
        it is silently dropped regardless of our setting.  We only
        assert that our guard does not *prevent* it.
        """
        enabled = "+fontserverconnections" in xserver.extra_args

        req = x11.GetFontPathRequest()
        xclient.send_request(req.to_bytes())
        resp = xclient.recv_response(timeout=3.0)
        assert isinstance(resp, X11Reply), "GetFontPath failed"
        paths = x11.GetFontPathReply.from_reply(resp.data).paths

        assert "built-ins" in paths, "built-ins should remain in the font path"

        if not enabled:
            assert fs_entry not in paths, (
                "Font server entry should have been stripped from "
                "the default font path when font server "
                "connections are disabled"
            )

        assert xserver.is_alive, "Server crashed during font path setup"

    def test_set_fontpath_with_fontserver(self, xserver, xclient, fs_entry):
        """
        SetFontPath with a font server entry should fail with BadAlloc when
        font server connections are disabled (the default or explicit
        -fontserverconnections).

        When +fontserverconnections is set it should succeed but
        where libXfont is built with --disable-fc (e.g. Debian)
        it will fail with BadValue (not BadAlloc).
        """
        enabled = "+fontserverconnections" in xserver.extra_args

        # Get the original font path first
        req = x11.GetFontPathRequest()
        xclient.send_request(req.to_bytes())
        resp = xclient.recv_response(timeout=3.0)
        assert isinstance(resp, X11Reply), "GetFontPath failed"
        original_paths = x11.GetFontPathReply.from_reply(resp.data).paths

        # Try to set font path to include a font server entry
        # alongside the existing paths so the default font
        # remains available.
        new_paths = original_paths + [fs_entry]
        req = x11.SetFontPathRequest(paths=new_paths)
        xclient.send_request(req.to_bytes())
        resp = xclient.recv_response(timeout=3.0)

        if enabled:
            # With +fontserverconnections we may get BadValue
            # if libXfont2 was built with --disable-fc. As
            # long as it's not BadAlloc we assume it worked.
            if isinstance(resp, X11Error):
                assert resp.error_code != x11.BadAlloc, (
                    "SetFontPath returned BadAlloc -- "
                    "_init_fs_handlers guard fired despite "
                    "+fontserverconnections being set"
                )
        else:
            assert isinstance(resp, X11Error), (
                "Expected SetFontPath to fail with a font server "
                f"entry when connections are disabled, got {resp}"
            )

            # Verify the font path is unchanged
            req = x11.GetFontPathRequest()
            xclient.send_request(req.to_bytes())
            resp = xclient.recv_response(timeout=3.0)
            assert isinstance(resp, X11Reply), (
                "GetFontPath failed after rejected SetFontPath"
            )
            current_paths = x11.GetFontPathReply.from_reply(resp.data).paths
            assert current_paths == original_paths, (
                "Font path was modified despite SetFontPath returning an error"
            )

        assert xserver.is_alive, "Server crashed handling font server path"
