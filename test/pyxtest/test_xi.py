# SPDX-License-Identifier: MIT
#
# Security tests for XI/XI2 (XInput) extension vulnerabilities.

import struct
import time

import pytest
from inputtest import InputTestConnection
from proto import x11, xfixes, xi, xtest
from proto.xi import (
    XI_GestureSwipeBegin,
    XI_GestureSwipeEnd,
    XI_GestureSwipeUpdate,
)
from xclient import Extension, X11Error, X11Reply


def find_device(
    resp, name_prefix: str, use: int | None = None, byte_order: str = "<"
) -> int | None:
    """Parse an XIQueryDevice reply to find a master device by name.

    Returns the device ID or None if not found.
    """
    if not isinstance(resp, X11Reply):
        return None

    reply = xi.XIQueryDeviceReply.from_reply(resp.data, byte_order)

    for device in reply.devices:
        if (use is None or device.use == use) and device.name.startswith(name_prefix):
            return device.deviceid

    return None


@pytest.fixture
def xi_xclient(xclient):
    """Provide an xclient with XI2 initialized."""
    ext = xclient.query_extension(Extension.XI)
    if not ext:
        pytest.skip("XInput extension not available")

    req = xi.XIQueryVersionRequest(opcode=ext.opcode)
    xclient.send_request(req)
    xclient.recv_response(timeout=5.0)
    return xclient


@pytest.fixture
def xi_xclient_swapped(xclient_swapped):
    """Provide a byte-swapped xclient with XI2 initialized."""
    ext = xclient_swapped.query_extension(Extension.XI)
    if not ext:
        pytest.skip("XInput extension not available")

    req = xi.XIQueryVersionRequest(opcode=ext.opcode)
    xclient_swapped.send_request(req)
    xclient_swapped.recv_response(timeout=5.0)
    return xclient_swapped


