# SPDX-License-Identifier: MIT
#
# Security tests for XVideo extension vulnerabilities.

import ctypes
import struct

import pytest
from proto import shm, x11, xv
from xclient import BadAccess, BadValue, Extension, X11Error, X11Reply

# SysV IPC constants
IPC_CREAT = 0o1000
IPC_PRIVATE = 0
IPC_RMID = 0


@pytest.fixture
def xv_port(xclient):
    """Return (xclient, xv_opcode, port_id) for an image-capable Xv port."""
    ext = xclient.query_extension(Extension.XVIDEO)
    if not ext:
        pytest.skip("XVideo extension not available")

    xclient.send_request(xv.QueryExtensionRequest(opcode=ext.opcode))
    resp = xclient.recv_response(timeout=5.0)
    if not isinstance(resp, X11Reply):
        pytest.skip("XvQueryExtension failed")

    xclient.send_request(
        xv.QueryAdaptorsRequest(opcode=ext.opcode, window=xclient.root_window)
    )
    resp = xclient.recv_response(timeout=5.0)
    if not isinstance(resp, X11Reply):
        pytest.skip("XvQueryAdaptors failed")

    bo = xclient._byte_order
    num_adaptors = struct.unpack_from(f"{bo}H", resp.data, 8)[0]
    if num_adaptors == 0:
        pytest.skip("No Xv adaptors (needs glamor Xv, e.g. Xwayland)")

    # Reply body follows the 32-byte header
    body = resp.data[32:]
    offset = 0
    for _ in range(num_adaptors):
        if offset + 12 > len(body):
            break
        base_id, name_size, num_ports, num_formats, atype = struct.unpack_from(
            f"{bo}IHHHB", body, offset
        )
        offset += 12
        offset += (name_size + 3) & ~3
        offset += num_formats * 8

        if (atype & xv.XvImageMask) and (atype & xv.XvInputMask) and num_ports > 0:
            return xclient, ext.opcode, base_id

    pytest.skip("No image-capable Xv adaptor found")


class TestXvShmPutImage:
    def test_offset_wrap(self, xserver, xv_port):
        """
        ProcXvShmPutImage compared (size_needed + offset) > shmdesc->size
        with mixed signed/unsigned arithmetic. A negative size_needed plus
        a crafted offset wraps to 0 and bypasses the bounds check.

        Reject negative sizes and compare with subtraction.
        """
        xclient, opcode, port = xv_port
        wid = xclient.create_window(width=64, height=64)
        gcid = xclient.alloc_id()
        xclient.send_request(x11.CreateGCRequest(cid=gcid, drawable=wid))
        xclient.flush_responses(timeout=0.2)

        shm_ext = xclient.query_extension(Extension.MIT_SHM)
        if not shm_ext:
            pytest.skip("MIT-SHM extension not available")

        libc = ctypes.CDLL("libc.so.6", use_errno=True)
        shmid = libc.shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0o600)
        if shmid < 0:
            raise OSError(ctypes.get_errno(), "shmget(4096) failed")
        shmseg = xclient.alloc_id()
        try:
            xclient.send_request(
                shm.AttachRequest(
                    opcode=shm_ext.opcode,
                    shmseg=shmseg,
                    shmid=shmid,
                    read_only=False,
                )
            )
            err = xclient.recv_response(timeout=0.3)
            if isinstance(err, X11Error):
                pytest.skip(f"ShmAttach failed: {err}")

            # offset=4 cancels size_needed=-4 (unfixed NV12 math); also
            # cancels size_needed=-1 from the fixed glamor sentinel.
            xclient.send_request(
                xv.ShmPutImageRequest(
                    opcode=opcode,
                    port=port,
                    drawable=wid,
                    gc=gcid,
                    shmseg=shmseg,
                    id=xv.FOURCC_NV12,
                    offset=4,
                    width=xv.OVERFLOW_WIDTH,
                    height=xv.OVERFLOW_HEIGHT,
                    src_w=1,
                    src_h=1,
                    drw_w=1,
                    drw_h=1,
                )
            )
            resp = xclient.recv_response(timeout=5.0)

            assert xserver.is_alive, "Server crashed on XvShmPutImage overflow"
            assert isinstance(resp, X11Error), f"Expected BadAccess, got {resp}"
            assert resp.error_code == BadAccess, (
                f"Expected BadAccess ({BadAccess}), got {resp.error_code}"
            )
        finally:
            libc.shmctl(shmid, IPC_RMID, None)


