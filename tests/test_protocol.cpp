#include "protocol/Protocol.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void run_file_io_tests();

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_message_type_values_are_stable() {
    require(static_cast<std::uint8_t>(tcpft::protocol::MessageType::Hello) == 1U,
            "Hello message type changed");
    require(static_cast<std::uint8_t>(tcpft::protocol::MessageType::FileInfo) == 2U,
            "FileInfo message type changed");
    require(static_cast<std::uint8_t>(tcpft::protocol::MessageType::Chunk) == 3U,
            "Chunk message type changed");
    require(static_cast<std::uint8_t>(tcpft::protocol::MessageType::TransferComplete) == 4U,
            "TransferComplete message type changed");
    require(static_cast<std::uint8_t>(tcpft::protocol::MessageType::Error) == 5U,
            "Error message type changed");
}

void test_error_payload_round_trip() {
    const std::string message = "transfer failed";
    const std::vector<std::uint8_t> payload(message.begin(), message.end());
    require(tcpft::protocol::parse_error(payload) == message,
            "error payload round-trip failed");
}

void test_file_info_parser_rejects_invalid_payload() {
    bool rejected = false;
    try {
        static_cast<void>(tcpft::protocol::parse_file_info({0, 0, 0}));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "invalid file-info payload was accepted");
}

} // namespace

void run_protocol_tests() {
    test_message_type_values_are_stable();
    test_error_payload_round_trip();
    test_file_info_parser_rejects_invalid_payload();
}

int main() {
    try {
        std::cout << "Running file I/O tests..." << std::endl;
        run_file_io_tests();
        std::cout << "File I/O tests passed." << std::endl;

        std::cout << "Running protocol tests..." << std::endl;
        run_protocol_tests();
        std::cout << "Protocol tests passed." << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "TEST FAILURE: " << error.what() << std::endl;
        return 1;
    }
}
