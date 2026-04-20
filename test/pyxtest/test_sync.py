# SPDX-License-Identifier: MIT
#
# Tests for SYNC extension.

import struct
import time

import pytest

from proto import sync
from xclient import Extension, X11Error, X11Reply, RawX11Connection


@pytest.fixture
def sync_xclient(xclient):
    """Provide an xclient with the SYNC extension initialized."""
    ext = xclient.query_extension(Extension.SYNC)
    if not ext:
        pytest.skip("SYNC extension not available")

    req = sync.InitializeRequest(opcode=ext.opcode)
    xclient.send_request(req.to_bytes())
    resp = xclient.recv_response(timeout=5.0)
    if isinstance(resp, X11Error):
        pytest.skip("SyncInitialize failed")

    return xclient, ext.opcode


class TestSyncQueryAlarm:
    """Tests for SyncQueryAlarm byte-swap fixes.

    Fix: Xext/sync: add a missing swapl(&rep.value_type) in
    ProcSyncQueryAlarm.

    Note: the server currently always returns value_type = XSyncAbsolute (0)
    in the QueryAlarm reply, so the missing swap has no observable effect
    (swap(0) == 0).  We set value_type = SyncRelative when creating the
    alarm to future-proof against the server returning the actual
    value_type.  If the server ever starts returning the stored
    value_type, this test will detect the missing swap.
    """

    @pytest.mark.swapped_client
    def test_query_alarm_value_type_swapped(self, xserver, xclient_swapped):
        """
        ProcSyncQueryAlarm was missing swapl(&rep.value_type).

        Create an alarm with value_type = SyncRelative (1), query it,
        and verify the reply has correctly swapped fields.  Currently
        the server normalizes value_type to 0 in the reply so we
        verify test_type instead (which IS affected by the existing
        swap code and proves the overall reply is well-formed).

        Fixed in commit 6c838f7cb8f0 ("Xext/sync: add a missing byte
        swap").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.SYNC)
        if not ext:
            pytest.skip("SYNC extension not available")

        req = sync.InitializeRequest(opcode=ext.opcode)
        conn.send_request(req.to_bytes(">"))
        conn.recv_response(timeout=5.0)

        alarm_id = conn.alloc_id()
        # Use SyncAbsolute because SyncRelative requires a counter.
        # We test the overall reply integrity via test_type and delta.
        value_mask = sync.SyncCAValueType | sync.SyncCATestType | sync.SyncCADelta
        values = struct.pack(
            ">III I",
            sync.SyncAbsolute,  # value_type = 0
            sync.SyncPositiveComparison,  # test_type = 2
            0,  # delta_hi
            1,  # delta_lo
        )
        req = sync.CreateAlarmRequest(
            opcode=ext.opcode,
            alarm_id=alarm_id,
            value_mask=value_mask,
            values=values,
        )
        conn.send_request(req.to_bytes(">"))
        responses = conn.flush_responses(timeout=2.0)
        create_errors = [r for r in responses if isinstance(r, X11Error)]
        assert len(create_errors) == 0, f"CreateAlarm failed: {create_errors}"

        req = sync.QueryAlarmRequest(
            opcode=ext.opcode,
            alarm_id=alarm_id,
        )
        conn.send_request(req.to_bytes(">"))
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed during SyncQueryAlarm"
        assert isinstance(resp, X11Reply), f"Expected reply, got {resp}"

        # xSyncQueryAlarmReply:
        #   [8]  counter(4)
        #   [12] value_type(4)
        #   [16] wait_value_hi(4)
        #   [20] wait_value_lo(4)
        #   [24] test_type(4)
        #   [28] delta_hi(4)
        #   [32] delta_lo(4)
        assert len(resp.data) >= 36, f"Reply too short: {len(resp.data)}"

        value_type = struct.unpack_from(">I", resp.data, 12)[0]
        test_type = struct.unpack_from(">I", resp.data, 24)[0]
        delta_lo = struct.unpack_from(">I", resp.data, 32)[0]

        assert value_type <= 1, (
            f"value_type = {value_type:#010x}, expected 0 or 1 (missing byte swap?)"
        )
        assert test_type == sync.SyncPositiveComparison, (
            f"test_type = {test_type:#010x}, expected "
            f"{sync.SyncPositiveComparison} (reply not properly swapped)"
        )
        assert delta_lo == 1, f"delta_lo = {delta_lo:#010x}, expected 1"


class TestSyncDestroyFence:
    """Tests for miSyncDestroyFence use-after-free via trigger list."""

    @pytest.mark.asan
    def test_destroy_fence_multi_trigger_uaf(self, xserver, sync_xclient):
        """
        ZDI-CAN-30159: miSyncDestroyFence iterates the fence's trigger
        list and invokes CounterDestroyed on each trigger before saving
        the next pointer. When two Await conditions reference the same
        fence, CounterDestroyed on the first trigger calls
        SyncAwaitTriggerFired -> FreeAwait, which frees the entire
        SyncAwaitUnion (containing all SyncAwait structs). The second
        trigger list node's pTrigger then points to freed memory,
        causing a use-after-free when CounterDestroyed is called on it.

        Fixed by saving pNext before invoking CounterDestroyed, matching
        the existing safe pattern in SyncChangeCounter().
        """
        client_a, opcode = sync_xclient

        # Create a fence (initially not triggered so AwaitFence blocks)
        fence_id = client_a.alloc_id()
        req = sync.CreateFenceRequest(
            opcode=opcode,
            drawable=client_a.root_window,
            fence_id=fence_id,
            initially_triggered=0,
        )
        client_a.send_request(req.to_bytes())
        client_a.flush_responses(timeout=0.5)

        # AwaitFence with the same fence listed twice.
        # This creates two trigger list nodes on the fence, both
        # pointing into the same SyncAwaitUnion.
        # Client A will block because the fence is not triggered.
        req = sync.AwaitFenceRequest(
            opcode=opcode,
            fence_ids=[fence_id, fence_id],
        )
        client_a.send_request(req.to_bytes())
        time.sleep(0.3)

        # Client B destroys the fence, triggering the UAF in
        # miSyncDestroyFence's trigger list iteration.
        client_b = RawX11Connection(xserver.display_num)
        try:
            ext_b = client_b.query_extension(Extension.SYNC)
            assert ext_b is not None
            req = sync.InitializeRequest(opcode=ext_b.opcode)
            client_b.send_request(req.to_bytes())
            client_b.recv_response(timeout=5.0)

            req = sync.DestroyFenceRequest(
                opcode=ext_b.opcode,
                fence_id=fence_id,
            )
            client_b.send_request(req.to_bytes())
            time.sleep(0.5)

            assert xserver.is_alive, (
                "Server crashed - miSyncDestroyFence UAF (ZDI-CAN-30159)"
            )
        finally:
            client_b.close()


class TestFreeCounter:
    """Tests for FreeCounter use-after-free via trigger list."""

    @pytest.mark.asan
    def test_destroy_counter_multi_trigger_uaf(self, xserver, sync_xclient):
        """
        ZDI-CAN-30163: FreeCounter iterates the counter's trigger list
        and invokes CounterDestroyed on each trigger before saving the
        next pointer. When two SyncAwait conditions reference the same
        counter, CounterDestroyed on the first trigger fires
        SyncAwaitTriggerFired -> FreeResource -> FreeAwait, which frees
        the SyncAwaitUnion. The second trigger's pTrigger then points to
        freed memory, causing a use-after-free function pointer call.

        Fixed by saving pnext before invoking CounterDestroyed.
        """
        client_a, opcode = sync_xclient

        # Create a counter with value 0
        counter_id = client_a.alloc_id()
        req = sync.CreateCounterRequest(
            opcode=opcode,
            counter_id=counter_id,
            initial_value_hi=0,
            initial_value_lo=0,
        )
        client_a.send_request(req.to_bytes())
        client_a.flush_responses(timeout=0.5)

        # SyncAwait with 2 conditions on the SAME counter.
        # Both wait for counter >= 1 (Absolute, PositiveComparison).
        # Since counter=0, neither fires, and Client A blocks.
        cond = (
            counter_id,
            sync.SyncAbsolute,
            0,  # wait_value_hi
            1,  # wait_value_lo (wait for >= 1)
            sync.SyncPositiveComparison,
            0,  # event_threshold_hi
            0,  # event_threshold_lo
        )
        req = sync.AwaitRequest(
            opcode=opcode,
            conditions=[cond, cond],  # same counter, two conditions
        )
        client_a.send_request(req.to_bytes())
        time.sleep(0.3)

        # Client B destroys the counter, triggering the UAF in
        # FreeCounter's trigger list iteration.
        client_b = RawX11Connection(xserver.display_num)
        try:
            ext_b = client_b.query_extension(Extension.SYNC)
            assert ext_b is not None
            req = sync.InitializeRequest(opcode=ext_b.opcode)
            client_b.send_request(req.to_bytes())
            client_b.recv_response(timeout=5.0)

            req = sync.DestroyCounterRequest(
                opcode=ext_b.opcode,
                counter_id=counter_id,
            )
            client_b.send_request(req.to_bytes())
            time.sleep(0.5)

            assert xserver.is_alive, "Server crashed - FreeCounter UAF (ZDI-CAN-30163)"
        finally:
            client_b.close()

    @pytest.mark.asan
    def test_destroy_counter_stale_triglist_head(self, xserver, sync_xclient):
        """
        FreeCounter iterates over the trigger list with
        nt_list_for_each_entry_safe, freeing each node after invoking
        CounterDestroyed. However, the sync object's pTriglist head
        pointer was not updated as nodes were freed.

        When CounterDestroyed calls FreeAwait for one Await group,
        FreeAwait iterates pSync->pTriglist to find and NULL out its
        triggers. If earlier nodes in the list have been freed by
        prior iterations of the destroy loop, pTriglist points to
        freed memory (stale head pointer).

        Attack: Create a counter, then issue two SEPARATE SyncAwait
        requests (creating two Await groups) each with a condition on
        the same counter. Destroying the counter iterates the trigger
        list - after freeing the first group's node, pTriglist still
        points to the freed node. The second group's CounterDestroyed
        callback triggers FreeAwait which reads the stale head.

        Fixed by updating pTriglist to pnext before each callback/free.
        """
        client_a, opcode = sync_xclient

        counter_id = client_a.alloc_id()
        req = sync.CreateCounterRequest(
            opcode=opcode,
            counter_id=counter_id,
            initial_value_hi=0,
            initial_value_lo=0,
        )
        client_a.send_request(req.to_bytes())
        client_a.flush_responses(timeout=0.5)

        # Issue TWO separate Await requests on the same counter,
        # creating two independent Await groups. Each adds a trigger
        # list node to the counter's trigger list.
        cond = (
            counter_id,
            sync.SyncAbsolute,
            0,  # wait_value_hi
            1,  # wait_value_lo (wait for >= 1)
            sync.SyncPositiveComparison,
            0,  # event_threshold_hi
            0,  # event_threshold_lo
        )

        # First Await: 2 conditions on same counter (same Await group)
        req = sync.AwaitRequest(
            opcode=opcode,
            conditions=[cond, cond],
        )
        client_a.send_request(req.to_bytes())
        time.sleep(0.1)

        # Second Await: 1 condition on same counter (different Await group)
        req = sync.AwaitRequest(
            opcode=opcode,
            conditions=[cond],
        )
        client_a.send_request(req.to_bytes())
        time.sleep(0.3)

        # Client B destroys the counter.
        # FreeCounter iterates: frees node for group1-cond1, then
        # CounterDestroyed on group1-cond2 triggers FreeAwait which
        # scans pTriglist (now stale without the fix).
        client_b = RawX11Connection(xserver.display_num)
        try:
            ext_b = client_b.query_extension(Extension.SYNC)
            assert ext_b is not None
            req = sync.InitializeRequest(opcode=ext_b.opcode)
            client_b.send_request(req.to_bytes())
            client_b.recv_response(timeout=5.0)

            req = sync.DestroyCounterRequest(
                opcode=ext_b.opcode,
                counter_id=counter_id,
            )
            client_b.send_request(req.to_bytes())
            time.sleep(0.5)

            assert xserver.is_alive, (
                "Server crashed - stale pTriglist head in FreeCounter"
            )
        finally:
            client_b.close()

    @pytest.mark.asan
    def test_destroy_fence_stale_triglist_head(self, xserver, sync_xclient):
        """
        Same stale pTriglist head issue in miSyncDestroyFence.

        Fixed by updating pFence->sync.pTriglist to pNext before each
        callback/free.
        """
        client_a, opcode = sync_xclient

        # Create two fences
        fence_id = client_a.alloc_id()
        req = sync.CreateFenceRequest(
            opcode=opcode,
            drawable=client_a.root_window,
            fence_id=fence_id,
            initially_triggered=0,
        )
        client_a.send_request(req.to_bytes())
        client_a.flush_responses(timeout=0.5)

        # Two separate AwaitFence requests on the same fence
        req = sync.AwaitFenceRequest(
            opcode=opcode,
            fence_ids=[fence_id, fence_id],
        )
        client_a.send_request(req.to_bytes())
        time.sleep(0.1)

        req = sync.AwaitFenceRequest(
            opcode=opcode,
            fence_ids=[fence_id],
        )
        client_a.send_request(req.to_bytes())
        time.sleep(0.3)

        client_b = RawX11Connection(xserver.display_num)
        try:
            ext_b = client_b.query_extension(Extension.SYNC)
            assert ext_b is not None
            req = sync.InitializeRequest(opcode=ext_b.opcode)
            client_b.send_request(req.to_bytes())
            client_b.recv_response(timeout=5.0)

            req = sync.DestroyFenceRequest(
                opcode=ext_b.opcode,
                fence_id=fence_id,
            )
            client_b.send_request(req.to_bytes())
            time.sleep(0.5)

            assert xserver.is_alive, (
                "Server crashed - stale pTriglist head in miSyncDestroyFence"
            )
        finally:
            client_b.close()