class TestXIPassiveGrab:
    """Tests for XIPassiveGrabDevice/UngrabDevice vulnerabilities."""

    def test_passive_grab_detail_above_255(self, xserver, xi_xclient):
        """
        CVE-2022-46341 / ZDI-CAN-19381: OOB write via oversized detail
        in XIPassiveGrabDevice.

        The ``detail`` field is 32 bits on the wire but the server's
        grab mask arrays are only 256 bits wide.  A detail > 255
        causes an OOB array access in the grab handling code.

        The fix pretends that details > 255 are already grabbed,
        returning a reply with XIAlreadyGrabbed status for every
        requested modifier.

        Fixed in commit 51eb63b0ee15 ("Xi: disallow passive grabs with a
        detail > 255").
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        wid = xi_xclient.create_window()

        req = xi.XIPassiveGrabDeviceRequest(
            opcode=opcode,
            grab_window=wid,
            detail=256,  # OOB: valid range is 0-255
            grab_type=xi.XIGrabtypeButton,
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - OOB in XIPassiveGrabDevice (CVE-2022-46341)"
        )
        # The server returns a normal reply with every modifier marked
        # as XIAlreadyGrabbed.
        assert isinstance(resp, X11Reply), f"Expected a reply, got {resp}"
        num_modifiers = struct.unpack_from("<H", resp.data, 8)[0]
        assert num_modifiers > 0, "Expected at least one failed modifier"
        for i in range(num_modifiers):
            status = resp.data[32 + i * 8 + 4]
            assert status == xi.XIAlreadyGrabbed, (
                f"Modifier {i}: expected XIAlreadyGrabbed ({xi.XIAlreadyGrabbed}), "
                f"got status {status}"
            )

    def test_passive_ungrab_detail_oob_write(self, xserver, xi_xclient):
        """
        CVE-2022-46341 / ZDI-CAN-19381: OOB write in
        ProcXIPassiveUngrabDevice via oversized detail value.

        The ``detail`` field is 32 bits on the wire but
        DeleteDetailFromMask() uses it to index into a 256-bit
        (8 x CARD32) mask array.  A detail > 255 writes past the end
        of the allocated mask, corrupting heap memory.

        The fix rejects detail > 255 early, returning BadValue.

        Fixed in commit 51eb63b0ee15 ("Xi: disallow passive grabs with a
        detail > 255").
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        wid = xi_xclient.create_window()

        req = xi.XIPassiveUngrabDeviceRequest(
            opcode=opcode,
            grab_window=wid,
            detail=256,  # OOB: valid range is 0-255
            grab_type=xi.XIGrabtypeButton,
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - OOB write in XIPassiveUngrabDevice (CVE-2022-46341)"
        )
        # The fix returns BadValue (error code 2)
        assert isinstance(resp, X11Error), f"Expected an error reply, got {resp}"
        assert resp.error_code == x11.BadValue, (
            f"Expected BadValue ({x11.BadValue}), got error code {resp.error_code}"
        )

    @pytest.mark.asan
    def test_passive_ungrab_modifier_oob_write(self, xserver, xi_xclient):
        """
        ZDI-CAN-32366: ProcXIPassiveUngrabDevice does not validate modifier
        values. The grab path (ProcXIPassiveGrabDevice) validates modifiers
        via CheckGrabValues(), but the ungrab path passes them directly to
        DeletePassiveGrabFromList(). BITCLEAR(mask, modifier) with
        modifier > 255 indexes mask[modifier>>5] past the 8-word (32-byte)
        mask allocation, causing a controlled single-bit-clear at an
        attacker-chosen heap offset.

        The fix adds the same modifier validation to the ungrab path.
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        wid = xi_xclient.create_window()

        # First create a valid grab so DeletePassiveGrabFromList has
        # something to match against
        grab_req = xi.XIPassiveGrabDeviceRequest(
            opcode=opcode,
            grab_window=wid,
            detail=1,
            grab_type=xi.XIGrabtypeButton,
        )
        xi_xclient.send_request(grab_req)
        xi_xclient.recv_response(timeout=2.0)

        # Now try to ungrab with an oversized modifier (0x10000)
        # This would cause BITCLEAR to write past the mask allocation
        req = xi.XIPassiveUngrabDeviceRequest(
            opcode=opcode,
            grab_window=wid,
            detail=1,
            grab_type=xi.XIGrabtypeButton,
            modifiers=[0x10000],  # > 255: OOB write via BITCLEAR
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - XIPassiveUngrabDevice modifier OOB write: "
            "BITCLEAR(mask, modifier) with modifier > 255"
        )
        assert isinstance(resp, X11Error), f"Expected an error, got {resp}"
        assert resp.error_code == x11.BadValue, (
            f"Expected BadValue ({x11.BadValue}), got error code {resp.error_code}"
        )

    @pytest.mark.asan
    def test_passive_grab_modifier_device_uaf(self, xserver, xi_xclient):
        """
        ZDI-CAN-31832: Use-after-free in CheckPassiveGrab via
        grab->modifierDevice after device removal.

        XGrabDeviceButton creates a passive grab with grabtype XI. The grab
        stores a raw DeviceIntPtr in grab->modifierDevice without any reference
        counting or lifetime management.

        When the modifier device (a master keyboard) is removed via
        XIChangeHierarchy(XIRemoveMaster), CloseDevice frees
        the device struct, but doesn't clear the now-dangling
        grab->modifierDevice pointer.

        A subsequent pointer event routed to the grab window triggers
        CheckPassiveGrabsOnWindow -> CheckPassiveGrab, which
        dereferences the freed device via gdev = grab->modifierDevice
        then reads gdev->key.
        """
        conn = xi_xclient
        opcode = conn.query_extension(Extension.XI).opcode

        # 1. Create a window for the grab target
        wid = conn.create_window()
        req = x11.MapWindowRequest(window=wid)
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # 2. Create a new master device pair "uaf"
        add_info = xi.XIAddMasterInfo(name="uaf", send_core=1, enable=1)
        req = xi.XIChangeHierarchyRequest(
            opcode=opcode,
            num_changes=1,
            changes_data=add_info.to_bytes("<"),
        )
        conn.send_request(req)
        conn.flush_responses(timeout=1.0)

        req = xi.XIQueryDeviceRequest(opcode=opcode, deviceid=xi.XIAllDevices)
        conn.send_request(req)
        resp = conn.recv_response(timeout=2.0)
        kbd_id = find_device(resp, name_prefix="uaf", use=xi.XIMasterKeyboard)
        if kbd_id is None:
            pytest.fail("Could not find 'uaf' master keyboard device")

        # 3. XGrabDeviceButton: passive grab on VCP with modifierDevice=kbd_id.
        #    This creates a grabtype=XI grab with modifierDevice pointing
        #    to the new master keyboard.
        req = xi.XGrabDeviceButtonRequest(
            opcode=opcode,
            grab_window=wid,
            grabbed_device=xi.VirtualCorePointer,
            modifier_device=kbd_id,
            button=0,  # AnyButton
            modifiers=0x8000,  # AnyModifier
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # 4. Remove the master device - this frees the device struct
        rem_info = xi.XIRemoveMasterInfo(
            deviceid=kbd_id,
            return_mode=xi.XIFloating,
        )
        req = xi.XIChangeHierarchyRequest(
            opcode=opcode,
            num_changes=1,
            changes_data=rem_info.to_bytes("<"),
        )
        conn.send_request(req)
        conn.flush_responses(timeout=1.0)

        # 6. Warp the pointer into the grab window to trigger
        #    CheckMotion -> ActivateEnterGrab -> CheckPassiveGrab -> UAF
        req = x11.WarpPointerRequest(
            dst_window=wid,
            dst_x=50,
            dst_y=50,
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - passive grab modifierDevice "
            "use-after-free on device removal (ZDI-CAN-31832)"
        )


class TestXIChangeProperty:
    """Tests for XIChangeProperty vulnerabilities."""

    def test_num_items_overflow_truncation(self, xserver, xi_xclient):
        """
        CVE-2022-46344 / ZDI-CAN-19405: Integer truncation in
        ProcXIChangeProperty length check.

        ``totalSize = num_items * (format / 8)`` was computed as a 32-bit
        int. With format=32 and num_items=0x40000000, the multiplication
        overflows to 0, passing the REQUEST_FIXED_SIZE check. The server
        would then read num_items elements from beyond the request buffer.

        The fix changed totalSize from ``int`` to ``uint64_t``.

        Fixed in commit 8f454b793e1f ("Xi: avoid integer truncation in
        length check of ProcXIChangeProperty").
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        prop_atom = xi_xclient.intern_atom("_TEST_OVERFLOW")
        type_atom = xi_xclient.intern_atom("INTEGER")

        # format=32 (4 bytes per item), num_items=0x40000000
        # totalSize as int32: 0x40000000 * 4 = 0x100000000 → truncates to 0
        # totalSize as uint64: 0x100000000 → fails REQUEST_FIXED_SIZE
        req = xi.XIChangePropertyRequest(
            opcode=opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=xi.PropModeReplace,
            num_items=0x40000000,
            data=b"",  # no actual data
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)

        assert xserver.is_alive, (
            "Server crashed - integer truncation in XIChangeProperty (CVE-2022-46344)"
        )
        # The server should reject with BadLength (16).  Without the fix
        # the truncated totalSize (0) passes REQUEST_FIXED_SIZE and the
        # server tries to allocate 4 GB, failing with BadAlloc (11)
        # instead.
        assert isinstance(resp, X11Error), f"Expected an error, got {resp}"
        assert resp.error_code == x11.BadLength, (
            f"Expected BadLength ({x11.BadLength}), got error code {resp.error_code} - "
            f"integer truncation not caught by length check"
        )

    def test_prepend_property_size_and_offset(self, xserver, xi_xclient):
        """
        CVE-2023-5367 / ZDI-CAN-22153: Incorrect size and offset
        calculation when prepending to XI device properties.

        Two bugs in XIChangeDeviceProperty (and the identical RandR code):
        1. new_value.size was set to ``len`` instead of ``total_len``
           (new + existing), so the property lost the old data's size.
        2. The old_data offset for PropModePrepend used
           ``prop_value->size`` instead of ``len``, placing old data at
           the wrong position and writing out of bounds.

        This test sets a property, then prepends to it and reads back
        the result. On a fixed server, the property contains all values
        in the correct order.

        Fixed in commit 541ab2ecd41d ("Xi/randr: fix handling of
        PropModeAppend/Prepend").
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode

        prop_atom = xi_xclient.intern_atom("_TEST_PREPEND")
        type_atom = xi_xclient.intern_atom("INTEGER")

        # Step 1: Set initial property with values [10, 20, 30]
        initial_data = struct.pack("<III", 10, 20, 30)
        req = xi.XIChangePropertyRequest(
            opcode=opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=xi.PropModeReplace,
            data=initial_data,
        )
        xi_xclient.send_request(req)
        xi_xclient.flush_responses(timeout=0.5)

        # Step 2: Prepend values [1, 2]
        prepend_data = struct.pack("<II", 1, 2)
        req = xi.XIChangePropertyRequest(
            opcode=opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=xi.PropModePrepend,
            data=prepend_data,
        )
        xi_xclient.send_request(req)
        xi_xclient.flush_responses(timeout=0.5)

        assert xserver.is_alive, (
            "Server crashed - OOB write in XIChangeProperty prepend (CVE-2023-5367)"
        )

        # Step 3: Read back and verify
        req = xi.XIGetPropertyRequest(
            opcode=opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=2.0)

        assert isinstance(resp, X11Reply), f"Expected a reply, got {resp}"
        num_items = struct.unpack_from("<I", resp.data, 16)[0]
        assert num_items == 5, (
            f"Expected 5 items (2 prepended + 3 original), got {num_items}"
        )

        values = struct.unpack_from(f"<{num_items}I", resp.data, 32)
        assert values == (1, 2, 10, 20, 30), (
            f"Expected (1, 2, 10, 20, 30), got {values}"
        )

    @pytest.mark.swapped_client
    @pytest.mark.parametrize(
        "change_cls,get_cls",
        [
            (xi.XIChangePropertyRequest, xi.XIGetPropertyRequest),
            (xi.XChangeDevicePropertyRequest, xi.XGetDevicePropertyRequest),
        ],
        ids=["xi2", "xi1"],
    )
    def test_change_property_data_format32_swapped(
        self, xserver, xi_xclient_swapped, change_cls, get_cls
    ):
        """
        SProcXIChangeProperty and SProcXChangeDeviceProperty did not
        byte-swap the property data payload for format=32 properties.

        Set a format=32 property from a byte-swapped client with known
        values, then read it back. Without the fix, the stored values
        have the wrong byte order, causing a round-trip mismatch.

        Fixed in commit 243ef9bc2 ("Xi: Swap property data in
        SProcXChangeDeviceProperty/SProcXIChangeProperty").
        """
        conn = xi_xclient_swapped
        ext = conn.query_extension(Extension.XI)

        prop_atom = conn.intern_atom("_TEST_SWAP_FORMAT32")
        type_atom = conn.intern_atom("INTEGER")

        test_values = [0x12345678, 0xDEADBEEF, 42]
        data = b""
        for v in test_values:
            data += struct.pack(">I", v)

        req = change_cls(
            opcode=ext.opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
            format=32,
            mode=xi.PropModeReplace,
            data=data,
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        assert xserver.is_alive, "Server crashed during ChangeProperty"

        req = get_cls(
            opcode=ext.opcode,
            deviceid=xi.VirtualCorePointer,
            property_atom=prop_atom,
            type_atom=type_atom,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=2.0)

        assert isinstance(resp, X11Reply), f"Expected a reply, got {resp}"

        # Both XI1 and XI2 GetProperty replies share the same layout
        # for the fields we care about:
        #   bytes 16-19: num_items (CARD32)
        #   byte 20:     format   (CARD8)
        #   bytes 32+:   property data
        num_items = struct.unpack_from(">I", resp.data, 16)[0]
        fmt = resp.data[20]
        assert num_items == len(test_values), (
            f"Expected {len(test_values)} items, got {num_items}"
        )
        assert fmt == 32, f"Expected format 32, got {fmt}"

        values = struct.unpack_from(f">{num_items}I", resp.data, 32)
        assert values == tuple(test_values), (
            f"Property data round-trip failed: expected "
            f"{[hex(v) for v in test_values]}, got "
            f"{[hex(v) for v in values]} - property data not byte-swapped "
            f"in SProc handler for {change_cls.__name__}"
        )


class TestXIChangeDeviceControl:
    @pytest.mark.swapped_client
    @pytest.mark.xorg_only
    def test_change_device_control_resolution_values_swapped(
        self, xserver, xclient_swapped
    ):
        """
        SProcXChangeDeviceControl did not byte-swap the resolution
        values array for DEVICE_RESOLUTION.

        Send a ChangeDeviceControl/DEVICE_RESOLUTION with a resolution
        value that is valid in native byte order but out-of-range when
        byte-reversed (e.g. 1000 = 0x000003E8 → 0xE8030000 reversed).

        Without the fix: the garbled value exceeds max_resolution →
        BadValue error.
        With the fix: the correct value is in range → success reply
        (or BadMatch if the device doesn't support it, but not BadValue).

        Fixed in commit e24bd73e9d6f ("Xi: add missing byte-swap of
        resolution values in SProcXChangeDeviceControl").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.XI)
        if not ext:
            pytest.skip("XInput extension not available")

        req = xi.XIQueryVersionRequest(opcode=ext.opcode)
        conn.send_request(req)
        conn.recv_response(timeout=5.0)

        ctl = xi.DeviceResolutionCtl(
            first_valuator=0,
            num_valuators=1,
            resolutions=[1000],
        )
        ctl_bytes = ctl.to_bytes(">")

        req = xi.XChangeDeviceControlRequest(
            opcode=ext.opcode,
            control=xi.DEVICE_RESOLUTION,
            deviceid=xi.VirtualCorePointer,
            control_data=ctl_bytes,
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=2.0)

        assert xserver.is_alive, "Server crashed"

        # Without the fix: BadValue (error code 2) because the
        # byte-reversed resolution 0xE8030000 exceeds max_resolution.
        # With the fix: either a reply (success) or BadMatch (device
        # doesn't support resolution control), but NOT BadValue.
        if isinstance(resp, X11Error):
            assert resp.error_code != x11.BadValue, (
                "ChangeDeviceControl returned BadValue - "
                "resolution values not byte-swapped"
            )

    @pytest.mark.swapped_client
    def test_change_device_control_resolution_unbounded_swap(
        self, xserver, xi_xclient_swapped
    ):
        """Unbounded SwapLongs of DEVICE_RESOLUTION valuators."""
        conn = xi_xclient_swapped
        opcode = conn.query_extension(Extension.XI).opcode
        bo = conn._byte_order

        ctl = xi.DeviceResolutionCtl(
            first_valuator=0,
            num_valuators=255,
            resolutions=[],
        )
        bad = xi.XChangeDeviceControlRequest(
            opcode=opcode,
            control=xi.DEVICE_RESOLUTION,
            deviceid=xi.VirtualCorePointer,
            control_data=ctl.to_bytes(bo),
        )
        canary = x11.InternAtomRequest(name="_TEST_CDC_UNBOUNDED_SWAP")

        conn.send_request(bad.to_bytes(bo) + canary.to_bytes(bo))
        conn.seq += 1

        resp_bad = conn.recv_response(timeout=2.0)
        resp_canary = conn.recv_response(timeout=2.0)

        assert xserver.is_alive, "Server crashed"
        assert isinstance(resp_bad, X11Error), f"Expected error, got {resp_bad}"
        assert resp_bad.error_code == x11.BadLength, (
            f"Expected BadLength (16), got {resp_bad.error_code}"
        )
        assert isinstance(resp_canary, X11Reply), (
            f"InternAtom canary corrupted, got {resp_canary}"
        )
        atom = struct.unpack_from(f"{bo}I", resp_canary.data, 8)[0]
        assert atom != 0, "InternAtom canary returned None atom"


class TestXIChangeCursor:
    def test_change_cursor_null_window(self, xserver, xi_xclient):
        """
        XIChangeCursor dereferences pWin even if it's not set
        """
        opcode = xi_xclient.query_extension(Extension.XI).opcode
        req = xi.XIChangeCursorRequest(
            opcode=opcode,
            window=0,
            cursor=0,
            deviceid=xi.VirtualCorePointer,
        )
        xi_xclient.send_request(req)
        resp = xi_xclient.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed"

        # Without the fix: SegFault on a NULL WindowPtr
        # With the fix: BadWindow
        assert isinstance(resp, X11Error), f"Expected an error, got {resp}"
        assert resp.error_code == x11.BadWindow, (
            "ChangeCursor didn't return BadWindow for Window 0"
        )


class TestXIBarrier:
    @pytest.mark.asan
    def test_barrier_leave_event_buffer_overflow(self, xserver, xi_xclient):
        """
        ZDI-CAN-31938: Heap buffer overflow in input_constrain_cursor
        from too many barrier leave events.

        input_constrain_cursor() writes barrier hit/leave events
        into a fixed-size internal event buffer (GetMaximumEventsNum()
        = 100 slots). When all barriers are in the "released" state,
        the first loop marks every barrier as hit (released barriers
        skip the clamp, so dir never clears and iteration
        continues). The second loop emits one event per hit barrier
        with no capacity check.

        Creating >100 barriers at the same position, releasing all
        of them (by setting eventid=1 which matches the initial
        barrier_event_id), and driving pointer motion across them
        causes 200+ events to be written into the 100-slot buffer
        which is generally considered a bad idea..
        """
        conn = xi_xclient
        xi_opcode = conn.query_extension(Extension.XI).opcode

        xf = conn.query_extension(Extension.XFIXES)
        if not xf:
            pytest.skip("XFIXES extension not available")

        xt = conn.query_extension(Extension.XTEST)
        if not xt:
            pytest.skip("XTEST extension not available")

        req = xfixes.XFixesQueryVersionRequest(
            opcode=xf.opcode, major_version=5, minor_version=0
        )
        conn.send_request(req)
        resp = conn.recv_response(timeout=5.0)
        if resp is None:
            pytest.fail("XFixesQueryVersion got no response")

        # Create 200 barriers at x=500, vertical line y=[0,4000].
        # All at the same position so pointer motion crosses them all.
        barriers = []
        for _ in range(200):
            bid = conn.alloc_id()
            req = xfixes.XFixesCreatePointerBarrierRequest(
                opcode=xf.opcode,
                barrier=bid,
                window=conn.root_window,
                x1=500,
                y1=0,
                x2=500,
                y2=4000,
                directions=0,
                num_devices=0,
            )
            conn.send_request(req)
            barriers.append(bid)
        conn.flush_responses(timeout=1.0)

        # Release all barriers by sending XIBarrierReleasePointer
        # with eventid=1 (matching the initial barrier_event_id).
        # This puts all barriers into the "released" state.
        req = xi.XIBarrierReleasePointerRequest(
            opcode=xi_opcode,
            barriers=[(xi.VirtualCorePointer, bid, 1) for bid in barriers],
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # Position the pointer to the left of the barriers
        req = x11.WarpPointerRequest(
            dst_window=conn.root_window,
            dst_x=100,
            dst_y=2000,
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # Use xtest to generate relative pointer motion
        # crossing the barrier line at x=500 (motion dx=+500).
        # This will trigger a BarrierHit on all 200 barriers.
        req = xtest.XTestFakeInputRequest(
            opcode=xt.opcode,
            event_type=xtest.MotionNotify,
            detail=1,  # relative motion
            root_x=500,
            root_y=0,
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - barrier leave event buffer overflow (ZDI-CAN-31938)"
        )


# --- Xorg config template for inputtest gesture device ---

_GESTURE_XORG_CONF = """\
Section "ServerFlags"
    Option "AutoAddDevices" "false"
    Option "AutoEnableDevices" "false"
    Option "AllowEmptyInput" "true"
EndSection

Section "Device"
    Identifier "DummyCard"
    Driver "modesetting"
EndSection

Section "Screen"
    Identifier "DummyScreen"
    Device "DummyCard"
EndSection

Section "InputDevice"
    Identifier "GestureDevice"
    Driver "inputtest"
    Option "DeviceType" "PointerGesture"
    Option "SocketPath" "{socket_path}"
EndSection

Section "ServerLayout"
    Identifier "GestureLayout"
    Screen "DummyScreen"
    InputDevice "GestureDevice" "CorePointer"
EndSection
"""


class TestGestureWithInputTestDriver:
    """
    Gesture tests using the xf86-input-inputtest driver. XTEST doesn't
    support gestures so the only way to emulate gestures are uinput (requires
    root) or Xorg tests with the inputtest driver.
    """

    @pytest.fixture
    def xserver_args(self, tmp_path):
        """Override xserver_args to provide a custom Xorg config with
        the inputtest driver configured for PointerGesture events."""
        # Create a unique socket path for this test
        socket_path = tmp_path / "inputtest.sock"
        # Store it so the test can access it
        self._inputtest_socket_path = socket_path

        # Write the Xorg config to a temp file
        conf_path = str(tmp_path / "gesture-test.conf")
        with open(conf_path, "w") as f:
            f.write(_GESTURE_XORG_CONF.format(socket_path=socket_path))

        # These args are appended after the defaults in _build_command.
        # Since Xorg takes the last occurrence of -config, our -config
        # overrides the -config /dev/null from the builddir defaults.
        return ["-config", conf_path, "-configdir", "/dev/null"]

    @pytest.mark.asan
    @pytest.mark.xorg_only
    def test_gesture_sprite_uaf_on_window_destroy(self, xserver, xi_xclient):
        """
        ZDI-CAN-32753: Use-after-free in DeliverOneGestureEvent via
        gesture sprite after child window destruction.

        When a gesture begins (e.g. GestureSwipeBegin),
        ``GestureBuildSprite`` copies the current pointer sprite trace
        into the gesture's sprite. This trace contains raw ``WindowPtr``
        pointers to each window from the root down to the window under
        the pointer (e.g. root -> parent -> child).

        If the child window is destroyed while the gesture is active,
        the ``WindowPtr`` in the gesture sprite becomes dangling.
        A subsequent gesture update event triggers
        ``DeliverOneGestureEvent`` which walks the sprite trace and
        dereferences the freed ``WindowPtr``, causing a use-after-free.

        Test sequence:
            1. Start Xorg with inputtest driver (PointerGesture device)
            2. Create parent window, create child window inside it
            3. Warp pointer into child (builds sprite trace: root,
               parent, child)
            4. XISelectEvents for gesture swipe on parent
            5. Send GestureSwipeBegin via inputtest (captures sprite)
            6. DestroyWindow(child) -- sprite retains dangling pointer
            7. Send GestureSwipeUpdate -- dereferences freed WindowPtr
            8. Assert server is still alive (ASAN catches the UAF)
        """
        conn = xi_xclient
        opcode = conn.query_extension(Extension.XI).opcode

        # 1. Create parent window (200x200)
        parent_wid = conn.create_window(width=200, height=200)
        req = x11.MapWindowRequest(window=parent_wid)
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # 2. Create child window (100x100 inside parent)
        child_wid = conn.create_window(
            width=100, height=100, parent=parent_wid, x=10, y=10
        )
        req = x11.MapWindowRequest(window=child_wid)
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # 3. Warp pointer into child to build the sprite trace
        #    (root -> parent -> child)
        req = x11.WarpPointerRequest(dst_window=child_wid, dst_x=50, dst_y=50)
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # 4. XISelectEvents on parent for gesture swipe events.
        #    Gesture swipe event types: Begin=30, Update=31, End=32
        #    We need a mask with bits 30-32 set.
        #    Bit 30 is in byte 3 (30//8=3, bit 30%8=6), bit 0x40
        #    Bit 31 is in byte 3 (31//8=3, bit 31%8=7), bit 0x80
        #    Bit 32 is in byte 4 (32//8=4, bit 32%8=0), bit 0x01
        #    We need at least 5 bytes of mask, padded to 8 (2 words).
        mask_bytes = bytearray(8)

        def set_bit(mask: bytearray, bit: int):
            mask[bit // 8] |= 1 << (bit % 8)

        set_bit(mask_bytes, XI_GestureSwipeBegin)
        set_bit(mask_bytes, XI_GestureSwipeEnd)
        set_bit(mask_bytes, XI_GestureSwipeUpdate)
        req = xi.XISelectEventsRequest(
            opcode=opcode,
            window=parent_wid,
            masks=[(xi.XIAllDevices, bytes(mask_bytes))],
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # 5. Connect to the inputtest driver and send gesture begin
        socket_path = self._inputtest_socket_path
        it_conn = InputTestConnection.connect(socket_path, timeout_secs=1.0)

        it_conn.send_gesture_swipe(
            XI_GestureSwipeBegin, num_touches=3, delta_x=1.0, delta_y=1.0
        )
        it_conn.sync()

        # Give the server time to process the begin event
        time.sleep(0.2)

        # 6. Destroy the child window -- the gesture sprite still holds
        #    a pointer to the now-freed WindowPtr
        req = x11.DestroyWindowRequest(window=child_wid)
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # Give the server time to process the destroy
        time.sleep(0.2)

        # 7. Send gesture swipe update -- this triggers
        #    DeliverOneGestureEvent which walks the sprite trace
        #    and dereferences the freed child WindowPtr (UAF)
        it_conn.send_gesture_swipe(
            XI_GestureSwipeUpdate, num_touches=3, delta_x=1.0, delta_y=1.0
        )
        it_conn.sync()

        # Give ASAN time to detect and report the UAF
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - gesture sprite use-after-free on window "
            "destruction (ZDI-CAN-32753)"
        )

        # 8. Cleanup: send gesture end
        it_conn.send_gesture_swipe(
            XI_GestureSwipeEnd, num_touches=3, delta_x=0.0, delta_y=0.0
        )
        it_conn.sync()
        it_conn.close()
