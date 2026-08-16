#include "protocol/Protocol.hpp"

#include <cassert>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

void run_file_io_tests();

namespace {

void test_message_type_values_are_stable() {
    assert(static_cast<std::uint8_t>(tcpft::protocol::MessageType::Hello) == 1U);
    assert(static_cast<std::uint8_t>(tcpft::protocol::MessageType::FileInfo) == 2U);
    assert(static_cast<std::uint8_t>(tcpft::protocol::MessageType::Chunk) == 3U);
    assert(static_cast<std::uint8_t>(tcpft::protocol::MessageType::TransferComplete) == 4U);
    assert(static_cast<std::uint8_t>(tcpft::protocol::MessageType::Error) == 5U);
}

void test_error_payload_round_trip() {
    const std::string message = "transfer failed";
    const std::vector<std::uint8_t> payload(message.begin(), message.end());
    assert(tcpft::protocol::parse_error(payload) == message);
}

void test_file_info_parser_rejects_invalid_payload() {
    bool rejected = false;
    try {
        tcpft::protocol::parse_file_info({0, 0, 0});
    } catch (const std::exception&) {
        rejected = true;
    }

    if (!rejected) {
        throw std::runtime_error("parse_file_info accepted an invalid payload");
    }
}

} // namespace

void run_protocol_tests() {
    test_message_type_values_are_stable();
    test_error_payload_round_trip();
    test_file_info_parser_rejects_invalid_payload();
}

int main() {
    run_file_io_tests();
    run_protocol_tests();
    return 0;
}
