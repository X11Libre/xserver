# SPDX-License-Identifier: MIT
#
# SYNC extension protocol request builders.

import struct
from dataclasses import dataclass

# SYNC minor opcodes
SyncInitialize = 0
SyncCreateCounter = 2
SyncSetCounter = 3
SyncChangeCounter = 4
SyncDestroyCounter = 6
SyncAwait = 7
SyncCreateAlarm = 8
SyncQueryAlarm = 10
SyncCreateFence = 14
SyncTriggerFence = 15
SyncResetFence = 16
SyncDestroyFence = 17
SyncAwaitFence = 19

# SYNC alarm value mask bits
SyncCACounter = 1 << 0
SyncCAValueType = 1 << 1
SyncCAValue = 1 << 2
SyncCATestType = 1 << 3
SyncCADelta = 1 << 4
SyncCAEvents = 1 << 5

# Value types
SyncAbsolute = 0
SyncRelative = 1

# Test types
SyncPositiveTransition = 0
SyncNegativeTransition = 1
SyncPositiveComparison = 2
SyncNegativeComparison = 3


@dataclass
class InitializeRequest:
    """SyncInitialize request."""

    opcode: int
    major: int = 3
    minor: int = 1

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBHBB xx",
            self.opcode,
            SyncInitialize,
            2,  # 8 bytes = 2 words
            self.major,
            self.minor,
        )


@dataclass
class CreateAlarmRequest:
    """SyncCreateAlarm request.

    value_mask selects which attributes are present in the value list.
    """

    opcode: int
    alarm_id: int
    value_mask: int = 0
    values: bytes = b""

    def to_bytes(self, byte_order: str = "<") -> bytes:
        total = 12 + len(self.values)
        pad_len = (4 - total % 4) % 4
        length = (total + pad_len) // 4

        header = struct.pack(
            f"{byte_order}BBH I I",
            self.opcode,
            SyncCreateAlarm,
            length,
            self.alarm_id,
            self.value_mask,
        )
        return header + self.values + b"\x00" * pad_len


@dataclass
class QueryAlarmRequest:
    """SyncQueryAlarm request."""

    opcode: int
    alarm_id: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            SyncQueryAlarm,
            2,
            self.alarm_id,
        )


@dataclass
class CreateCounterRequest:
    """SyncCreateCounter request (16 bytes)."""

    opcode: int
    counter_id: int
    initial_value_hi: int = 0
    initial_value_lo: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I iI",
            self.opcode,
            SyncCreateCounter,
            4,  # length = 4 words
            self.counter_id,
            self.initial_value_hi,
            self.initial_value_lo,
        )


@dataclass
class SetCounterRequest:
    """SyncSetCounter request (16 bytes)."""

    opcode: int
    counter_id: int
    value_hi: int = 0
    value_lo: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I iI",
            self.opcode,
            SyncSetCounter,
            4,  # length = 4 words
            self.counter_id,
            self.value_hi,
            self.value_lo,
        )


@dataclass
class DestroyCounterRequest:
    """SyncDestroyCounter request (8 bytes)."""

    opcode: int
    counter_id: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            SyncDestroyCounter,
            2,  # length = 2 words
            self.counter_id,
        )


@dataclass
class AwaitRequest:
    """SyncAwait request with variable number of wait conditions.

    Each wait condition is 28 bytes:
      counter(4) + value_type(4) + wait_value_hi(4) + wait_value_lo(4) +
      test_type(4) + event_threshold_hi(4) + event_threshold_lo(4)
    """

    opcode: int
    conditions: list[tuple[int, int, int, int, int, int, int]]
    """List of (counter, value_type, wait_hi, wait_lo, test_type, thresh_hi, thresh_lo)"""

    def to_bytes(self, byte_order: str = "<") -> bytes:
        n = len(self.conditions)
        total_bytes = 4 + n * 28
        length = total_bytes // 4

        header = struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            SyncAwait,
            length,
        )

        payload = b""
        for (
            counter,
            vtype,
            wait_hi,
            wait_lo,
            ttype,
            thresh_hi,
            thresh_lo,
        ) in self.conditions:
            payload += struct.pack(
                f"{byte_order}I I iI I iI",
                counter,
                vtype,
                wait_hi,
                wait_lo,
                ttype,
                thresh_hi,
                thresh_lo,
            )

        return header + payload


@dataclass
class CreateFenceRequest:
    """SyncCreateFence request (16 bytes)."""

    opcode: int
    drawable: int
    fence_id: int
    initially_triggered: int = 0

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I I B xxx",
            self.opcode,
            SyncCreateFence,
            4,  # length = 4 words
            self.drawable,
            self.fence_id,
            self.initially_triggered,
        )


@dataclass
class DestroyFenceRequest:
    """SyncDestroyFence request (8 bytes)."""

    opcode: int
    fence_id: int

    def to_bytes(self, byte_order: str = "<") -> bytes:
        return struct.pack(
            f"{byte_order}BBH I",
            self.opcode,
            SyncDestroyFence,
            2,  # length = 2 words
            self.fence_id,
        )


@dataclass
class AwaitFenceRequest:
    """SyncAwaitFence request with variable number of fence IDs."""

    opcode: int
    fence_ids: list[int]

    def to_bytes(self, byte_order: str = "<") -> bytes:
        n = len(self.fence_ids)
        length = 1 + n  # 1 word header + n words of fence IDs

        header = struct.pack(
            f"{byte_order}BBH",
            self.opcode,
            SyncAwaitFence,
            length,
        )

        payload = b""
        for fid in self.fence_ids:
            payload += struct.pack(f"{byte_order}I", fid)

        return header + payload
