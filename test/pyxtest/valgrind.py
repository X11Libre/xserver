# SPDX-License-Identifier: MIT
#
# Valgrind XML output parser for extracting memory errors and suppressions.

from __future__ import annotations

import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


def _suppression_from_element(elem: ET.Element) -> str:
    """Format a ``<suppression>`` XML element as a suppression file entry."""
    name = elem.findtext("sname", "")
    kind = elem.findtext("skind", "")
    auxiliary = elem.findtext("skaux")

    lines = ["{", f"   {name}", f"   {kind}"]
    if auxiliary:
        lines.append(f"   {auxiliary}")
    for sframe in elem.iter("sframe"):
        fun = sframe.findtext("fun")
        obj = sframe.findtext("obj")
        if fun is not None:
            lines.append(f"   fun:{fun}")
        elif obj is not None:
            lines.append(f"   obj:{obj}")
        else:
            lines.append("   ...")
    lines.append("}")
    return "\n".join(lines)


@dataclass
class ValgrindError:
    """Represents a single valgrind error extracted from XML output."""

    kind: str  # e.g. "InvalidRead", "InvalidWrite"
    what: str  # human-readable description
    stack_frames: list[tuple[str, str | None, str | None]]  # (func, file, line)
    suppression: str | None = None  # ready-to-paste suppression file entry

    def __str__(self):
        lines = [f"{self.kind}: {self.what}"]
        for func, srcfile, line in self.stack_frames[:8]:
            loc = f" ({srcfile}:{line})" if srcfile else ""
            lines.append(f"    at {func}{loc}")
        if self.suppression:
            lines.append("")
            lines.append(self.suppression)
        return "\n".join(lines)

    @classmethod
    def from_xml(cls, xml_path: Path) -> list[ValgrindError]:
        """Parse valgrind XML output and return a list of ValgrindError."""
        if not xml_path.is_file():
            return []

        try:
            tree = ET.parse(xml_path)
        except ET.ParseError:
            return []

        errors = []
        root = tree.getroot()

        for error_elem in root.iter("error"):
            kind_elem = error_elem.find("kind")
            kind = kind_elem.text if kind_elem is not None else "Unknown"
            assert kind is not None

            what_elem = error_elem.find("what")
            if what_elem is None:
                what_elem = error_elem.find("xwhat/text")
            what = what_elem.text if what_elem is not None else "Unknown error"
            assert what is not None

            frames = []
            stack_elem = error_elem.find("stack")
            if stack_elem is not None:
                for frame in stack_elem.iter("frame"):
                    fn = frame.findtext("fn", "???")
                    srcfile = frame.findtext("file", "")
                    line = frame.findtext("line", "")
                    frames.append((fn, srcfile, line))

            suppression = None
            supp_elem = error_elem.find("suppression")
            if supp_elem is not None:
                suppression = _suppression_from_element(supp_elem)

            errors.append(ValgrindError(kind, what, frames, suppression))

        return errors
