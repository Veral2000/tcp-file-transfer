# Docker Deployment

Docker support is maintained on the `docker` branch so the `main` branch remains focused on the native C++ application.

## Development deployment

From the repository root:

```bash
mkdir -p received
./deploy/docker/run.sh docker-server
```

The development server bind-mounts `./received` into `/data` and maps the current Linux UID/GID to avoid host permission problems.

Send a file with the native client:

```bash
./build/ft-client send ./test.bin 127.0.0.1:9000
```

Or use the containerized client:

```bash
./deploy/docker/run.sh docker-client ./test.bin 127.0.0.1:9000
```

## Production deployment

```bash
./deploy/docker/run.sh compose-up
```

The production Compose deployment uses a Docker-managed `tcpft-data` volume mounted at `/data` and restarts the server unless explicitly stopped.

Check status:

```bash
docker compose -f deploy/docker/docker-compose.yml ps
```

Follow logs:

```bash
./deploy/docker/run.sh compose-logs
```

Stop:

```bash
./deploy/docker/run.sh compose-down
```

Remove stored data as well:

```bash
docker compose -f deploy/docker/docker-compose.yml down -v
```

## Build image

```bash
./deploy/docker/run.sh docker-build
```

The Dockerfile uses a multi-stage build and runs the runtime process as the non-root `tcpft` user.