class TestXvPutImage:
    def test_negative_size(self, xserver, xv_port):
        """
        When ddQueryImageAttributes() returns a negative size, adding the
        PutImage header size can make the length check accept a request
        that contains no image payload (e.g. size=-1 → required length 10).

        Reject negative sizes with BadValue before length validation.
        """
        xclient, opcode, port = xv_port
        wid = xclient.create_window(width=64, height=64)
        gcid = xclient.alloc_id()
        xclient.send_request(x11.CreateGCRequest(cid=gcid, drawable=wid))
        xclient.flush_responses(timeout=0.2)

        # Header-only PutImage (40 bytes / 10 words) — the pre-fix
        # overflow path accepted this for NV12 43690x65534.
        xclient.send_request(
            xv.PutImageRequest(
                opcode=opcode,
                port=port,
                drawable=wid,
                gc=gcid,
                id=xv.FOURCC_NV12,
                width=xv.OVERFLOW_WIDTH,
                height=xv.OVERFLOW_HEIGHT,
                src_w=1,
                src_h=1,
                drw_w=1,
                drw_h=1,
                data=b"",
                length_override=10,
            )
        )
        resp = xclient.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed on XvPutImage size overflow"
        assert isinstance(resp, X11Error), f"Expected BadValue, got {resp}"
        assert resp.error_code == BadValue, (
            f"Expected BadValue ({BadValue}), got {resp.error_code}"
        )


class TestXvQueryImageAttributes:
    def test_yv12_size_overflow(self, xserver, xv_port):
        """
        glamor_xv_query_image_attributes() used 32-bit ints for size math.
        YV12 with w=43690, h=65534 overflows to a *positive* wrong size
        (~131064) instead of the true ~4GB, so a size < 0 protocol check
        alone cannot catch it.

        With the fix the adaptor returns -1 and QueryImageAttributes
        rejects it with BadValue.
        """
        xclient, opcode, port = xv_port

        xclient.send_request(
            xv.QueryImageAttributesRequest(
                opcode=opcode,
                port=port,
                id=xv.FOURCC_YV12,
                width=xv.OVERFLOW_WIDTH,
                height=xv.OVERFLOW_HEIGHT,
            )
        )
        resp = xclient.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed on YV12 size overflow"
        assert isinstance(resp, X11Error), f"Expected BadValue, got {resp}"
        assert resp.error_code == BadValue, (
            f"Expected BadValue ({BadValue}), got {resp.error_code}"
        )

    def test_negative_size(self, xserver, xv_port):
        """
        When ddQueryImageAttributes() returns a negative size, the reply
        would cast it to CARD32 and report a bogus data_size.

        Reject negative sizes with BadValue.
        """
        xclient, opcode, port = xv_port

        xclient.send_request(
            xv.QueryImageAttributesRequest(
                opcode=opcode,
                port=port,
                id=xv.FOURCC_NV12,
                width=xv.OVERFLOW_WIDTH,
                height=xv.OVERFLOW_HEIGHT,
            )
        )
        resp = xclient.recv_response(timeout=5.0)

        assert xserver.is_alive, "Server crashed on NV12 QueryImageAttributes"
        assert isinstance(resp, X11Error), f"Expected BadValue, got {resp}"
        assert resp.error_code == BadValue, (
            f"Expected BadValue ({BadValue}), got {resp.error_code}"
        )
