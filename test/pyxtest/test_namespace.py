# SPDX-License-Identifier: MIT
"""Runtime coverage for server-wide XNamespace component configuration.

Also runnable without pytest: XVFB_PATH=... python3 test_namespace.py
"""

import struct
import tempfile
import unittest
from contextlib import contextmanager
from pathlib import Path

from xclient import RawX11Connection, X11Error, X11Reply
from xserver import XServerProcess


class TestNamespaceComponents(unittest.TestCase):
    @contextmanager
    def namespace_clients(self, directives="", swapped=False):
        with tempfile.TemporaryDirectory() as directory:
            config = Path(directory) / "ns.conf"
            config.write_text(
                "namespace root\nauth MIT-MAGIC-COOKIE-1 " + "11" * 16 + "\n"
                "namespace first\nauth MIT-MAGIC-COOKIE-1 " + "22" * 16 + "\n"
                "namespace second\nauth MIT-MAGIC-COOKIE-1 " + "33" * 16 + "\n"
                + directives
            )
            server = XServerProcess(extra_args=["-namespace", str(config)])
            clients = []
            try:
                server.start()
                for value in (0x11, 0x22, 0x33):
                    clients.append(RawX11Connection(
                        server.display_num, swapped=swapped,
                        auth_name=b"MIT-MAGIC-COOKIE-1", auth_data=bytes([value]) * 16,
                    ))
                yield clients
                self.assertTrue(server.is_alive, "Server crashed")
            finally:
                for client in clients:
                    client.close()
                server.stop()

    def selection_owner(self, client, selection):
        client.send_request(struct.pack(f"{client._byte_order}BBHI", 23, 0, 2, selection))
        reply = client.recv_response()
        self.assertIsInstance(reply, X11Reply)
        return struct.unpack_from(f"{client._byte_order}I", reply.data, 8)[0]

    def test_clipboard_switches(self):
        cases = [
            ("", False),
            ("disable clipboard\n", True),
            ("disable clipboard\nenable clipboard\n", False),
            ("enable clipboard\ndisable clipboard\n", True),
            # Multiple names, whitespace, comments, CRLF, unknown names and
            # missing arguments must not interfere with recognized settings.
            ("disable\ndisable unknown\n"
             "disable devices\tclipboard events # comment\r\n", True),
        ]
        for directives, shared in cases:
            for swapped in (False, True):
                with self.subTest(directives=directives, swapped=swapped):
                    with self.namespace_clients(directives, swapped) as (_, first, second):
                        window = first.create_window()
                        for name in ("PRIMARY", "SECONDARY", "CLIPBOARD"):
                            selection = first.intern_atom(name)
                            first.send_request(struct.pack(
                                f"{first._byte_order}BBHIII", 22, 0, 4,
                                window, selection, 0,
                            ))
                            self.assertEqual(self.selection_owner(first, selection), window)
                            self.assertEqual(
                                self.selection_owner(second, selection), window if shared else 0,
                            )

    def test_resource_switch(self):
        for directives, allowed in (("", False), ("disable resources\n", True)):
            with self.subTest(directives=directives):
                with self.namespace_clients(directives) as (_, first, second):
                    window = second.create_window()
                    first.send_request(struct.pack("<BBHI", 15, 0, 2, window))  # QueryTree
                    reply = first.recv_response()
                    if allowed:
                        self.assertIsInstance(reply, X11Reply)
                    else:
                        self.assertIsInstance(reply, X11Error)
                        self.assertEqual(reply.error_code, 10)  # BadAccess

    def test_extension_switch_keeps_management_private(self):
        for directives, xtest_visible in (("", False), ("disable extensions\n", True)):
            with self.subTest(directives=directives):
                with self.namespace_clients(directives) as (root, first, _):
                    management = root.query_extension("X-NAMESPACE")
                    self.assertIsNotNone(management)
                    self.assertIsNone(first.query_extension("X-NAMESPACE"))
                    self.assertEqual(first.query_extension("XTEST") is not None, xtest_visible)
                    # Even guessing the opcode cannot bypass management authorization.
                    first.send_request(struct.pack("<BBHHH", management.opcode, 0, 2, 1, 0))
                    error = first.recv_response()
                    self.assertIsInstance(error, X11Error)
                    self.assertEqual(error.error_code, 10)  # BadAccess


if __name__ == "__main__":
    unittest.main()
