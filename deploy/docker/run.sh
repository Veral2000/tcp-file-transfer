#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
IMAGE="${TCPFT_IMAGE:-tcp-file-transfer:latest}"
PORT="${TCPFT_PORT:-9000}"
DATA_DIR="${TCPFT_DATA_DIR:-${ROOT_DIR}/received}"

usage() {
    cat <<'EOF'
TCP File Transfer Docker deployment helper

Usage:
  ./deploy/docker/run.sh build
  ./deploy/docker/run.sh server
  ./deploy/docker/run.sh client <file> <host:port>
  ./deploy/docker/run.sh docker-build
  ./deploy/docker/run.sh docker-server
  ./deploy/docker/run.sh docker-client <file> <host:port>
  ./deploy/docker/run.sh compose-up
  ./deploy/docker/run.sh compose-down
  ./deploy/docker/run.sh compose-logs

Modes:
  build          Build the Docker image.
  server         Start the development Docker server with ./received bind-mounted.
  client         Run the native client against a server.
  docker-build   Build the production Docker image.
  docker-server  Start the development Docker server.
  docker-client  Run the client as a one-shot Docker container.
  compose-up     Start the production server with a managed Docker volume.
  compose-down   Stop the production server.
  compose-logs   Follow production server logs.

Environment:
  TCPFT_PORT       Server port (default: 9000)
  TCPFT_DATA_DIR   Development output directory
  TCPFT_IMAGE      Docker image name (default: tcp-file-transfer:latest)
EOF
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Error: required command '$1' is not installed." >&2
        exit 1
    }
}

docker_build() {
    require_command docker
    docker build -f "${ROOT_DIR}/deploy/docker/Dockerfile" -t "${IMAGE}" "${ROOT_DIR}"
}

docker_server() {
    require_command docker
    mkdir -p "${DATA_DIR}"
    export UID="$(id -u)"
    export GID="$(id -g)"
    TCPFT_PORT="${PORT}" docker compose -f "${SCRIPT_DIR}/compose.dev.yml" up --build
}

docker_client() {
    require_command docker
    [[ $# -eq 2 ]] || { echo "Usage: ./deploy/docker/run.sh docker-client <file> <host:port>" >&2; exit 2; }
    local file="$1"
    local endpoint="$2"
    [[ -f "${file}" ]] || { echo "Error: file not found: ${file}" >&2; exit 1; }
    local abs_file
    abs_file="$(cd "$(dirname "${file}")" && pwd)/$(basename "${file}")"
    docker run --rm \
        --entrypoint /usr/local/bin/ft-client \
        -v "${abs_file}:/transfer/$(basename "${file}"):ro" \
        "${IMAGE}" \
        send "/transfer/$(basename "${file}")" "${endpoint}"
}

compose_up() {
    require_command docker
    TCPFT_PORT="${PORT}" docker compose -f "${SCRIPT_DIR}/docker-compose.yml" up --build -d
    docker compose -f "${SCRIPT_DIR}/docker-compose.yml" ps
}

compose_down() {
    require_command docker
    docker compose -f "${SCRIPT_DIR}/docker-compose.yml" down
}

compose_logs() {
    require_command docker
    docker compose -f "${SCRIPT_DIR}/docker-compose.yml" logs -f server
}

main() {
    local command="${1:-help}"
    shift || true
    case "${command}" in
        build|docker-build) docker_build "$@" ;;
        server|docker-server) docker_server "$@" ;;
        client|docker-client) docker_client "$@" ;;
        compose-up) compose_up "$@" ;;
        compose-down) compose_down "$@" ;;
        compose-logs) compose_logs "$@" ;;
        help|-h|--help) usage ;;
        *) echo "Unknown command: ${command}" >&2; usage; exit 2 ;;
    esac
}

main "$@"
