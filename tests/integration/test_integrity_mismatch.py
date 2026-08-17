#!/usr/bin/env python3
"""End-to-end negative test for SHA-256 transfer integrity.

The test deliberately sends the SHA-256 digest of one payload while sending
another payload. The server must detect the mismatch and report an integrity
failure.

Usage:
    python3 tests/integration/test_integrity_mismatch.py ./build/ft-server
"""

from __future__ import annotations

import hashlib
import os
from pathlib import Path
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time

MAGIC = 0x54435046  # "TCPF"
VERSION = 1
HEADER = struct.Struct("!I H B B Q")
FILE_INFO = struct.Struct("!Q H")
CHUNK = struct.Struct("!Q Q I")

HELLO = 1
FILE_INFO_TYPE = 2
CHUNK_TYPE = 3
TRANSFER_COMPLETE = 4
FILE_HASH = 6


def send_message(sock: socket.socket, message_type: int, payload: bytes = b"") -> None:
    sock.sendall(HEADER.pack(MAGIC, VERSION, message_type, 0, len(payload)))
    if payload:
        sock.sendall(payload)


def send_corrupted_transfer(port: int) -> None:
    original = b"ATI integrity test: original payload\n"
    corrupted = b"ATI integrity test: corrupted payload\n"
    filename = b"integrity-mismatch.txt"

    original_hash = hashlib.sha256(original).digest()

    with socket.create_connection(("127.0.0.1", port), timeout=5) as sock:
        send_message(sock, HELLO)

        file_info = FILE_INFO.pack(len(corrupted), len(filename)) + filename
        send_message(sock, FILE_INFO_TYPE, file_info)

        # Deliberately send the hash of the ORIGINAL payload while transmitting
        # the CORRUPTED payload. The server must reject this transfer.
        send_message(sock, FILE_HASH, original_hash)

        chunk = CHUNK.pack(0, 0, len(corrupted)) + corrupted
        send_message(sock, CHUNK_TYPE, chunk)
        send_message(sock, TRANSFER_COMPLETE)


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <ft-server-path>", file=sys.stderr)
        return 2

    server_path = Path(sys.argv[1]).resolve()
    if not server_path.is_file():
        print(f"Error: server executable not found: {server_path}", file=sys.stderr)
        return 2

    port = 19001
    temp_root = Path(tempfile.mkdtemp(prefix="tcpft-integrity-"))
    output_dir = temp_root / "received"
    output_dir.mkdir()

    server = subprocess.Popen(
        [str(server_path), str(port), str(output_dir)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    try:
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if server.poll() is not None:
                stdout, stderr = server.communicate()
                raise RuntimeError(
                    "ft-server exited before accepting connections:\n"
                    f"stdout:\n{stdout}\n"
                    f"stderr:\n{stderr}"
                )
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                    break
            except OSError:
                time.sleep(0.05)
        else:
            raise RuntimeError("timed out waiting for ft-server")

        send_corrupted_transfer(port)

        deadline = time.monotonic() + 5
        stderr = ""
        while time.monotonic() < deadline:
            if server.poll() is not None:
                break
            time.sleep(0.05)

        # The server is expected to remain alive after rejecting one client.
        if server.poll() is not None:
            stdout, stderr = server.communicate()
            raise RuntimeError(
                "ft-server terminated after integrity failure:\n"
                f"stdout:\n{stdout}\n"
                f"stderr:\n{stderr}"
            )

        # Give the server a short period to flush its error message.
        time.sleep(0.1)
        if server.stderr is not None:
            try:
                stderr = os.read(server.stderr.fileno(), 8192).decode(errors="replace")
            except (BlockingIOError, OSError):
                stderr = ""

        if "SHA-256 integrity check failed" not in stderr:
            raise RuntimeError(
                "server did not report the expected SHA-256 integrity failure:\n"
                + stderr
            )

        print("Integrity mismatch integration test passed.")
        return 0

    finally:
        if server.poll() is None:
            server.terminate()
            try:
                server.wait(timeout=2)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait()
        shutil.rmtree(temp_root, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
