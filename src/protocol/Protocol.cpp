#include "protocol/Protocol.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace tcpft::protocol {
namespace {

constexpr std::uint64_t kMaxPayloadSize = 32ULL * 1024ULL * 1024ULL;

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
}

std::uint16_t read_u16(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    if (data.size() - pos < 2U) throw std::runtime_error("truncated uint16");
    const std::uint16_t value = static_cast<std::uint16_t>(data[pos]) << 8U |
                                static_cast<std::uint16_t>(data[pos + 1U]);
    pos += 2U;
    return value;
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    if (data.size() - pos < 4U) throw std::runtime_error("truncated uint32");
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        value = (value << 8U) | data[pos + static_cast<std::size_t>(i)];
    }
    pos += 4U;
    return value;
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    if (data.size() - pos < 8U) throw std::runtime_error("truncated uint64");
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8U) | data[pos + static_cast<std::size_t>(i)];
    }
    pos += 8U;
    return value;
}

std::vector<std::uint8_t> encode_header(MessageType type, std::uint64_t payload_size) {
    std::vector<std::uint8_t> header;
    header.reserve(kHeaderSize);
    append_u32(header, kMagic);
    append_u16(header, kVersion);
    header.push_back(static_cast<std::uint8_t>(type));
    header.push_back(0U);
    append_u64(header, payload_size);
    return header;
}

} // namespace

void send_message(network::TcpSocket& socket, MessageType type,
                  const std::vector<std::uint8_t>& payload) {
    if (payload.size() > kMaxPayloadSize) {
        throw std::runtime_error("payload exceeds protocol maximum");
    }
    const auto header = encode_header(type, static_cast<std::uint64_t>(payload.size()));
    socket.send_all(header.data(), header.size());
    if (!payload.empty()) socket.send_all(payload.data(), payload.size());
}

MessageHeader receive_header(network::TcpSocket& socket) {
    std::vector<std::uint8_t> header(kHeaderSize);
    socket.receive_all(header.data(), header.size());

    std::size_t pos = 0;
    const std::uint32_t magic = read_u32(header, pos);
    const std::uint16_t version = read_u16(header, pos);
    const auto type = static_cast<MessageType>(header[pos++]);
    ++pos;
    const std::uint64_t payload_size = read_u64(header, pos);

    if (magic != kMagic) throw std::runtime_error("invalid protocol magic");
    if (version != kVersion) throw std::runtime_error("unsupported protocol version");
    if (payload_size > kMaxPayloadSize) throw std::runtime_error("payload exceeds protocol maximum");

    switch (type) {
        case MessageType::Hello:
        case MessageType::FileInfo:
        case MessageType::Chunk:
        case MessageType::TransferComplete:
        case MessageType::Error:
            break;
        default:
            throw std::runtime_error("unknown message type");
    }

    return MessageHeader{type, payload_size};
}

std::vector<std::uint8_t> receive_payload(network::TcpSocket& socket,
                                           const MessageHeader& header) {
    if (header.payload_size > kMaxPayloadSize ||
        header.payload_size > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
        throw std::runtime_error("invalid payload size");
    }
    std::vector<std::uint8_t> payload(static_cast<std::size_t>(header.payload_size));
    if (!payload.empty()) socket.receive_all(payload.data(), payload.size());
    return payload;
}

void send_hello(network::TcpSocket& socket) {
    send_message(socket, MessageType::Hello, {});
}

void send_file_info(network::TcpSocket& socket, const FileInfo& info) {
    if (info.filename.empty() || info.filename.size() > kMaxFilenameLength) {
        throw std::runtime_error("invalid filename length");
    }
    if (info.size > kMaxFileSize) {
        throw std::runtime_error("file exceeds 16 GiB limit");
    }

    std::vector<std::uint8_t> payload;
    append_u64(payload, info.size);
    append_u16(payload, static_cast<std::uint16_t>(info.filename.size()));
    payload.insert(payload.end(), info.filename.begin(), info.filename.end());
    send_message(socket, MessageType::FileInfo, payload);
}

void send_chunk(network::TcpSocket& socket, const Chunk& chunk) {
    if (chunk.data.empty() || chunk.data.size() > kMaxChunkSize) {
        throw std::runtime_error("invalid chunk size");
    }
    std::vector<std::uint8_t> payload;
    payload.reserve(20U + chunk.data.size());
    append_u64(payload, chunk.index);
    append_u64(payload, chunk.offset);
    append_u32(payload, static_cast<std::uint32_t>(chunk.data.size()));
    payload.insert(payload.end(), chunk.data.begin(), chunk.data.end());
    send_message(socket, MessageType::Chunk, payload);
}

void send_transfer_complete(network::TcpSocket& socket) {
    send_message(socket, MessageType::TransferComplete, {});
}

void send_error(network::TcpSocket& socket, const std::string& message) {
    if (message.size() > 1024U * 1024U) throw std::runtime_error("error message too large");
    send_message(socket, MessageType::Error,
                 std::vector<std::uint8_t>(message.begin(), message.end()));
}

FileInfo parse_file_info(const std::vector<std::uint8_t>& payload) {
    std::size_t pos = 0;
    const FileSize size = read_u64(payload, pos);
    const std::uint16_t name_length = read_u16(payload, pos);
    if (name_length == 0U || name_length > kMaxFilenameLength || payload.size() - pos != name_length) {
        throw std::runtime_error("invalid file metadata");
    }
    std::string filename(payload.begin() + static_cast<std::ptrdiff_t>(pos), payload.end());
    if (size > kMaxFileSize) throw std::runtime_error("file exceeds 16 GiB limit");
    return FileInfo{std::move(filename), size};
}

Chunk parse_chunk(const std::vector<std::uint8_t>& payload) {
    std::size_t pos = 0;
    const ChunkIndex index = read_u64(payload, pos);
    const FileOffset offset = read_u64(payload, pos);
    const std::uint32_t data_size = read_u32(payload, pos);
    if (data_size == 0U || data_size > kMaxChunkSize || payload.size() - pos != data_size) {
        throw std::runtime_error("invalid chunk payload");
    }
    std::vector<std::uint8_t> data(payload.begin() + static_cast<std::ptrdiff_t>(pos), payload.end());
    return Chunk{index, offset, std::move(data)};
}

std::string parse_error(const std::vector<std::uint8_t>& payload) {
    return std::string(payload.begin(), payload.end());
}

} // namespace tcpft::protocol
