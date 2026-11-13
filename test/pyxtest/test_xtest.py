# SPDX-License-Identifier: MIT
#
# Security tests for XTEST extension vulnerabilities.

import pytest
from proto import xtest
from proto.x11 import BadValue
from xclient import Extension, X11Error, X11Reply

# Stack array in ProcXTestFakeInput is MAX_VALUATORS (36) ints.
# Each DeviceValuator event can carry at most 6 axes.
MAX_VALUATORS = 36
VALUATORS_PER_EVENT = 6


@pytest.fixture
def xtest_xclient(xclient):
    """Provide an xclient with XTEST and XInput first-event base."""
    xt = xclient.query_extension(Extension.XTEST)
    if not xt:
        pytest.skip("XTEST extension not available")

    xi = xclient.query_extension(Extension.XI)
    if not xi:
        pytest.skip("XInput extension not available")

    req = xtest.GetVersionRequest(opcode=xt.opcode)
    xclient.send_request(req)
    resp = xclient.recv_response(timeout=5.0)
    if not isinstance(resp, X11Reply):
        pytest.skip("XTestGetVersion failed")

    return xclient, xt.opcode, xi.first_event


class TestXTestFakeInput:
    def test_valuator_range(self, xserver, xtest_xclient):
        """
        XTestFakeInput copied DeviceValuator fields into a fixed stack
        array before checking that first + count fit in the device axis
        range and in MAX_VALUATORS. A crafted request with enough
        valuator events could write past that array.

        Validate the count and range before indexing the stack array.
        """
        conn, opcode, first_event = xtest_xclient
        deviceid = 2  # Virtual core pointer

        n_records = (MAX_VALUATORS // VALUATORS_PER_EVENT) + 1
        events = [
            xtest.DeviceMotionNotifyEvent(
                first_event=first_event,
                deviceid=deviceid,
            )
        ]
        for i in range(n_records):
            events.append(
                xtest.DeviceValuatorEvent(
                    type=first_event + xtest.XI_DeviceValuator,
                    deviceid=deviceid,
                    first_valuator=i * VALUATORS_PER_EVENT,
                    num_valuators=VALUATORS_PER_EVENT,
                )
            )

        conn.send_request(xtest.FakeInputRequest(opcode=opcode, events=events))
        resp = conn.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed on XTestFakeInput valuator overflow"
        assert isinstance(resp, X11Error), f"Expected BadValue, got {resp}"
        assert resp.error_code == BadValue, (
            f"Expected BadValue ({BadValue}), got {resp.error_code}"
        )
