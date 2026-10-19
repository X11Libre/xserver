# SPDX-License-Identifier: MIT
#
# XTEST extension protocol request builders for security testing.

import struct
from dataclasses import dataclass

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
