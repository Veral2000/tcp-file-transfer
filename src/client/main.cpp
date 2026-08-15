#include "common/Types.hpp"
#include "network/TcpSocket.hpp"
#include "protocol/Protocol.hpp"
#include "transfer/FileReader.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace {

std::uint16_t parse_port(const std::string& value) {
    const auto parsed = std::stoul(value);
    if (parsed == 0U || parsed > 65535U) throw std::invalid_argument("invalid port");
    return static_cast<std::uint16_t>(parsed);
}

std::pair<std::string, std::uint16_t> parse_endpoint(const std::string& endpoint) {
    const auto separator = endpoint.rfind(':');
    if (separator == std::string::npos || separator == 0U || separator + 1U >= endpoint.size()) {
        throw std::invalid_argument("endpoint must be host:port");
    }
    return {endpoint.substr(0, separator), parse_port(endpoint.substr(separator + 1U))};
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ft-client send <file> <host:port>\n";
        return 2;
    }
    if (std::string(argv[1]) != "send") {
        std::cerr << "Only the 'send' command is currently supported.\n";
        return 2;
    }

    try {
        tcpft::network::NetworkRuntime runtime;
        tcpft::transfer::FileReader reader(argv[2]);
        const auto [host, port] = parse_endpoint(argv[3]);

        auto socket = tcpft::network::TcpSocket::create();
        socket.connect(host.c_str(), port);

        tcpft::protocol::send_hello(socket);
        tcpft::protocol::send_file_info(socket, reader.info());

        std::cout << "Sending '" << reader.info().filename << "' ("
                  << reader.info().size << " bytes)\n";

        while (reader.has_more()) {
            auto data = reader.read_chunk(tcpft::kDefaultChunkSize);
            tcpft::protocol::Chunk chunk{
                reader.current_chunk(),
                reader.current_offset() - static_cast<tcpft::FileOffset>(data.size()),
                std::move(data)
            };
            tcpft::protocol::send_chunk(socket, chunk);

            const auto percent = reader.info().size == 0U
                ? 100U
                : static_cast<unsigned int>((reader.current_offset() * 100U) / reader.info().size);
            std::cout << "\rProgress: " << percent << "% ("
                      << reader.current_offset() << "/" << reader.info().size << " bytes)"
                      << std::flush;
        }

        tcpft::protocol::send_transfer_complete(socket);
        std::cout << "\nTransfer complete.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Client error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
