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
- Multi-stage Docker image
- Non-root container runtime
- Production and development Docker Compose configurations
- Container healthcheck
- Unified `run.sh` deployment helper

Planned next:

- SHA-256 integrity verification
- Per-chunk integrity and acknowledgements
- Retry and resumable transfers
- TLS 1.3 transport security
- Compression policy
- Performance benchmarking and bandwidth optimization
- Expanded integration/failure testing

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

## Unified deployment helper

`run.sh` provides one entry point for native and containerized deployment. It treats the **server as a long-running service** and the **client as a one-shot transfer command**.

Make it executable once:

```bash
chmod +x run.sh
```

Show available commands:

```bash
./run.sh help
```

### Native server

```bash
./run.sh build
./run.sh server 9000 ./received
```

The server stays running and waits for transfer clients.

### Native client

The client is intentionally not a daemon. It starts, transfers one file, and exits:

```bash
./run.sh client ./test.bin 127.0.0.1:9000
```

For a remote server:

```bash
./run.sh client ./test.bin 192.168.1.50:9000
```

### Docker server

Build and deploy the production-style server:

```bash
./run.sh docker-build
./run.sh docker-server
```

The server runs as a detached Compose service with persistent Docker-managed storage.

Check it with:

```bash
docker compose ps
docker compose logs -f server
```

### Docker client

The client is launched as a one-shot container. The input file is mounted read-only into the client container:

```bash
./run.sh docker-client ./test.bin 127.0.0.1:9000
```

When the server is running on the Docker host and the client is also containerized, use a host-reachable endpoint such as `host.docker.internal:9000` where supported:

```bash
./run.sh docker-client ./test.bin host.docker.internal:9000
```

When both client and server are containers, they should eventually be placed on the same Docker network and addressed by the server service name, for example `server:9000`. This keeps service discovery independent of host IP addresses.

### Compose lifecycle

Production server deployment:

```bash
./run.sh compose-up
```

Stop the server:

```bash
./run.sh compose-down
```

Follow logs:

```bash
./run.sh compose-logs
```

## Docker

Build the image directly:

```bash
docker build -t tcp-file-transfer:latest .
```

The image uses a multi-stage build. The final runtime image contains only the transfer binaries and runtime dependencies and runs as the non-root `tcpft` user.

### Docker Compose storage models

The production `docker-compose.yml` uses a **managed Docker volume**:

```text
Docker host
    |
    v
+---------------------------+
| ft-server container       |
|                           |
| /data                     |
+-------------+-------------+
              |
              v
        tcpft-data volume
```

This avoids host bind-mount UID/GID problems and keeps received data persistent across container recreation.

The development `compose.dev.yml` uses:

```text
./received:/data
```

and maps the current host UID/GID into the container, so received files are directly visible in the repository without requiring `chmod 777`.

## Server/client deployment model

The application has two different runtime roles:

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

The server should be deployed as a persistent process/container because it listens for incoming transfers. The client should normally be started per transfer because it has no reason to remain alive after completing a file operation.

This separation also makes the utility suitable for both deployment models:

- **Server:** Docker Compose/service deployment.
- **Client:** CLI invocation or short-lived container.

For a future multi-container deployment, both roles can share a dedicated Docker network and the client can connect to the Compose service name instead of a hard-coded IP address.

## Development Docker deployment

```bash
mkdir -p received
export UID=$(id -u)
export GID=$(id -g)
docker compose -f compose.dev.yml up --build
```

Then from another terminal:

```bash
./run.sh client ./test.bin 127.0.0.1:9000
```

Received files appear under:

```text
./received/
```

Stop it with:

```bash
docker compose -f compose.dev.yml down
```

> Do not use `chmod 777` as the normal deployment solution. The development Compose configuration maps the host UID/GID, while the production configuration uses a Docker-managed volume owned by the container runtime user.

## Run natively

Start the receiver:

```bash
./build/ft-server 9000 ./received
```

On Windows with a multi-config generator:

```powershell
.\build\Release\ft-server.exe 9000 .\received
```

Send a file from another terminal/machine:

```bash
./build/ft-client send ./test.bin 127.0.0.1:9000
```

The receiver stores the file below the configured output directory.

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
7. Keep the container runtime minimal and run the service as a non-root user.
8. Use host UID/GID mapping for development bind mounts and Docker-managed volumes for production persistence.
9. Treat the server as a persistent service and the client as a one-shot transfer operation.

## Security roadmap

The current foundation validates protocol boundaries and destination filenames but does not yet provide confidentiality or peer authentication. TLS 1.3 and certificate validation are planned for the secure-transfer milestone.

## Assignment mapping

The ATI assignment asks for TCP transfer, files up to 16 GB, integrity checks, error handling, cross-platform compatibility, and optionally improved bandwidth utilization. It also requests source code, build/run documentation, unit tests, and a project-description document with architecture, design considerations, C4 diagrams, and performance metrics.
