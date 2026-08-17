#include "crypto/Sha256.hpp"
#include "network/TcpSocket.hpp"
#include "protocol/Protocol.hpp"
#include "transfer/FileWriter.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

std::uint16_t parse_port(const char* value) {
    const auto parsed = std::stoul(value);
    if (parsed == 0U || parsed > 65535U) throw std::invalid_argument("invalid port");
    return static_cast<std::uint16_t>(parsed);
}

void handle_client(tcpft::network::TcpSocket& client,
                   const std::filesystem::path& output_dir) {
    const auto hello = tcpft::protocol::receive_header(client);
    if (hello.type != tcpft::protocol::MessageType::Hello || hello.payload_size != 0U) {
        throw std::runtime_error("expected HELLO message");
    }

    const auto info_header = tcpft::protocol::receive_header(client);
    if (info_header.type != tcpft::protocol::MessageType::FileInfo) {
        throw std::runtime_error("expected FILE_INFO message");
    }
    const auto info = tcpft::protocol::parse_file_info(
        tcpft::protocol::receive_payload(client, info_header));

    const auto hash_header = tcpft::protocol::receive_header(client);
    if (hash_header.type != tcpft::protocol::MessageType::FileHash) {
        throw std::runtime_error("expected FILE_HASH message");
    }
    const auto expected_hash = tcpft::protocol::parse_file_hash(
        tcpft::protocol::receive_payload(client, hash_header));

    std::cout << "Receiving '" << info.filename << "' (" << info.size << " bytes)\n";
    std::cout << "Expected SHA-256: " << tcpft::crypto::to_hex(expected_hash) << '\n';

    tcpft::transfer::FileWriter writer(output_dir, info);
    tcpft::ChunkIndex expected_chunk = 0;

    while (true) {
        const auto header = tcpft::protocol::receive_header(client);
        const auto payload = tcpft::protocol::receive_payload(client, header);

        if (header.type == tcpft::protocol::MessageType::Chunk) {
            const auto chunk = tcpft::protocol::parse_chunk(payload);
            if (chunk.index != expected_chunk) throw std::runtime_error("unexpected chunk index");
            writer.write_chunk(chunk.offset, chunk.data);
            ++expected_chunk;
            std::cout << "\rReceived " << writer.path().filename().string()
                      << ": " << writer.path().string() << std::flush;
        } else if (header.type == tcpft::protocol::MessageType::TransferComplete) {
            if (!payload.empty()) throw std::runtime_error("invalid completion payload");
            writer.finalize();

            const auto actual_hash = tcpft::crypto::hash_file(writer.path());
            std::cout << "\nActual SHA-256:   " << tcpft::crypto::to_hex(actual_hash) << '\n';
            if (actual_hash != expected_hash) {
                throw std::runtime_error("SHA-256 integrity check failed");
            }

            std::cout << "Integrity check: PASS\n";
            std::cout << "Transfer complete: " << writer.path() << "\n";
            return;
        } else if (header.type == tcpft::protocol::MessageType::Error) {
            throw std::runtime_error("client error: " + tcpft::protocol::parse_error(payload));
        } else {
            throw std::runtime_error("unexpected message during transfer");
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ft-server <port> <output-directory>\n";
        return 2;
    }

    try {
        tcpft::network::NetworkRuntime runtime;
        const auto port = parse_port(argv[1]);
        const std::filesystem::path output_dir(argv[2]);

        auto listener = tcpft::network::TcpSocket::create();
        listener.bind_and_listen(port);
        std::cout << "Listening on TCP port " << port << "...\n";

        while (true) {
            try {
                auto client = listener.accept();
                std::cout << "Client connected.\n";
                handle_client(client, output_dir);
            } catch (const std::exception& ex) {
                std::cerr << "Client session error: " << ex.what() << '\n';
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << '\n';
        return 1;
    }
}
