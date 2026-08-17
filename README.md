# TCP File Transfer

Cross-platform TCP file transfer utility implemented in C++17 for the ATI Platform Engineer screening assignment.

## Current status

**v0.2.0 — streaming TCP transfer with SHA-256 integrity verification**

Implemented:

- TCP client/server
- Linux and Windows socket abstraction
- Versioned application-layer framing
- Streaming file I/O
- 64-bit file sizes and offsets
- Files up to 16 GiB
- 4 MiB default transfer chunks
- Destination filename/path traversal protection
- SHA-256 file integrity verification
- Protocol message for transmitting the expected SHA-256 digest
- C++ unit tests for file I/O, protocol parsing, and SHA-256 known vectors
- Python end-to-end negative integration test for SHA-256 mismatch detection
- CMake + CTest build
- Native installation of `ft-client` and `ft-server`

Planned next:

- Per-chunk integrity and acknowledgements
- Retry and resumable transfers
- TLS 1.3 transport security
- Compression policy
- Performance benchmarking and bandwidth optimization
- Expanded integration/failure testing

> Container deployment is maintained separately on the `docker` branch so `main` remains focused on the native application.

## Build

### Linux / WSL

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Windows

Use Visual Studio 2022 or another CMake-capable generator:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Install

The native client and server can be installed system-wide using CMake. The default installation prefix is `/usr/local`, so the executables are installed into `/usr/local/bin` on Linux/WSL.

### Linux / WSL

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

Verify the installation:

```bash
which ft-client
which ft-server
```

Expected:

```text
/usr/local/bin/ft-client
/usr/local/bin/ft-server
```

The utilities can then be executed from any working directory:

```bash
ft-server 9000 ./received
ft-client send ./test.bin 127.0.0.1:9000
```

To install under `/usr/bin` instead, explicitly select `/usr` as the CMake installation prefix:

```bash
sudo cmake --install build --prefix /usr
```

> `/usr/local/bin` is recommended for this project because it keeps locally built software separate from files managed by the operating-system package manager.

## Run

The application has two runtime roles: a long-running TCP server and a one-shot client.

### Server

Linux / WSL:

```bash
mkdir -p received
./build/ft-server 9000 ./received
```

After installation, the server can also be started from anywhere:

```bash
mkdir -p received
ft-server 9000 ./received
```

Windows:

```powershell
mkdir received
.\build\Release\ft-server.exe 9000 .\received
```

The server listens for incoming TCP connections and writes received files below the configured output directory.

### Client

Linux / WSL:

```bash
./build/ft-client send ./test.bin 127.0.0.1:9000
```

After installation:

```bash
ft-client send ./test.bin 127.0.0.1:9000
```

Windows:

```powershell
.\build\Release\ft-client.exe send .\test.bin 127.0.0.1:9000
```

For a remote server, replace `127.0.0.1` with the server's reachable IP address or hostname.

## Server/client deployment model

```text
                 TCP
      +--------------------------+
      |                          |
      v                          v
+-------------+            +-------------+
|   SERVER    |            |   CLIENT    |
| long-running|            | one-shot     |
| service     |            | transfer     |
+-------------+            +-------------+
```

The server is a persistent process because it listens for incoming transfers. The client normally starts for a transfer, completes it, and exits.

## Protocol

The wire protocol currently uses a fixed 16-byte header:

```text
+----------+---------+------+----------+----------------+
| Magic    | Version | Type | Reserved | Payload Size   |
| uint32   | uint16  | u8   | u8       | uint64         |
+----------+---------+------+----------+----------------+
```

All multi-byte fields are encoded in network byte order (big-endian).

Messages currently supported:

- `HELLO`
- `FILE_INFO`
- `FILE_HASH`
- `CHUNK`
- `TRANSFER_COMPLETE`
- `ERROR`

### SHA-256 integrity flow

The client calculates the SHA-256 digest of the source file before transmission and sends it in the `FILE_HASH` message. The server receives the digest, writes the streamed file, calculates the SHA-256 digest of the received file, and compares the two values.

```text
Client                              Server
  |                                   |
  | FILE_INFO                         |
  |---------------------------------->| 
  |                                   |
  | FILE_HASH = SHA256(source)        |
  |---------------------------------->| 
  |                                   |
  | CHUNK data                        |
  |---------------------------------->| 
  |                                   |
  | TRANSFER_COMPLETE                 |
  |---------------------------------->| 
  |                                   |
  |                         SHA256(received)
  |                         compare hashes
  |                                   |
  |                         MATCH -> PASS
  |                         MISMATCH -> ERROR
```

TCP provides reliable, ordered byte-stream delivery. The application protocol is responsible for transfer semantics, framing, validation, integrity, and later resume/retry behavior.

## Testing

### C++ unit tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The C++ test executable covers:

- File round-trip I/O
- Oversized-file rejection
- Protocol message parsing
- SHA-256 known vectors
- Incremental SHA-256 updates
- SHA-256 file hashing
- File-hash protocol payload validation

### SHA-256 integrity mismatch integration test

The repository also contains a Python end-to-end negative test at:

```text
tests/integration/test_integrity_mismatch.py
```

It starts `ft-server`, sends the SHA-256 digest of an original payload while deliberately transmitting a different payload, and verifies that the server reports an integrity failure. The test uses the real application protocol rather than mocking the hashing logic.

Run it after building the server:

```bash
python3 tests/integration/test_integrity_mismatch.py ./build/ft-server
```

Expected output:

```text
Integrity mismatch integration test passed.
```

The integration test is intentionally a negative-path test and is separate from the C++ unit-test suite. It is currently intended to run on Linux/WSL where the build and runtime environment are already available.

## Design principles

1. Stream files instead of loading them into memory.
2. Use 64-bit file sizes/offsets for large-file support.
3. Keep platform-specific socket details behind one abstraction.
4. Validate protocol input before allocating or writing data.
5. Keep the protocol explicit and versioned so it can evolve.
6. Measure performance before introducing application-level pipelining.
7. Treat the server as a persistent service and the client as a one-shot transfer operation.
8. Verify received content independently from TCP transport reliability.

## Security roadmap

The current foundation validates protocol boundaries, destination filenames, and file integrity but does not yet provide confidentiality or peer authentication. TLS 1.3 and certificate validation are planned for the secure-transfer milestone.

## Assignment mapping

The ATI assignment asks for TCP transfer, files up to 16 GB, integrity checks, error handling, cross-platform compatibility, and optionally improved bandwidth utilization. It also requests source code, build/run documentation, unit tests, and a project-description document with architecture, design considerations, C4 diagrams, and performance metrics.
