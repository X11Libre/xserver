# SPDX-License-Identifier: MIT
#
# AddressSanitizer (ASAN) output parser for extracting memory errors.

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


@dataclass
class AsanError:
    """Represents a single ASAN error extracted from log/stderr output."""

    kind: str  # e.g. "heap-buffer-overflow", "heap-use-after-free"
    description: str  # The full ERROR line
    stack_frames: list[tuple[str, str | None, str | None]]  # (func, file, line)

    def __str__(self):
        lines = [f"{self.kind}: {self.description}"]
        for func, srcfile, line in self.stack_frames[:8]:
            loc = f" ({srcfile}:{line})" if srcfile else ""
            lines.append(f"    at {func}{loc}")
        return "\n".join(lines)

    @classmethod
    def from_log(cls, log_path: Path) -> list[AsanError]:
        """Parse ASAN output from a log file.

        ASAN may append a PID suffix to the log_path, so we glob for
        matching files (e.g. log_path.1234).
        """
        errors: list[AsanError] = []

        # ASAN appends .<pid> to the log_path
        candidates = list(log_path.parent.glob(f"{log_path.name}.*"))
        if log_path.is_file():
            candidates.append(log_path)

        for path in candidates:
            try:
                text = path.read_text(errors="replace")
            except OSError:
                continue
            errors.extend(cls.from_text(text))

        return errors

    @classmethod
    def from_text(cls, text: str) -> list[AsanError]:
        """Parse ASAN errors from text output (stderr or log file).

        Recognises the standard ASAN report format::

            ==PID==ERROR: AddressSanitizer: heap-buffer-overflow on ...
            READ of size 4 at 0x... thread T0
                #0 0xaddr in func file.c:123
                #1 0xaddr in func2 file2.c:456

            SUMMARY: AddressSanitizer: heap-buffer-overflow ...
        """
        errors: list[AsanError] = []

        # Pattern for the ERROR line
        error_re = re.compile(r"==\d+==ERROR: AddressSanitizer: ([\w-]+)(.*)")
        # Pattern for stack frames:
        #   #0 0x... in function_name file.c:123:45
        #   #0 0x... in function_name (module+0xoffset)
        #   #0 0x... (module+0xoffset)
        frame_re = re.compile(
            r"\s+#\d+\s+\S+\s+in\s+(\S+)\s+(\S+?)(?::(\d+))?(?::\d+)?\s*$"
        )
        # Frame without source info (just address in module)
        frame_nosrc_re = re.compile(r"\s+#\d+\s+\S+\s+in\s+(\S+)")

        lines = text.splitlines()
        i = 0
        while i < len(lines):
            m = error_re.search(lines[i])
            if not m:
                i += 1
                continue

            kind = m.group(1)
            description = m.group(2).strip()

            # Collect stack frames following the ERROR line
            frames: list[tuple[str, str | None, str | None]] = []
            i += 1
            while i < len(lines):
                line = lines[i]
                # Stop at blank lines, SUMMARY lines, or new ERROR lines
                if not line.strip() or line.strip().startswith("SUMMARY:"):
                    break
                if error_re.search(line):
                    break

                fm = frame_re.match(line)
                if fm:
                    func = fm.group(1)
                    srcfile = fm.group(2)
                    lineno = fm.group(3)
                    # Filter out non-file entries like (module+0xoffset)
                    if srcfile and srcfile.startswith("("):
                        srcfile = None
                        lineno = None
                    frames.append((func, srcfile, lineno))
                else:
                    fm2 = frame_nosrc_re.match(line)
                    if fm2:
                        frames.append((fm2.group(1), None, None))
                i += 1

            errors.append(cls(kind=kind, description=description, stack_frames=frames))

        return errors
