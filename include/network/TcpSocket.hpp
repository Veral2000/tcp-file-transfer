#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#endif

namespace tcpft::network {

class SocketError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class TcpSocket {
public:
    TcpSocket() = default;
    explicit TcpSocket(std::intptr_t native_handle) noexcept;
    ~TcpSocket();

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    static TcpSocket create();
    void connect(const char* host, std::uint16_t port);
    void bind_and_listen(std::uint16_t port, int backlog = 8);
    TcpSocket accept();

    std::size_t send_all(const void* data, std::size_t size);
    std::size_t receive_some(void* data, std::size_t size);
    void receive_all(void* data, std::size_t size);

    bool valid() const noexcept;
    void close() noexcept;
    std::intptr_t native_handle() const noexcept;

private:
    explicit TcpSocket(std::intptr_t native_handle, bool owned) noexcept;
    std::intptr_t handle_{-1};
    bool owned_{true};
};

class NetworkRuntime {
public:
    NetworkRuntime();
    ~NetworkRuntime();
    NetworkRuntime(const NetworkRuntime&) = delete;
    NetworkRuntime& operator=(const NetworkRuntime&) = delete;
};

} // namespace tcpft::network
