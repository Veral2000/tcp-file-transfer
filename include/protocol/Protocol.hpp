#pragma once

#include "common/Types.hpp"
#include "crypto/Sha256.hpp"
#include "network/TcpSocket.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tcpft::protocol {

constexpr std::uint32_t kMagic = 0x54435046U; // "TCPF"
constexpr std::uint16_t kVersion = 1U;
constexpr std::size_t kHeaderSize = 16U;
constexpr std::size_t kMaxFilenameLength = 4096U;

enum class MessageType : std::uint8_t {
    Hello = 1,
    FileInfo = 2,
    Chunk = 3,
    TransferComplete = 4,
    Error = 5,
    FileHash = 6,
};

struct MessageHeader {
    MessageType type{};
    std::uint64_t payload_size{0};
};

struct Chunk {
    ChunkIndex index{0};
    FileOffset offset{0};
    std::vector<std::uint8_t> data;
};

void send_message(network::TcpSocket& socket, MessageType type,
                  const std::vector<std::uint8_t>& payload);

MessageHeader receive_header(network::TcpSocket& socket);
std::vector<std::uint8_t> receive_payload(network::TcpSocket& socket,
                                           const MessageHeader& header);

void send_hello(network::TcpSocket& socket);
void send_file_info(network::TcpSocket& socket, const FileInfo& info);
void send_file_hash(network::TcpSocket& socket, const crypto::Sha256Digest& digest);
void send_chunk(network::TcpSocket& socket, const Chunk& chunk);
void send_transfer_complete(network::TcpSocket& socket);
void send_error(network::TcpSocket& socket, const std::string& message);

FileInfo parse_file_info(const std::vector<std::uint8_t>& payload);
crypto::Sha256Digest parse_file_hash(const std::vector<std::uint8_t>& payload);
Chunk parse_chunk(const std::vector<std::uint8_t>& payload);
std::string parse_error(const std::vector<std::uint8_t>& payload);

} // namespace tcpft::protocol
