# TCP File Transfer

Cross-platform TCP file transfer utility implemented in C++17 for the ATI Platform Engineer screening assignment.

## Current status

**v0.1.0 — streaming TCP transfer with end-to-end SHA-256 verification**

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
- Protocol validation and error handling
- C++ unit tests with CMake + CTest
- Python end-to-end integrity mismatch integration test
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

| Type | Purpose |
|---|---|
| `HELLO` | Starts a transfer session |
| `FILE_INFO` | Announces filename and expected file size |
| `FILE_HASH` | Announces expected SHA-256 digest |
| `CHUNK` | Carries a bounded section of file data |
| `TRANSFER_COMPLETE` | Indicates that all chunks have been sent |
| `ERROR` | Reports a protocol or transfer error |

TCP provides reliable, ordered byte-stream delivery. The application protocol is responsible for transfer semantics, framing, validation, integrity, and future resume/retry behavior.

## Resource limits

Current protocol/application limits are defined centrally in `include/common/Types.hpp`:

- Default chunk size: **4 MiB**
- Maximum chunk size: **16 MiB**
- Maximum file size: **16 GiB**
- Maximum protocol filename length: **4096 bytes**

Files are streamed rather than loaded completely into memory.

## Testing

### C++ unit tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The CTest target covers file I/O, protocol behavior, and SHA-256 known vectors/file hashing.

### Integrity mismatch integration test

The repository also contains a Python end-to-end negative test that sends the SHA-256 digest of one payload while transmitting a different payload. The server must detect the mismatch and remain available for subsequent client sessions.

```bash
python3 tests/integration/test_integrity_mismatch.py ./build/ft-server
```

Expected result:

```text
Integrity mismatch integration test passed.
```

## Design principles

1. Stream files instead of loading them into memory.
2. Use 64-bit file sizes/offsets for large-file support.
3. Keep platform-specific socket details behind one abstraction.
4. Validate protocol input before allocating or writing data.
5. Keep the protocol explicit and versioned so it can evolve.
6. Use end-to-end SHA-256 verification rather than relying only on TCP delivery guarantees.
7. Keep the server persistent and isolate individual client-session failures.
8. Measure performance before introducing application-level pipelining.

## Security considerations

The current implementation provides protocol validation, filename/path traversal protection, file-size limits, and end-to-end SHA-256 integrity verification. It does **not** provide confidentiality, encryption, or peer authentication.

TLS 1.3 with certificate validation is planned for a future secure-transfer milestone.

## CI

GitHub Actions builds and tests the project on both Linux and Windows. The CI pipeline validates the CMake build and CTest suite on each supported platform.

## Branch strategy

```text
main
├── feature/native-installation
├── feature/docker-deployment
└── feature/file-integrity
```

- `main` — stable, reviewed integration branch
- `feature/native-installation` — native installation/CLI work
- `feature/docker-deployment` — Docker/containerization work
- `feature/file-integrity` — SHA-256 and transfer-integrity work

## Docker

Docker deployment is intentionally separated from the native application branch. See `feature/docker-deployment` for the container image, Compose configuration, and Docker-specific run scripts.

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
