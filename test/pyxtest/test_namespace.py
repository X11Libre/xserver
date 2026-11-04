# SPDX-License-Identifier: MIT
#
# Tests for the Xnamespace extension: cross-namespace isolation, window-tree
# projection, root properties, background input isolation, clipboard modes,
# and system tray docking.
#
# Physical input is injected by the superPower root namespace through XTEST.
# XTEST events are delivered like device events (no SendEvent bit), so they
# exercise the same focus/paste ticket paths as real hardware.
#
# Not covered here: cgroup-based namespace assignment (needs a client in a
# separate cgroup) and the xorg.conf NamespaceConfig option (Xorg only).

import select
import struct
import time

import pytest

from proto.x11 import BadAccess
from xclient import Extension, RawX11Connection, X11Error, X11Reply

COOKIE_ROOT = bytes.fromhex("11" * 16)
COOKIE_SBA = bytes.fromhex("22" * 16)
COOKIE_SBB = bytes.fromhex("33" * 16)
COOKIE_NOMOUSE = bytes.fromhex("44" * 16)

XNS_CONFIG = """\
namespace root
superpower
auth MIT-MAGIC-COOKIE-1 11111111111111111111111111111111

namespace sb_a
allow mouse-motion shape transparency xinput xkeyboard clipboard-auto
auth MIT-MAGIC-COOKIE-1 22222222222222222222222222222222

namespace sb_b
allow mouse-motion shape transparency xinput xkeyboard clipboard
auth MIT-MAGIC-COOKIE-1 33333333333333333333333333333333

namespace sb_nomouse
allow xinput xkeyboard
auth MIT-MAGIC-COOKIE-1 44444444444444444444444444444444
"""

# Core protocol constants
XA_PRIMARY = 1
XA_ATOM = 4
XA_STRING = 31
XA_WINDOW = 33
XA_CUT_BUFFER0 = 9
KeyPress, KeyRelease, ButtonPress, ButtonRelease, MotionNotify = 2, 3, 4, 5, 6
PropertyNotify, SelectionRequest, SelectionNotify, ClientMessage = 28, 30, 31, 33
GenericEvent = 35
ShiftMask = 0x01
GrabSuccess, AlreadyGrabbed = 0, 1
KEY_BUTTON_MASK = 0x0F  # KeyPress | KeyRelease | ButtonPress | ButtonRelease
PropertyChangeMask = 0x00400000

XK_Shift_L, XK_Control_L, XK_Caps_Lock = 0xFFE1, 0xFFE3, 0xFFE5
XK_Insert, XK_v = 0xFF63, 0x0076

# Extension minor opcodes
X_XTestFakeInput = 2
X_QueryDeviceState = 30
X_XIQueryPointer = 40
X_XISelectEvents = 46
X_XIQueryVersion = 47
XI_RawMotion = 17
XIAllDevices = 0
VCP, VCK = 2, 3
X_kbGetState, X_kbGetIndicatorState, X_kbSelectEvents = 4, 12, 1
XkbUseCoreKbd, XkbStateNotify, XkbStateNotifyMask = 0x0100, 2, 1 << 2
X_XnsSetNamespaceFlags = 5
XNS_CAPABILITY_CLIPBOARD = 1 << 6

FOCUS_TICKET_MS = 1000


