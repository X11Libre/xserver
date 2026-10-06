# SPDX-License-Identifier: MIT
#
# XTEST extension protocol request builders for security testing.

import struct
from dataclasses import dataclass, field

# XTEST minor opcodes
XTestGetVersion = 0
XTestCompareCursor = 1
XTestFakeInput = 2

# X11 event types used with FakeInput
KeyPress = 2
KeyRelease = 3
ButtonPress = 4
ButtonRelease = 5
MotionNotify = 6

# XI1 event offsets relative to the XInput first-event base
XI_DeviceValuator = 0
XI_DeviceMotionNotify = 5

MORE_EVENTS = 0x80


@dataclass
class GetVersionRequest:
    """XTestGetVersion request."""

    opcode: int
    major: int = 2
    minor: int = 2

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH BxH",
            self.opcode,
            XTestGetVersion,
            2,
            self.major,
            self.minor,
        )


@dataclass
class XTestFakeInputRequest:
    """XTestFakeInput request.

    Wire format (xXTestFakeInputReq):
        CARD8    reqType         (XTEST major opcode)
        CARD8    xtReqType       (2)
        CARD16   length          (9)
        xEvent   fake_event      (32 bytes)
        CARD32   pad             (4 bytes, brings total to 36)

    The embedded xEvent has the standard X event layout.
    For MotionNotify with detail=1, it is a relative motion event.
    The root_x and root_y fields specify the motion delta for
    relative events.
    """

    opcode: int
    event_type: int = MotionNotify
    detail: int = 0  # 1 = relative for MotionNotify
    time: int = 0  # CurrentTime
    root_window: int = 0  # None
    root_x: int = 0
    root_y: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        # Build the 32-byte xEvent structure
        # xEvent layout for key/button/motion:
        #   type(1) + detail(1) + sequenceNumber(2) +
        #   time(4) + root(4) + event(4) + child(4) +
        #   rootX(2) + rootY(2) + eventX(2) + eventY(2) +
        #   state(2) + sameScreen(1) + pad(1)
        event = struct.pack(
            f"{byte_order}BBH I III hh hh HBx",
            self.event_type,
            self.detail,
            0,  # sequenceNumber (ignored)
            self.time,
            self.root_window,
            0,  # event window
            0,  # child window
            self.root_x,
            self.root_y,
            0,  # eventX
            0,  # eventY
            0,  # state
            0,  # sameScreen
        )

        # Request header + event + padding to make 36 bytes total
        header = struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            XTestFakeInput,
            9,  # 36 bytes = 9 words
        )
        return header + event + b"\x00" * (36 - 4 - len(event))


@dataclass
class DeviceValuatorEvent:
    """32-byte XI DeviceValuator event."""

    type: int
    deviceid: int
    first_valuator: int = 0
    num_valuators: int = 1
    valuators: list[int] = field(default_factory=list)

    def to_bytes(self, byte_order: str = "<") -> bytes:
        vals = list(self.valuators) + [0] * 6
        return struct.pack(
            f"{byte_order}BBH HBB iiiiii",
            self.type,
            self.deviceid,
            0,  # sequence
            0,  # device_state
            self.num_valuators,
            self.first_valuator,
            vals[0],
            vals[1],
            vals[2],
            vals[3],
            vals[4],
            vals[5],
        )


@dataclass
class DeviceMotionNotifyEvent:
    """32-byte XI DeviceMotionNotify event."""

    first_event: int
    deviceid: int
    detail: int = 0  # xFalse → absolute
    more_events: bool = False

    def to_bytes(self, byte_order: str = "<") -> bytes:
        deviceid = self.deviceid | (MORE_EVENTS if self.more_events else 0)
        return struct.pack(
            f"{byte_order}BBH IIII hhhh HBB",
            self.first_event + XI_DeviceMotionNotify,
            self.detail,
            0,  # sequence
            0,  # time
            0,  # root
            0,  # event
            0,  # child
            0,  # root_x
            0,  # root_y
            0,  # event_x
            0,  # event_y
            0,  # state
            0,  # same_screen
            deviceid,
        )


@dataclass
class FakeInputRequest:
    """XTestFakeInput request.

    The request is an xReq header plus one or more 32-byte xEvents.
    ``events`` is a chain of XI event objects; MORE_EVENTS is set on
    all but the last when serializing.
    """

    opcode: int
    events: list[DeviceMotionNotifyEvent | DeviceValuatorEvent]

    def serialize_event_chain(self, byte_order: str = "<") -> bytes:
        """Serialize the XI event chain, setting MORE_EVENTS on all but the last."""
        out = bytearray()
        for i, ev in enumerate(self.events):
            more = i < len(self.events) - 1
            if isinstance(ev, DeviceMotionNotifyEvent):
                ev = DeviceMotionNotifyEvent(
                    first_event=ev.first_event,
                    deviceid=ev.deviceid,
                    detail=ev.detail,
                    more_events=more,
                )
            elif isinstance(ev, DeviceValuatorEvent):
                deviceid = ev.deviceid & ~MORE_EVENTS
                if more:
                    deviceid |= MORE_EVENTS
                ev = DeviceValuatorEvent(
                    type=ev.type,
                    deviceid=deviceid,
                    first_valuator=ev.first_valuator,
                    num_valuators=ev.num_valuators,
                    valuators=ev.valuators,
                )
            out.extend(ev.to_bytes(byte_order))
        return bytes(out)

    def to_bytes(self, byte_order: str = "<") -> bytes:
        payload = self.serialize_event_chain(byte_order)
        assert len(payload) % 32 == 0
        length = 1 + len(payload) // 4
        return (
            struct.pack(
                f"{byte_order}BBH",
                self.opcode,
                XTestFakeInput,
                length,
            )
            + payload
        )
