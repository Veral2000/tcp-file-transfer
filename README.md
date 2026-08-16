# TCP File Transfer

Cross-platform TCP file transfer utility implemented in C++17 for the ATI Platform Engineer screening assignment.

## Current status

**v0.1.0 — streaming TCP transfer foundation**

Implemented:

- TCP client/server
- Linux and Windows socket abstraction
- Versioned application-layer framing
- Streaming file I/O
- 64-bit file sizes and offsets
- Files up to 16 GiB
- 4 MiB default transfer chunks
- Destination filename/path traversal protection
- Basic protocol and file I/O tests
- CMake + CTest build

Planned next:

- SHA-256 integrity verification
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

## Run

The application has two runtime roles: a long-running TCP server and a one-shot client.

### Server

Linux / WSL:

```bash
mkdir -p received
./build/ft-server 9000 ./received
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
- `CHUNK`
- `TRANSFER_COMPLETE`
- `ERROR`

TCP provides reliable, ordered byte-stream delivery. The application protocol is responsible for transfer semantics, framing, validation, integrity, and later resume/retry behavior.

## Design principles

1. Stream files instead of loading them into memory.
2. Use 64-bit file sizes/offsets for large-file support.
3. Keep platform-specific socket details behind one abstraction.
4. Validate protocol input before allocating or writing data.
5. Keep the protocol explicit and versioned so it can evolve.
6. Measure performance before introducing application-level pipelining.
7. Treat the server as a persistent service and the client as a one-shot transfer operation.

## Security roadmap

The current foundation validates protocol boundaries and destination filenames but does not yet provide confidentiality or peer authentication. TLS 1.3 and certificate validation are planned for the secure-transfer milestone.

## Assignment mapping

The ATI assignment asks for TCP transfer, files up to 16 GB, integrity checks, error handling, cross-platform compatibility, and optionally improved bandwidth utilization. It also requests source code, build/run documentation, unit tests, and a project-description document with architecture, design considerations, C4 diagrams, and performance metrics.
