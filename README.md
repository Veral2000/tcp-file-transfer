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
- GitHub Actions CI for Linux and Windows builds/tests
- Separate Docker deployment branch

Planned next:

- Per-chunk integrity and acknowledgements
- Retry and resumable transfers
- TLS 1.3 transport security
- Compression policy
- Performance benchmarking and bandwidth optimization
- Expanded integration/failure testing

> Container deployment is maintained separately on `feature/docker-deployment` so `main` remains focused on the native application.

## Architecture

The implementation is organized into a small set of focused layers:

```text
                         TCP File Transfer
                                |
              +-----------------+-----------------+
              |                                   |
          ft-client                           ft-server
              |                                   |
        +-----+------+                     +------+-----+
        | FileReader |                     | FileWriter |
        +-----+------+                     +------+-----+
              |                                   |
        +-----+------+                     +------+-----+
        |  SHA-256   |<---- FILE_HASH ---->|  SHA-256   |
        +-----+------+                     +------+-----+
              |                                   |
              +---------- Protocol ---------------+
                              |
                         TcpSocket
                              |
                         TCP / OS API
```

The detailed architecture, component responsibilities, protocol flow, design trade-offs, security considerations, and evolution plan are documented in [`docs/architecture.md`](docs/architecture.md).

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

> `/usr/local/bin` is recommended because it keeps locally built software separate from files managed by the operating-system package manager.

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

## Transfer and integrity flow

The client calculates the SHA-256 digest of the source file before transmission and sends it to the server in a `FILE_HASH` protocol message. The file itself is then streamed as bounded-size `CHUNK` messages.

```text
Client                                      Server
  |                                           |
  |------------- HELLO --------------------->|
  |------------- FILE_INFO ----------------->|
  |------------- FILE_HASH ----------------->|
  |------------- CHUNK --------------------->|
  |------------- CHUNK --------------------->|
  |                  ...                      |
  |-------- TRANSFER_COMPLETE -------------->|
  |                                           |
  |                         hash received file|
  |                         compare SHA-256   |
  |                                           |
  |              PASS / integrity failure    |
```

The server verifies both the announced file size and the final SHA-256 digest. A mismatch is treated as a failed client session and does not terminate the persistent server process.

The current implementation finalizes the destination file before performing the final hash comparison. Therefore, a failed integrity check can leave the received file on disk. Atomic temporary-file handling (`.part` + rename after successful verification) is a planned hardening step.

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

## Docker

The current foundation validates protocol boundaries, destination filenames, and file integrity but does not yet provide confidentiality or peer authentication. TLS 1.3 and certificate validation are planned for the secure-transfer milestone.

## Assignment mapping

The implementation addresses the core assignment requirements:

| Requirement | Implementation |
|---|---|
| TCP file transfer | `ft-client` / `ft-server` over TCP |
| Large files | 64-bit sizes/offsets, 16 GiB application limit |
| Integrity | End-to-end SHA-256 verification |
| Error handling | Protocol/session validation and isolated client failures |
| Cross-platform | POSIX sockets and Windows Winsock behind `TcpSocket` |
| Efficient memory use | Bounded 4 MiB streaming chunks |
| Testing | CMake/CTest unit tests + Python integration test |
| Build | CMake |
| Documentation | README + architecture/design document |
| Containerization | Separate Docker deployment branch |

Optional bandwidth optimization such as pipelining/windowing, performance benchmarking, retry/resume, and TLS are intentionally treated as subsequent milestones rather than mixed into the initial transfer foundation.
