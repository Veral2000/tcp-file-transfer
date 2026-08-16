#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CLIENT_BIN="${BUILD_DIR}/ft-client"
SERVER_BIN="${BUILD_DIR}/ft-server"
IMAGE="${TCPFT_IMAGE:-tcp-file-transfer:latest}"
PORT="${TCPFT_PORT:-9000}"
DATA_DIR="${TCPFT_DATA_DIR:-${ROOT_DIR}/received}"

usage() {
    cat <<'EOF'
TCP File Transfer deployment helper

Usage:
  ./run.sh build
  ./run.sh test
  ./run.sh server [port] [output-directory]
  ./run.sh client <file> <host:port>
  ./run.sh docker-build
  ./run.sh docker-server
  ./run.sh docker-client <file> <host:port>
  ./run.sh compose-up
  ./run.sh compose-down
  ./run.sh compose-logs

Modes:
  build          Build native C++ binaries with CMake.
  test           Build and run CTest.
  server         Run the native TCP server. Long-running service.
  client         Run the native client. One-shot transfer operation.
  docker-build   Build the production Docker image.
  docker-server  Deploy the server as a detached Docker Compose service.
  docker-client  Run the client as a one-shot Docker container.
  compose-up     Start the production server deployment.
  compose-down   Stop the production server deployment.
  compose-logs   Follow production server logs.

Environment:
  TCPFT_PORT       Server port (default: 9000)
  TCPFT_DATA_DIR   Native/development output directory
  TCPFT_IMAGE      Docker image name (default: tcp-file-transfer:latest)

Examples:
  ./run.sh server 9000 ./received
  ./run.sh client ./test.bin 127.0.0.1:9000
  ./run.sh docker-build
  ./run.sh docker-server
  ./run.sh docker-client ./test.bin 127.0.0.1:9000
EOF
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Error: required command '$1' is not installed." >&2
        exit 1
    }
}

ensure_native_build() {
    require_command cmake
    if [[ ! -x "${CLIENT_BIN}" || ! -x "${SERVER_BIN}" ]]; then
        echo "Native binaries not found. Building..."
        cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
        cmake --build "${BUILD_DIR}" --parallel
    fi
}

build() {
    require_command cmake
    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" --parallel
}

test() {
    build
    require_command ctest
    ctest --test-dir "${BUILD_DIR}" --output-on-failure
}

native_server() {
    ensure_native_build
    local port="${1:-${PORT}}"
    local output_dir="${2:-${DATA_DIR}}"
    mkdir -p "${output_dir}"
    echo "Starting native TCP server on port ${port}"
    echo "Output directory: ${output_dir}"
    exec "${SERVER_BIN}" "${port}" "${output_dir}"
}

native_client() {
    ensure_native_build
    [[ $# -eq 2 ]] || { echo "Usage: ./run.sh client <file> <host:port>" >&2; exit 2; }
    exec "${CLIENT_BIN}" send "$1" "$2"
}

docker_build() {
    require_command docker
    docker build -t "${IMAGE}" "${ROOT_DIR}"
}

docker_server() {
    require_command docker
    echo "Starting Dockerized TCP server on port ${PORT}"
    TCPFT_PORT="${PORT}" docker compose -f "${ROOT_DIR}/docker-compose.yml" up --build -d
    docker compose -f "${ROOT_DIR}/docker-compose.yml" ps
}

docker_client() {
    require_command docker
    [[ $# -eq 2 ]] || { echo "Usage: ./run.sh docker-client <file> <host:port>" >&2; exit 2; }
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
    TCPFT_PORT="${PORT}" docker compose -f "${ROOT_DIR}/docker-compose.yml" up --build -d
    docker compose -f "${ROOT_DIR}/docker-compose.yml" ps
}

compose_down() {
    require_command docker
    docker compose -f "${ROOT_DIR}/docker-compose.yml" down
}

compose_logs() {
    require_command docker
    docker compose -f "${ROOT_DIR}/docker-compose.yml" logs -f server
}

main() {
    local command="${1:-help}"
    shift || true

    case "${command}" in
        build) build "$@" ;;
        test) test "$@" ;;
        server) native_server "$@" ;;
        client) native_client "$@" ;;
        docker-build) docker_build "$@" ;;
        docker-server) docker_server "$@" ;;
        docker-client) docker_client "$@" ;;
        compose-up) compose_up "$@" ;;
        compose-down) compose_down "$@" ;;
        compose-logs) compose_logs "$@" ;;
        help|-h|--help) usage ;;
        *) echo "Unknown command: ${command}" >&2; usage; exit 2 ;;
    esac
}

main "$@"
