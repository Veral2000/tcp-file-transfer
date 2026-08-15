#include "network/TcpSocket.hpp"

#include <cerrno>
#include <cstring>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace tcpft::network {
namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;
constexpr NativeSocket kInvalidSocket = INVALID_SOCKET;
#else
using NativeSocket = int;
constexpr NativeSocket kInvalidSocket = -1;
#endif

NativeSocket to_native(std::intptr_t handle) noexcept {
#ifdef _WIN32
    return static_cast<SOCKET>(handle);
#else
    return static_cast<int>(handle);
#endif
}

std::intptr_t from_native(NativeSocket handle) noexcept {
    return static_cast<std::intptr_t>(handle);
}

void close_native(NativeSocket socket) noexcept {
#ifdef _WIN32
    if (socket != kInvalidSocket) {
        ::closesocket(socket);
    }
#else
    if (socket != kInvalidSocket) {
        ::close(socket);
    }
#endif
}

std::string last_error_message(const char* operation) {
#ifdef _WIN32
    const int error = ::WSAGetLastError();
    return std::string(operation) + " failed with Winsock error " + std::to_string(error);
#else
    return std::string(operation) + " failed: " + std::strerror(errno);
#endif
}

} // namespace

NetworkRuntime::NetworkRuntime() {
#ifdef _WIN32
    WSADATA data{};
    if (::WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        throw SocketError("WSAStartup failed");
    }
#endif
}

NetworkRuntime::~NetworkRuntime() {
#ifdef _WIN32
    ::WSACleanup();
#endif
}

TcpSocket::TcpSocket(std::intptr_t native_handle) noexcept
    : handle_(native_handle), owned_(true) {}

TcpSocket::TcpSocket(std::intptr_t native_handle, bool owned) noexcept
    : handle_(native_handle), owned_(owned) {}

TcpSocket::~TcpSocket() {
    close();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : handle_(other.handle_), owned_(other.owned_) {
    other.handle_ = -1;
    other.owned_ = false;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = other.handle_;
        owned_ = other.owned_;
        other.handle_ = -1;
        other.owned_ = false;
    }
    return *this;
}

TcpSocket TcpSocket::create() {
    const NativeSocket socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == kInvalidSocket) {
        throw SocketError(last_error_message("socket"));
    }
    return TcpSocket(from_native(socket));
}

void TcpSocket::connect(const char* host, std::uint16_t port) {
    if (!valid()) {
        throw SocketError("connect called on invalid socket");
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    const std::string port_string = std::to_string(port);
    const int status = ::getaddrinfo(host, port_string.c_str(), &hints, &result);
    if (status != 0) {
#ifdef _WIN32
        throw SocketError("getaddrinfo failed: " + std::to_string(status));
#else
        throw SocketError(std::string("getaddrinfo failed: ") + gai_strerror(status));
#endif
    }

    bool connected = false;
    for (addrinfo* current = result; current != nullptr; current = current->ai_next) {
        if (::connect(to_native(handle_), current->ai_addr,
                      static_cast<int>(current->ai_addrlen)) == 0) {
            connected = true;
            break;
        }
    }
    ::freeaddrinfo(result);

    if (!connected) {
        throw SocketError(last_error_message("connect"));
    }
}

void TcpSocket::bind_and_listen(std::uint16_t port, int backlog) {
    if (!valid()) {
        throw SocketError("bind_and_listen called on invalid socket");
    }

    int reuse = 1;
    if (::setsockopt(to_native(handle_), SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const char*>(&reuse), sizeof(reuse)) != 0) {
        throw SocketError(last_error_message("setsockopt"));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (::bind(to_native(handle_), reinterpret_cast<const sockaddr*>(&address),
               sizeof(address)) != 0) {
        throw SocketError(last_error_message("bind"));
    }

    if (::listen(to_native(handle_), backlog) != 0) {
        throw SocketError(last_error_message("listen"));
    }
}

TcpSocket TcpSocket::accept() {
    sockaddr_storage address{};
#ifdef _WIN32
    int address_length = static_cast<int>(sizeof(address));
#else
    socklen_t address_length = sizeof(address);
#endif

    const NativeSocket client = ::accept(to_native(handle_),
                                         reinterpret_cast<sockaddr*>(&address),
                                         &address_length);
    if (client == kInvalidSocket) {
        throw SocketError(last_error_message("accept"));
    }
    return TcpSocket(from_native(client));
}

std::size_t TcpSocket::send_all(const void* data, std::size_t size) {
    const auto* bytes = static_cast<const char*>(data);
    std::size_t sent = 0;

    while (sent < size) {
        const auto remaining = size - sent;
#ifdef _WIN32
        const int request = static_cast<int>(remaining > static_cast<std::size_t>(INT_MAX)
                                                 ? INT_MAX : remaining);
        const int result = ::send(to_native(handle_), bytes + sent, request, 0);
#else
        const std::size_t request = remaining;
        const ssize_t result = ::send(to_native(handle_), bytes + sent, request, MSG_NOSIGNAL);
#endif
        if (result <= 0) {
            throw SocketError(last_error_message("send"));
        }
        sent += static_cast<std::size_t>(result);
    }
    return sent;
}

std::size_t TcpSocket::receive_some(void* data, std::size_t size) {
    if (size == 0U) {
        return 0U;
    }
#ifdef _WIN32
    const int request = static_cast<int>(size > static_cast<std::size_t>(INT_MAX)
                                             ? INT_MAX : size);
    const int result = ::recv(to_native(handle_), static_cast<char*>(data), request, 0);
#else
    const ssize_t result = ::recv(to_native(handle_), data, size, 0);
#endif
    if (result < 0) {
        throw SocketError(last_error_message("recv"));
    }
    return static_cast<std::size_t>(result);
}

void TcpSocket::receive_all(void* data, std::size_t size) {
    auto* bytes = static_cast<char*>(data);
    std::size_t received = 0;
    while (received < size) {
        const std::size_t count = receive_some(bytes + received, size - received);
        if (count == 0U) {
            throw SocketError("connection closed before receiving expected data");
        }
        received += count;
    }
}

bool TcpSocket::valid() const noexcept {
    return handle_ != -1;
}

void TcpSocket::close() noexcept {
    if (valid() && owned_) {
        close_native(to_native(handle_));
    }
    handle_ = -1;
    owned_ = false;
}

std::intptr_t TcpSocket::native_handle() const noexcept {
    return handle_;
}

} // namespace tcpft::network