class NsConn(RawX11Connection):
    """RawX11Connection that keeps events (including XGE) aside from replies."""

    def __init__(self, xserver, cookie: bytes, swapped: bool = False):
        self.events: list[bytes] = []
        super().__init__(
            xserver.display_num,
            swapped=swapped,
            auth_proto=b"MIT-MAGIC-COOKIE-1",
            auth_data=cookie,
        )

    @property
    def bo(self) -> str:
        return self._byte_order

    def _read_one(self, timeout: float) -> bytes | None:
        if not select.select([self.sock], [], [], timeout)[0]:
            return None
        data = self._recv_exact(32, timeout=timeout)
        if data[0] == 1 or (data[0] & 0x7F) == GenericEvent:
            extra = struct.unpack_from(f"{self.bo}I", data, 4)[0]
            if extra:
                data += self._recv_exact(extra * 4, timeout=timeout)
        return data

    def recv_response(self, timeout: float = 5.0) -> X11Error | X11Reply | None:
        """Next reply or error; events seen on the way are queued."""
        while True:
            data = self._read_one(timeout)
            if data is None:
                return None
            if data[0] == 0:
                return X11Error.from_data(data, self.bo)
            if data[0] == 1:
                return X11Reply.from_data(data, self.bo)
            self.events.append(data)

    def reply(self, req: bytes) -> X11Error | X11Reply | None:
        self.send_request(req)
        return self.recv_response()

    def check(self) -> list[X11Error]:
        """Round-trip; return errors caused by the requests sent so far."""
        seq = self.send_request(struct.pack(f"{self.bo}BxH", 43, 1))  # GetInputFocus
        errors = []
        while True:
            resp = self.recv_response()
            assert resp is not None, "server did not answer GetInputFocus"
            if isinstance(resp, X11Error):
                errors.append(resp)
            elif resp.sequence == seq & 0xFFFF:
                return errors

    def take_events(self, kind: int | None = None) -> list[bytes]:
        self.check()
        events, self.events = self.events, []
        if kind is None:
            return events
        return [e for e in events if (e[0] & 0x7F) == kind]

    # --- core requests ---

    def change_attributes_event_mask(self, wid: int, mask: int) -> None:
        self.send_request(struct.pack(f"{self.bo}BxHIII", 2, 4, wid, 0x0800, mask))

    def map_window(self, wid: int) -> None:
        self.send_request(struct.pack(f"{self.bo}BxHI", 8, 2, wid))

    def reparent_window(self, wid: int, parent: int, x: int, y: int) -> None:
        self.send_request(struct.pack(f"{self.bo}BxHIIhh", 7, 4, wid, parent, x, y))

    def get_geometry(self, drawable: int):
        return self.reply(struct.pack(f"{self.bo}BxHI", 14, 2, drawable))

    def get_window_attributes(self, wid: int):
        return self.reply(struct.pack(f"{self.bo}BxHI", 3, 2, wid))

    def query_tree(self, wid: int):
        resp = self.reply(struct.pack(f"{self.bo}BxHI", 15, 2, wid))
        assert isinstance(resp, X11Reply), f"QueryTree failed: {resp}"
        _, parent, n = struct.unpack_from(f"{self.bo}IIH", resp.data, 8)
        children = list(struct.unpack_from(f"{self.bo}{n}I", resp.data, 32))
        return parent, children

    def translate_coords(self, src: int, dst: int, x: int, y: int):
        resp = self.reply(
            struct.pack(f"{self.bo}BxHIIhh", 40, 4, src, dst, x, y)
        )
        assert isinstance(resp, X11Reply), f"TranslateCoords failed: {resp}"
        return struct.unpack_from(f"{self.bo}Ihh", resp.data, 8)

    def change_property(self, wid, prop, ptype, fmt, data: bytes, mode: int = 0) -> None:
        pad = b"\x00" * ((4 - len(data) % 4) % 4)
        self.send_request(
            struct.pack(
                f"{self.bo}BBHIIIBxxxI",
                18, mode, (24 + len(data) + len(pad)) // 4,
                wid, prop, ptype, fmt, len(data) // (fmt // 8),
            )
            + data
            + pad
        )

    def delete_property(self, wid: int, prop: int) -> None:
        self.send_request(struct.pack(f"{self.bo}BxHII", 19, 3, wid, prop))

    def list_properties(self, wid: int) -> list[int]:
        resp = self.reply(struct.pack(f"{self.bo}BxHI", 21, 2, wid))
        assert isinstance(resp, X11Reply), f"ListProperties failed: {resp}"
        n = struct.unpack_from(f"{self.bo}H", resp.data, 8)[0]
        return list(struct.unpack_from(f"{self.bo}{n}I", resp.data, 32))

    def get_property(self, wid: int, prop: int, delete: int = 0, long_length: int = 64) -> bytes:
        resp = self.reply(
            struct.pack(f"{self.bo}BBHIIIII", 20, delete, 6, wid, prop, 0, 0, long_length)
        )
        assert isinstance(resp, X11Reply), f"GetProperty failed: {resp}"
        fmt = resp.data[1]
        nitems = struct.unpack_from(f"{self.bo}I", resp.data, 16)[0]
        return resp.data[32 : 32 + nitems * max(fmt // 8, 1)]

    def set_selection_owner(self, owner: int, selection: int) -> None:
        self.send_request(struct.pack(f"{self.bo}BxHIII", 22, 4, owner, selection, 0))

    def get_selection_owner(self, selection: int) -> int:
        resp = self.reply(struct.pack(f"{self.bo}BxHI", 23, 2, selection))
        assert isinstance(resp, X11Reply), f"GetSelectionOwner failed: {resp}"
        return struct.unpack_from(f"{self.bo}I", resp.data, 8)[0]

    def convert_selection(self, requestor, selection, target, prop) -> None:
        self.send_request(
            struct.pack(f"{self.bo}BxHIIIII", 24, 6, requestor, selection, target, prop, 0)
        )

    def send_event(self, dest: int, event: bytes) -> None:
        event = event + b"\x00" * (32 - len(event))
        self.send_request(struct.pack(f"{self.bo}BBHII", 25, 0, 11, dest, 0) + event)

    def grab_pointer(self, wid: int) -> int:
        resp = self.reply(
            struct.pack(f"{self.bo}BBHIHBBIII", 26, 0, 6, wid, 0x0004, 1, 1, 0, 0, 0)
        )
        assert isinstance(resp, X11Reply), f"GrabPointer failed: {resp}"
        return resp.data[1]

    def ungrab_pointer(self) -> None:
        self.send_request(struct.pack(f"{self.bo}BxHI", 27, 2, 0))

    def grab_keyboard(self, wid: int) -> int:
        resp = self.reply(
            struct.pack(f"{self.bo}BBHIIBBxx", 31, 0, 4, wid, 0, 1, 1)
        )
        assert isinstance(resp, X11Reply), f"GrabKeyboard failed: {resp}"
        return resp.data[1]

    def ungrab_keyboard(self) -> None:
        self.send_request(struct.pack(f"{self.bo}BxHI", 32, 2, 0))

    def query_pointer(self, wid: int):
        """Return (child, root_x, root_y, win_x, win_y, mask)."""
        resp = self.reply(struct.pack(f"{self.bo}BxHI", 38, 2, wid))
        assert isinstance(resp, X11Reply), f"QueryPointer failed: {resp}"
        return struct.unpack_from(f"{self.bo}IhhhhH", resp.data, 12)

    def query_keymap(self) -> bytes:
        resp = self.reply(struct.pack(f"{self.bo}BxH", 44, 1))
        assert isinstance(resp, X11Reply), f"QueryKeymap failed: {resp}"
        return resp.data[8:40]

    def keycode(self, keysym: int) -> int:
        resp = self.reply(struct.pack(f"{self.bo}BxHBBxx", 101, 2, 8, 248))
        assert isinstance(resp, X11Reply), f"GetKeyboardMapping failed: {resp}"
        per = resp.data[1]
        syms = struct.unpack_from(f"{self.bo}{248 * per}I", resp.data, 32)
        for i in range(248):
            if keysym in syms[i * per : (i + 1) * per]:
                return 8 + i
        pytest.skip(f"keysym {keysym:#x} not in keymap")

    # --- extensions ---

    def ext(self, name: str) -> int:
        e = self.query_extension(name)
        assert e is not None, f"{name} not available"
        return e.opcode

    def xi_init(self) -> int:
        op = self.ext(Extension.XI)
        resp = self.reply(struct.pack(f"{self.bo}BBHHH", op, X_XIQueryVersion, 2, 2, 4))
        assert isinstance(resp, X11Reply)
        return op

    def xi_query_pointer(self, wid: int):
        """Return (child, root_x, root_y, win_x, win_y) in whole pixels."""
        op = self.xi_init()
        resp = self.reply(
            struct.pack(f"{self.bo}BBHIHxx", op, X_XIQueryPointer, 3, wid, VCP)
        )
        assert isinstance(resp, X11Reply), f"XIQueryPointer failed: {resp}"
        child, rx, ry, wx, wy = struct.unpack_from(f"{self.bo}Iiiii", resp.data, 12)
        return child, rx >> 16, ry >> 16, wx >> 16, wy >> 16

    def query_device_state_keys(self, device: int) -> bytes:
        """Return the 32-byte key bitmap from XQueryDeviceState."""
        op = self.ext(Extension.XI)
        resp = self.reply(struct.pack(f"{self.bo}BBHBxxx", op, X_QueryDeviceState, 2, device))
        assert isinstance(resp, X11Reply), f"QueryDeviceState failed: {resp}"
        assert resp.data[32] == 0, "first class is not KeyClass"
        return resp.data[36:68]

    def xkb_get_state(self) -> tuple[int, int]:
        """Return (baseMods, lockedMods)."""
        op = self.xkb_use_extension()
        resp = self.reply(struct.pack(f"{self.bo}BBHHxx", op, X_kbGetState, 2, XkbUseCoreKbd))
        assert isinstance(resp, X11Reply), f"XkbGetState failed: {resp}"
        return resp.data[9], resp.data[11]

    def xkb_get_indicator_state(self) -> int:
        op = self.xkb_use_extension()
        resp = self.reply(
            struct.pack(f"{self.bo}BBHHxx", op, X_kbGetIndicatorState, 2, XkbUseCoreKbd)
        )
        assert isinstance(resp, X11Reply), f"XkbGetIndicatorState failed: {resp}"
        return struct.unpack_from(f"{self.bo}I", resp.data, 8)[0]


class Desk:
    """Root namespace (acting as WM) plus one mapped top-level per sandbox."""

    def __init__(self, xserver):
        self.root = NsConn(xserver, COOKIE_ROOT)
        self.a = NsConn(xserver, COOKIE_SBA)
        self.b = NsConn(xserver, COOKIE_SBB)
        self.nom = NsConn(xserver, COOKIE_NOMOUSE)
        self.root_id = self.root.root_window
        self.xtest = self.root.ext(Extension.XTEST)
        self.w_root = self.root.create_window(x=0, y=0, width=200, height=200)
        self.w_a = self.a.create_window(width=200, height=200)
        self.w_b = self.b.create_window(width=200, height=200)
        self.w_nom = self.nom.create_window(width=200, height=200)
        for conn, w in ((self.a, self.w_a), (self.b, self.w_b), (self.nom, self.w_nom)):
            conn.change_attributes_event_mask(w, KEY_BUTTON_MASK)
            assert conn.check() == []
        for x, w in ((300, self.w_a), (600, self.w_b), (900, self.w_nom)):
            self.root.reparent_window(w, self.root_id, x, 0)
        for w in (self.w_root, self.w_a, self.w_b, self.w_nom):
            self.root.map_window(w)
        self.focus(self.w_root)

    def close(self):
        for c in (self.root, self.a, self.b, self.nom):
            c.close()

    def all(self):
        return (self.root, self.a, self.b, self.nom)

    def sync_all(self):
        for c in self.all():
            c.check()

    def focus(self, wid: int) -> None:
        self.root.send_request(struct.pack("<BBHII", 42, 1, 3, wid, 0))
        assert self.root.check() == []
        self.sync_all()

    def fake(self, ev_type: int, detail: int = 0, x: int = 0, y: int = 0) -> None:
        self.root.send_request(
            struct.pack(
                "<BBHBBHIIIIhhIHBB",
                self.xtest, X_XTestFakeInput, 9, ev_type, detail, 0,
                0, self.root_id, 0, 0, x, y, 0, 0, 0, 0,
            )
        )
        assert self.root.check() == []
        self.sync_all()

    def key_down(self, keysym: int) -> None:
        self.fake(KeyPress, self.root.keycode(keysym))

    def key_up(self, keysym: int) -> None:
        self.fake(KeyRelease, self.root.keycode(keysym))

    def chord(self, mod: int, key: int) -> None:
        self.key_down(mod)
        self.key_down(key)
        self.key_up(key)
        self.key_up(mod)

    def move(self, x: int, y: int) -> None:
        self.fake(MotionNotify, 0, x, y)

    def click(self, button: int, x: int, y: int) -> None:
        self.move(x, y)
        self.fake(ButtonPress, button)
        self.fake(ButtonRelease, button)


@pytest.fixture
def xserver_args(tmp_path):
    cfg = tmp_path / "xnamespace.conf"
    cfg.write_text(XNS_CONFIG)
    return ["-namespace", str(cfg)]


@pytest.fixture
def desk(xserver):
    d = Desk(xserver)
    yield d
    d.close()


def _error_codes(errors: list[X11Error]) -> list[int]:
    return [e.error_code for e in errors]


class TestIsolation:
    def test_foreign_windows_and_kill_client(self, desk):
        """Foreign windows are opaque; KillClient on them or AllTemporary fails."""
        assert isinstance(desk.a.get_geometry(desk.w_a), X11Reply)
        for foreign in (desk.w_root, desk.w_b):
            resp = desk.a.get_geometry(foreign)
            assert isinstance(resp, X11Error) and resp.error_code == BadAccess

        for target in (desk.w_b, 0):  # 0 = AllTemporary
            desk.a.send_request(struct.pack("<BxHI", 113, 2, target))
            assert _error_codes(desk.a.check()) == [BadAccess]
        for op in (9, 11):  # MapSubwindows, UnmapSubwindows on root
            desk.a.send_request(struct.pack("<BxHI", op, 2, desk.root_id))
            assert _error_codes(desk.a.check()) == [BadAccess]
        assert isinstance(desk.b.get_geometry(desk.w_b), X11Reply)

    def test_grab_server_is_noop_for_sandbox(self, desk):
        """GrabServer from a sandbox succeeds but does not freeze other clients."""
        desk.a.send_request(struct.pack("<BxH", 36, 1))
        assert desk.a.check() == []
        assert isinstance(desk.root.get_geometry(desk.w_root), X11Reply)
        desk.a.send_request(struct.pack("<BxH", 37, 1))
        assert desk.a.check() == []

    def test_root_client_message_requires_own_window(self, desk):
        """
        A sandbox may send EWMH ClientMessages to root about its own window
        (_NET_ACTIVE_WINDOW on w_a), but must not send global WM commands
        whose target window is root, None, or a foreign window.
        """
        root, a = desk.root, desk.a
        root.change_attributes_event_mask(desk.root_id, 0x00180000)  # SubstructureRedirect|Notify
        assert root.check() == []
        net_active = a.intern_atom("_NET_ACTIVE_WINDOW")
        net_cur_desk = a.intern_atom("_NET_CURRENT_DESKTOP")

        def send_root_msg(target_win: int, msg_type: int) -> list[int]:
            root.take_events()
            ev = struct.pack(
                "<BBHII5I", ClientMessage, 32, 0, target_win, msg_type, 1, 0, 0, 0, 0
            )
            a.send_request(
                struct.pack("<BBHII", 25, 0, 11, desk.root_id, 0x00180000) + ev
            )
            assert a.check() == []
            return [
                struct.unpack_from("<I", e, 4)[0]
                for e in root.take_events(ClientMessage)
            ]

        assert send_root_msg(desk.w_a, net_active) == [desk.w_a]
        for forbidden in (desk.root_id, 0, desk.w_root, desk.w_b):
            assert send_root_msg(forbidden, net_cur_desk) == [], f"target {forbidden:#x} leaked"

    def test_extension_access_filtering(self, desk):
        for blocked in (
            Extension.COMPOSITE,
            Extension.XTEST,
            Extension.RECORD,
            Extension.SECURITY,
            Extension.MIT_SCREEN_SAVER,
            "X-NAMESPACE",
        ):
            assert desk.a.query_extension(blocked) is None, blocked
        for allowed in (
            Extension.SHAPE,
            Extension.XI,
            Extension.XKB,
            Extension.RANDR,
            Extension.XFIXES,
        ):
            assert desk.a.query_extension(allowed) is not None, allowed
        assert desk.root.query_extension("X-NAMESPACE") is not None


class TestTreeProjection:
    @pytest.mark.parametrize("swapped", [False, True], ids=["native", "swapped"])
    def test_reparented_hierarchy(self, xserver, swapped):
        """
        A host WM reparents sandbox windows into workspace -> split -> frame
        containers.  The sandbox must see its top-level as a direct child of
        root at screen coordinates and never see the containers.
        """
        root = NsConn(xserver, COOKIE_ROOT)
        a = NsConn(xserver, COOKIE_SBA, swapped=swapped)
        b = NsConn(xserver, COOKIE_SBB)
        root_id = root.root_window
        ws = root.create_window(x=0, y=20, width=1280, height=1004)
        split = root.create_window(parent=ws, width=1280, height=1004)
        frame_b = root.create_window(parent=split, width=640, height=1004)
        frame_a = root.create_window(parent=split, x=640, width=640, height=1004)
        w_b = b.create_window(width=638, height=1002)
        w_a = a.create_window(width=636, height=992)
        root.reparent_window(w_b, frame_b, 1, 1)
        root.reparent_window(w_a, frame_a, 2, 10)
        for w in (ws, split, frame_b, frame_a, w_b, w_a):
            root.map_window(w)
        assert root.check() == []

        parent, _ = a.query_tree(w_a)
        assert parent == root_id
        _, children = a.query_tree(root_id)
        assert w_a in children
        for hidden in (ws, split, frame_a, frame_b, w_b):
            assert hidden not in children

        resp = a.get_geometry(frame_a)
        assert isinstance(resp, X11Error) and resp.error_code == BadAccess

        resp = a.get_geometry(w_a)
        assert isinstance(resp, X11Reply)
        gx, gy, gw, gh = struct.unpack_from(f"{a.bo}hhHH", resp.data, 12)
        assert (gx, gy, gw, gh) == (642, 30, 636, 992)
        assert a.translate_coords(w_a, root_id, 0, 0)[1:] == (642, 30)

        assert a.translate_coords(root_id, root_id, 700, 100)[0] == w_a
        assert a.translate_coords(root_id, root_id, 100, 100)[0] == 0

        xtest = root.ext(Extension.XTEST)
        root.send_request(
            struct.pack(
                "<BBHBBHIIIIhhIHBB",
                xtest, X_XTestFakeInput, 9, MotionNotify, 0, 0,
                0, root_id, 0, 0, 700, 100, 0, 0, 0, 0,
            )
        )
        assert root.check() == []
        assert a.query_pointer(root_id)[0] == w_a
        assert a.xi_query_pointer(root_id)[0] == w_a
        for c in (root, a, b):
            c.close()


class TestRootProperties:
    def test_safe_properties_shared_sensitive_hidden(self, desk):
        root, a, b = desk.root, desk.a, desk.b
        res_mgr = root.intern_atom("RESOURCE_MANAGER")
        sensitive = [
            root.intern_atom("_NET_ACTIVE_WINDOW"),
            root.intern_atom("_NET_CLIENT_LIST"),
        ]
        root.change_property(desk.root_id, res_mgr, XA_STRING, 8, b"Xft.dpi:\t96\n")
        for prop in sensitive:
            root.change_property(desk.root_id, prop, XA_WINDOW, 32, struct.pack("<I", desk.w_root))
        root.change_property(desk.root_id, XA_CUT_BUFFER0, XA_STRING, 8, b"secret")
        assert root.check() == []

        assert a.get_property(desk.root_id, res_mgr) == b"Xft.dpi:\t96\n"
        a_props = set(a.list_properties(desk.root_id))
        assert res_mgr in a_props
        for prop in sensitive:
            assert a.get_property(desk.root_id, prop) == b"", prop
            assert prop not in a_props
        # CUT_BUFFER* is clipboard data: only visible with "allow clipboard"
        assert a.get_property(desk.root_id, XA_CUT_BUFFER0) == b""
        assert XA_CUT_BUFFER0 not in a_props
        assert b.get_property(desk.root_id, XA_CUT_BUFFER0) == b"secret"
        assert XA_CUT_BUFFER0 in set(b.list_properties(desk.root_id))

    def test_virtual_root_identifies_wm(self, desk):
        net_sup = desk.a.intern_atom("_NET_SUPPORTING_WM_CHECK")
        net_name = desk.a.intern_atom("_NET_WM_NAME")
        raw = desk.a.get_property(desk.root_id, net_sup)
        assert len(raw) == 4
        vroot = struct.unpack("<I", raw)[0]
        assert vroot not in (0, desk.root_id)
        assert desk.a.get_property(vroot, net_sup) == raw
        assert desk.a.get_property(vroot, net_name) == b"i3"

    def test_property_notify_only_for_safe_properties(self, desk):
        root, a, b = desk.root, desk.a, desk.b
        res_mgr = root.intern_atom("RESOURCE_MANAGER")
        active = root.intern_atom("_NET_ACTIVE_WINDOW")
        dummy = a.intern_atom("XNS_TS_PING")
        for c in (root, a, b):
            c.change_attributes_event_mask(desk.root_id, PropertyChangeMask)
            assert c.check() == []
            c.take_events()

        root.change_property(desk.root_id, active, XA_WINDOW, 32, struct.pack("<I", desk.w_root))
        assert root.check() == []
        assert a.take_events(PropertyNotify) == []

        root.change_property(desk.root_id, res_mgr, XA_STRING, 8, b"Xft.dpi:\t144\n")
        assert root.check() == []
        events = a.take_events(PropertyNotify)
        assert [struct.unpack_from("<I", e, 8)[0] for e in events] == [res_mgr]
        root.take_events()
        b.take_events()

        # QXcbConnection::getTimestamp() appends 0 items to rootWindow() and waits
        # for PropertyNotify with window == root_id; must stay isolated to sb_a.
        a.change_property(desk.root_id, dummy, XA_STRING, 8, b"12345678", mode=0)
        a.change_property(desk.root_id, dummy, XA_STRING, 8, b"", mode=2)
        # Partial GetProperty(delete=True) with bytesAfter > 0 must NOT emit PropertyDelete
        assert a.get_property(desk.root_id, dummy, delete=1, long_length=1) == b"1234"
        # Full GetProperty(delete=True) with bytesAfter == 0 emits PropertyDelete
        assert a.get_property(desk.root_id, dummy, delete=1, long_length=2) == b"12345678"
        # RotateProperties on virtual root emits PropertyNewValue with window == root_id
        dummy2 = a.intern_atom("XNS_TS_PING2")
        a.change_property(desk.root_id, dummy, XA_STRING, 8, b"a")
        a.change_property(desk.root_id, dummy2, XA_STRING, 8, b"b")
        a.take_events(PropertyNotify)
        a.send_request(struct.pack("<BxHIhhII", 114, 5, desk.root_id, 2, 1, dummy, dummy2))
        a.delete_property(desk.root_id, dummy)
        a.delete_property(desk.root_id, dummy2)
        assert a.check() == []
        own_ev = [struct.unpack_from("<IIIB", e, 4) for e in a.take_events(PropertyNotify)]
        assert [(w, atom, st) for w, atom, _, st in own_ev] == [
            (desk.root_id, dummy, 0),
            (desk.root_id, dummy2, 0),
            (desk.root_id, dummy, 1),
            (desk.root_id, dummy2, 1),
        ]
        assert all(ts > 0 for _, _, ts, _ in own_ev)
        assert root.take_events(PropertyNotify) == []
        assert b.take_events(PropertyNotify) == []


class TestInputIsolation:
    def test_keymap_and_device_state_hidden(self, desk):
        """A held key is visible to root but never to a sandbox, focused or not."""
        desk.key_down(XK_Shift_L)
        assert any(desk.root.query_keymap())
        assert any(desk.root.query_device_state_keys(VCK))
        desk.focus(desk.w_a)
        assert not any(desk.a.query_keymap())
        assert not any(desk.a.query_device_state_keys(VCK))

    def test_xkb_state_follows_focus(self, desk):
        desk.key_down(XK_Caps_Lock)
        desk.key_up(XK_Caps_Lock)
        desk.key_down(XK_Shift_L)
        assert desk.root.xkb_get_state()[0] & ShiftMask
        assert desk.root.xkb_get_indicator_state() != 0

        assert desk.a.xkb_get_state() == (0, 0)
        assert desk.a.xkb_get_indicator_state() == 0

        desk.focus(desk.w_a)
        base, locked = desk.a.xkb_get_state()
        assert base & ShiftMask and locked != 0
        assert desk.a.xkb_get_indicator_state() != 0

    def test_xkb_state_notify_follows_focus(self, desk):
        a = desk.a
        op = a.xkb_use_extension()
        xkb_event = a.query_extension(Extension.XKB).first_event
        a.send_request(
            struct.pack(
                "<BBHHHHHHH", op, X_kbSelectEvents, 4, XkbUseCoreKbd,
                XkbStateNotifyMask, 0, XkbStateNotifyMask, 0, 0,
            )
        )
        assert a.check() == []

        def mod_notifies():
            return [
                e for e in a.take_events(xkb_event)
                if e[1] == XkbStateNotify and (e[9] or e[10] or e[28])
            ]

        desk.key_down(XK_Shift_L)
        desk.key_up(XK_Shift_L)
        assert mod_notifies() == []

        desk.focus(desk.w_a)
        desk.key_down(XK_Shift_L)
        assert mod_notifies() != []

    def test_query_pointer_requires_mouse_motion(self, desk):
        desk.move(950, 50)  # inside w_nom
        desk.focus(desk.w_nom)
        assert desk.nom.query_pointer(desk.w_nom) == (0, 0, 0, 0, 0, 0)
        assert desk.nom.xi_query_pointer(desk.w_nom) == (0, 0, 0, 0, 0)
        # control: a sandbox with mouse-motion sees the real position
        assert desk.a.query_pointer(desk.root_id)[1:3] == (950, 50)
        assert desk.a.xi_query_pointer(desk.root_id)[1:3] == (950, 50)

    def test_grab_pointer_follows_focus(self, desk):
        assert desk.a.grab_pointer(desk.w_a) == AlreadyGrabbed
        desk.focus(desk.w_a)
        assert desk.a.grab_pointer(desk.w_a) == GrabSuccess
        desk.a.ungrab_pointer()
        assert desk.a.check() == []

    def test_active_keyboard_grab_revoked_on_focus_loss(self, desk):
        """
        If a focused sandbox grabs the keyboard and the host WM then moves
        focus to another window, the sandbox's keyboard grab must be
        deactivated immediately so it cannot keylog subsequent typing.
        """
        desk.root.change_attributes_event_mask(desk.w_root, KEY_BUTTON_MASK)
        assert desk.root.check() == []
        desk.focus(desk.w_a)
        assert desk.a.grab_keyboard(desk.w_a) == GrabSuccess

        # Host WM switches focus back to w_root
        desk.focus(desk.w_root)
        desk.a.take_events()
        desk.root.take_events()

        desk.key_down(XK_v)
        desk.key_up(XK_v)
        assert desk.a.take_events(KeyPress) == []
        assert desk.root.take_events(KeyPress) != []

    def test_click_grants_short_focus_ticket(self, desk):
        """Clicking an unfocused sandbox window (tray icon) lets it grab briefly."""
        desk.click(1, 350, 50)  # inside w_a, keyboard focus stays on w_root
        assert desk.a.grab_pointer(desk.w_a) == GrabSuccess
        desk.a.ungrab_pointer()
        assert desk.a.check() == []
        time.sleep(FOCUS_TICKET_MS / 1000 + 0.2)
        assert desk.a.grab_pointer(desk.w_a) == AlreadyGrabbed

    def test_raw_motion_follows_focus(self, desk):
        a = desk.a
        op = a.xi_init()
        a.send_request(
            struct.pack("<BBHIHxxHHI", op, X_XISelectEvents, 5, desk.root_id,
                        1, XIAllDevices, 1, 1 << XI_RawMotion)
        )
        assert a.check() == []

        def raw_motions():
            return [e for e in a.take_events(GenericEvent)
                    if struct.unpack_from("<H", e, 8)[0] == XI_RawMotion]

        desk.move(10, 10)
        desk.move(20, 20)
        assert raw_motions() == []
        desk.focus(desk.w_a)
        desk.move(30, 30)
        assert raw_motions() != []


class TestClipboard:
    def test_off_mode_uses_private_selection(self, desk):
        clip = desk.root.intern_atom("CLIPBOARD")
        desk.root.set_selection_owner(desk.w_root, clip)
        desk.focus(desk.w_nom)
        assert desk.nom.get_selection_owner(clip) == 0
        desk.nom.set_selection_owner(desk.w_nom, clip)
        assert desk.nom.check() == []
        assert desk.nom.get_selection_owner(clip) == desk.w_nom
        assert desk.root.get_selection_owner(clip) == desk.w_root

    def test_on_mode_shares_global_selection(self, desk):
        clip = desk.root.intern_atom("CLIPBOARD")
        desk.root.set_selection_owner(desk.w_root, clip)
        assert desk.root.check() == []
        assert desk.b.get_selection_owner(clip) == desk.w_root
        assert desk.nom.get_selection_owner(clip) == 0

    def test_auto_mode_copy_requires_focus(self, desk):
        clip = desk.root.intern_atom("CLIPBOARD")
        desk.root.set_selection_owner(desk.w_root, clip)
        desk.a.set_selection_owner(desk.w_a, clip)
        assert desk.a.check() == []
        assert desk.root.get_selection_owner(clip) == desk.w_root
        desk.focus(desk.w_a)
        desk.a.set_selection_owner(desk.w_a, clip)
        assert desk.a.check() == []
        assert desk.root.get_selection_owner(clip) == desk.w_a

    @pytest.mark.parametrize("gesture", ["ctrl+v", "shift+insert", "button2", "button1"])
    def test_auto_mode_paste_ticket(self, desk, gesture):
        clip = desk.root.intern_atom("CLIPBOARD")
        desk.root.set_selection_owner(desk.w_root, clip)
        desk.focus(desk.w_a)
        desk.move(350, 50)
        assert desk.a.get_selection_owner(clip) == 0
        # Synthetic SendEvent(KeyPress) from inside the sandbox must not grant paste
        desk.a.send_event(desk.w_a, struct.pack("<BBHIIIIhhhhHB", KeyPress, 9, 0, 0, desk.root_id, desk.w_a, 0, 0, 0, 0, 0, 0, 1))
        assert desk.a.check() == []
        assert desk.a.get_selection_owner(clip) == 0
        if gesture == "ctrl+v":
            desk.chord(XK_Control_L, XK_v)
        elif gesture == "shift+insert":
            desk.chord(XK_Shift_L, XK_Insert)
        elif gesture == "button2":
            desk.click(2, 350, 50)
        else:
            desk.click(1, 350, 50)
        assert desk.a.get_selection_owner(clip) == desk.w_root
        # Focus leaving w_a (even if it returns without a new press) revokes paste
        desk.focus(desk.w_root)
        assert desk.a.get_selection_owner(clip) == 0
        desk.focus(desk.w_a)
        assert desk.a.get_selection_owner(clip) == 0
        # Another namespace taking selection ownership while w_a is focused revokes paste
        desk.click(1, 350, 50)
        assert desk.a.get_selection_owner(clip) == desk.w_root
        desk.b.set_selection_owner(desk.w_b, clip)
        assert desk.b.check() == []
        assert desk.a.get_selection_owner(clip) == 0

    def test_unfocused_paste_gesture_grants_nothing(self, desk):
        clip = desk.root.intern_atom("CLIPBOARD")
        desk.root.set_selection_owner(desk.w_root, clip)
        desk.chord(XK_Control_L, XK_v)  # keyboard focus is on w_root
        assert desk.a.get_selection_owner(clip) == 0

    def test_cross_namespace_transfer(self, desk):
        """sb_b (on) owns CLIPBOARD; sb_a (auto) pastes after Ctrl+V."""
        a, b = desk.a, desk.b
        clip = a.intern_atom("CLIPBOARD")
        utf8 = a.intern_atom("UTF8_STRING")
        prop = a.intern_atom("XNS_TEST_PASTE")
        b.set_selection_owner(desk.w_b, clip)
        assert b.check() == []

        # without a pending request the owner cannot touch the requestor
        assert isinstance(b.get_window_attributes(desk.w_a), X11Error)
        assert isinstance(b.get_geometry(desk.w_a), X11Error)
        b.change_property(desk.w_a, prop, XA_STRING, 8, b"unsolicited")
        assert _error_codes(b.check()) == [BadAccess]

        desk.focus(desk.w_a)
        desk.chord(XK_Control_L, XK_v)
        b.take_events()
        a.convert_selection(desk.w_a, clip, utf8, prop)
        assert a.check() == []
        requests = b.take_events(SelectionRequest)
        assert len(requests) == 1
        requestor, selection, target, req_prop = struct.unpack_from("<IIII", requests[0], 12)
        assert (requestor, selection, target, req_prop) == (desk.w_a, clip, utf8, prop)

        # GDK wraps the requestor window via GetWindowAttributes/GetGeometry/QueryTree
        assert isinstance(b.get_window_attributes(desk.w_a), X11Reply)
        assert isinstance(b.get_geometry(desk.w_a), X11Reply)
        assert b.query_tree(desk.w_a) == (desk.root_id, [])
        # Changing window attributes or unmapping subwindows on the requestor remains blocked
        b.send_request(struct.pack("<BxHIII", 2, 4, desk.w_a, 0x0002, 0))  # CWBackPixel
        b.send_request(struct.pack("<BxHI", 11, 2, desk.w_a))  # UnmapSubwindows
        assert _error_codes(b.check()) == [BadAccess, BadAccess]

        b.change_property(desk.w_a, prop, XA_STRING, 8, b"hello")
        # Overwriting an existing property exercises DixWriteAccess | DixPostAccess
        b.change_property(desk.w_a, prop, XA_STRING, 8, b"hello2")
        b.send_event(
            desk.w_a,
            struct.pack("<BxHIIIII", SelectionNotify, 0, 0, desk.w_a, clip, utf8, prop),
        )
        assert b.check() == []
        notifies = a.take_events(SelectionNotify)
        assert len(notifies) == 1
        assert struct.unpack_from("<I", notifies[0], 20)[0] == prop
        assert a.get_property(desk.w_a, prop) == b"hello2"

        # INCR continuation: even after focus leaves w_a (pasteAllowed revoked),
        # the already-authorized selection transfer remains valid for PropertyDelete + chunk writes.
        b.change_attributes_event_mask(desk.w_a, PropertyChangeMask)
        assert b.check() == []
        b.take_events()
        desk.focus(desk.w_root)
        assert a.get_selection_owner(clip) == 0
        a.delete_property(desk.w_a, prop)
        assert a.check() == []
        del_ev = b.take_events(PropertyNotify)
        assert len(del_ev) == 1
        assert struct.unpack_from("<IIIB", del_ev[0], 4)[::3] == (desk.w_a, 1)
        b.change_property(desk.w_a, prop, XA_STRING, 8, b"chunk2")
        assert b.check() == []
        assert a.get_property(desk.w_a, prop) == b"chunk2"

    @pytest.mark.parametrize("host_takes_clipboard", [False, True])
    def test_export_revoked_when_host_takes_selection(self, desk, host_takes_clipboard):
        """An auto-mode CLIPBOARD export survives host PRIMARY, ends when host takes CLIPBOARD."""
        root, a = desk.root, desk.a
        clip = root.intern_atom("CLIPBOARD")
        prop = root.intern_atom("XNS_TEST_COPY")
        desk.focus(desk.w_a)
        a.set_selection_owner(desk.w_a, clip)
        assert a.check() == []
        a.take_events()
        root.convert_selection(desk.w_root, clip, XA_STRING, prop)
        assert root.check() == []
        assert len(a.take_events(SelectionRequest)) == 1

        # Even if the sandbox still owns PRIMARY, losing CLIPBOARD revokes the CLIPBOARD transfer
        a.set_selection_owner(desk.w_a, XA_PRIMARY)
        # Selecting text on the host (PRIMARY) must not break a pending CLIPBOARD paste
        root.set_selection_owner(desk.w_root, XA_PRIMARY)
        if host_takes_clipboard:
            a.set_selection_owner(desk.w_a, XA_PRIMARY)
            root.set_selection_owner(desk.w_root, clip)
        assert root.check() == []
        a.change_property(desk.w_root, prop, XA_STRING, 8, b"copied")
        errors = _error_codes(a.check())
        assert errors == ([BadAccess] if host_takes_clipboard else [])

    def test_set_namespace_flags_switches_mode_live(self, desk):
        root, nom = desk.root, desk.nom
        clip = root.intern_atom("CLIPBOARD")
        root.set_selection_owner(desk.w_root, clip)
        assert root.check() == []
        xns = root.ext("X-NAMESPACE")
        name = b"sb_nomouse"
        pad = b"\x00" * ((4 - len(name) % 4) % 4)

        def set_clipboard(on: bool) -> None:
            resp = root.reply(
                struct.pack("<BBHIIHxx", xns, X_XnsSetNamespaceFlags,
                            (16 + len(name) + len(pad)) // 4,
                            XNS_CAPABILITY_CLIPBOARD,
                            XNS_CAPABILITY_CLIPBOARD if on else 0, len(name))
                + name + pad
            )
            assert isinstance(resp, X11Reply), f"SetNamespaceFlags failed: {resp}"

        assert nom.get_selection_owner(clip) == 0
        set_clipboard(True)
        assert nom.get_selection_owner(clip) == desk.w_root
        set_clipboard(False)
        assert nom.get_selection_owner(clip) == 0


class TestSystemTray:
    def test_dock_request_only_for_own_icon(self, desk):
        root, a = desk.root, desk.a
        tray_sel = root.intern_atom("_NET_SYSTEM_TRAY_S0")
        tray_op = root.intern_atom("_NET_SYSTEM_TRAY_OPCODE")
        w_tray = root.create_window(width=24, height=24)
        root.set_selection_owner(w_tray, tray_sel)
        assert root.check() == []
        assert a.get_selection_owner(tray_sel) == w_tray

        def dock(icon: int) -> list[bytes]:
            a.send_event(
                w_tray,
                struct.pack("<BBHII5I", ClientMessage, 32, 0, w_tray, tray_op, 0, 0, icon, 0, 0),
            )
            assert a.check() == []
            return [struct.unpack_from("<I", e, 20)[0] for e in root.take_events(ClientMessage)]

        root.take_events()
        assert dock(desk.w_a) == [desk.w_a]
        assert dock(desk.w_root) == []
        for op in (9, 11):  # MapSubwindows, UnmapSubwindows on tray window
            a.send_request(struct.pack("<BxHI", op, 2, w_tray))
            assert _error_codes(a.check()) == [BadAccess]

    def test_xdnd_host_to_sandbox_and_uri_helper(self, desk, xserver):
        root, a, b = desk.root, desk.a, desk.b
        xdnd_sel = root.intern_atom("XdndSelection")
        xdnd_enter = root.intern_atom("XdndEnter")
        xdnd_leave = root.intern_atom("XdndLeave")
        xdnd_status = root.intern_atom("XdndStatus")
        xdnd_finished = root.intern_atom("XdndFinished")
        xdnd_typelist = root.intern_atom("XdndTypeList")
        uri_list = root.intern_atom("text/uri-list")
        secret_prop = root.intern_atom("HOST_SECRET")
        target_prop = root.intern_atom("XNS_XDND_DATA")
        helper_sel = root.intern_atom("_XNS_URI_HELPER_sb_a")

        desk.focus(desk.w_root)
        root.set_selection_owner(desk.w_root, xdnd_sel)
        root.change_property(desk.w_root, xdnd_typelist, XA_ATOM, 32, struct.pack("<I", uri_list))
        root.change_property(desk.w_root, secret_prop, XA_STRING, 8, b"secret")
        assert root.check() == []

        def raw_get_prop(conn, wid: int, prop: int):
            return conn.reply(struct.pack("<BBHIIIII", 20, 0, 6, wid, prop, 0, 0, 64))

        # Before XdndEnter, unfocused auto-mode sandbox cannot read XdndTypeList or XdndSelection
        assert isinstance(raw_get_prop(a, desk.w_root, xdnd_typelist), X11Error)
        assert a.get_selection_owner(xdnd_sel) == 0

        # Forged XdndEnter from inside sandbox must not open access to w_root
        a.send_event(
            desk.w_a,
            struct.pack("<BBHII5I", ClientMessage, 32, 0, desk.w_a, xdnd_enter, desk.w_root, 0, 0, 0, 0),
        )
        assert a.check() == []
        assert isinstance(raw_get_prop(a, desk.w_root, xdnd_typelist), X11Error)

        # Genuine XdndEnter from host opens XdndTypeList, XdndSelection, and XdndStatus/Finished
        root.send_event(
            desk.w_a,
            struct.pack("<BBHII5I", ClientMessage, 32, 0, desk.w_a, xdnd_enter, desk.w_root, 1, uri_list, 0, 0),
        )
        assert root.check() == []
        assert a.get_property(desk.w_root, xdnd_typelist) == struct.pack("<I", uri_list)
        # Other host properties and writes remain blocked
        assert isinstance(raw_get_prop(a, desk.w_root, secret_prop), X11Error)
        a.change_property(desk.w_root, xdnd_typelist, XA_ATOM, 32, struct.pack("<I", 0))
        assert _error_codes(a.check()) == [BadAccess]
        assert a.get_selection_owner(xdnd_sel) == desk.w_root

        # Active XDND session must not open global CLIPBOARD without focus
        clip = root.intern_atom("CLIPBOARD")
        root.set_selection_owner(desk.w_root, clip)
        assert root.check() == []
        assert a.get_selection_owner(clip) == 0

        # XdndStatus reply to w_root is allowed only with own target window in data[0]
        root.take_events()
        a.send_event(
            desk.w_root,
            struct.pack("<BBHII5I", ClientMessage, 32, 0, desk.w_root, xdnd_status, desk.w_root, 1, 0, 0, 0),
        )
        assert a.check() == []
        assert root.take_events(ClientMessage) == []
        a.send_event(
            desk.w_root,
            struct.pack("<BBHII5I", ClientMessage, 32, 0, desk.w_root, xdnd_status, desk.w_a, 1, 0, 0, 0),
        )
        assert a.check() == []
        assert len(root.take_events(ClientMessage)) == 1

        # URI helper interception: ConvertSelection for text/uri-list routes SelectionRequest
        # to the namespace's URI helper first, while non-URI targets go directly to the host owner.
        helper = NsConn(xserver, COOKIE_ROOT)
        try:
            w_helper = helper.create_window(width=1, height=1)
            helper.set_selection_owner(w_helper, helper_sel)
            assert helper.check() == []

            # Even a focused sandbox cannot hijack _XNS_URI_HELPER_* from the host helper
            desk.focus(desk.w_a)
            a.set_selection_owner(desk.w_a, helper_sel)
            assert a.check() == []
            assert helper.get_selection_owner(helper_sel) == w_helper
            desk.focus(desk.w_root)

            # Non-URI target (XA_STRING) goes directly to root without bothering helper
            a.convert_selection(desk.w_a, xdnd_sel, XA_STRING, target_prop)
            assert a.check() == []
            assert len(root.take_events(SelectionRequest)) == 1
            assert helper.take_events(SelectionRequest) == []

            # URI target (text/uri-list) is routed to helper via SELECTION_FILTER_EV_REQUEST
            a.convert_selection(desk.w_a, xdnd_sel, uri_list, target_prop)
            assert a.check() == []
            assert root.take_events(SelectionRequest) == []
            req_evs = helper.take_events(SelectionRequest)
            assert len(req_evs) == 1
            _, req_time, req_owner, req_win, req_sel, req_target, req_prop = struct.unpack_from("<B3xIIIIII", req_evs[0])
            assert (req_owner, req_win, req_sel, req_target, req_prop) == (
                desk.w_root, desk.w_a, xdnd_sel, uri_list, target_prop
            )

            # Helper fetches from host owner onto its own w_helper window
            helper.convert_selection(w_helper, req_sel, req_target, req_prop)
            assert helper.check() == []
            host_reqs = root.take_events(SelectionRequest)
            assert len(host_reqs) == 1
            root.change_property(w_helper, req_prop, uri_list, 8, b"file:///tmp/drop.png\r\n")
            root.send_event(
                w_helper,
                struct.pack("<BxHIIIII", SelectionNotify, 0, req_time, w_helper, req_sel, req_target, req_prop),
            )
            assert root.check() == []
            assert len(helper.take_events(SelectionNotify)) == 1
            data = helper.get_property(w_helper, req_prop)
            assert data == b"file:///tmp/drop.png\r\n"

            # After staging the file, helper writes property and notifies sandbox requestor
            helper.change_property(req_win, req_prop, uri_list, 8, data)
            helper.send_event(
                req_win,
                struct.pack("<BxHIIIII", SelectionNotify, 0, req_time, req_win, req_sel, req_target, req_prop),
            )
            assert helper.check() == []
            assert len(a.take_events(SelectionNotify)) == 1
            assert a.get_property(desk.w_a, target_prop) == b"file:///tmp/drop.png\r\n"
        finally:
            helper.close()

        # XdndFinished closes the XDND session
        a.send_event(
            desk.w_root,
            struct.pack("<BBHII5I", ClientMessage, 32, 0, desk.w_root, xdnd_finished, desk.w_a, 1, 0, 0, 0),
        )
        assert a.check() == []
        assert a.get_selection_owner(xdnd_sel) == 0
        assert isinstance(raw_get_prop(a, desk.w_root, xdnd_typelist), X11Error)

        # XdndLeave from host also closes the session
        root.send_event(
            desk.w_a,
            struct.pack("<BBHII5I", ClientMessage, 32, 0, desk.w_a, xdnd_enter, desk.w_root, 1, uri_list, 0, 0),
        )
        assert root.check() == []
        assert a.get_selection_owner(xdnd_sel) == desk.w_root
        root.send_event(
            desk.w_a,
            struct.pack("<BBHII5I", ClientMessage, 32, 0, desk.w_a, xdnd_leave, desk.w_root, 0, 0, 0, 0),
        )
        assert root.check() == []
        assert a.get_selection_owner(xdnd_sel) == 0

        # Destroying the host XDND source window automatically invalidates xdndSourceWin
        w_src_tmp = root.create_window(width=10, height=10)
        root.set_selection_owner(w_src_tmp, xdnd_sel)
        root.send_event(
            desk.w_a,
            struct.pack("<BBHII5I", ClientMessage, 32, 0, desk.w_a, xdnd_enter, w_src_tmp, 1, uri_list, 0, 0),
        )
        assert root.check() == []
        assert a.get_selection_owner(xdnd_sel) == w_src_tmp
        root.send_request(struct.pack("<BxHI", 4, 2, w_src_tmp))  # DestroyWindow
        assert root.check() == []
        root.set_selection_owner(desk.w_root, xdnd_sel)
        assert root.check() == []
        assert a.get_selection_owner(xdnd_sel) == 0
