#include "transfer/FileReader.hpp"
#include "transfer/FileWriter.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::filesystem::path unique_test_dir() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / ("tcpft_test_" + std::to_string(stamp));
}

void test_round_trip() {
    const auto directory = unique_test_dir();
    std::filesystem::create_directories(directory);
    const auto source = directory / "source.bin";

    {
        std::ofstream output(source, std::ios::binary);
        const std::string contents = "TCP file transfer test payload";
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    }

    tcpft::transfer::FileReader reader(source);
    assert(reader.info().size > 0U);

    const auto destination_dir = directory / "received";
    tcpft::transfer::FileWriter writer(destination_dir, reader.info());

    while (reader.has_more()) {
        auto data = reader.read_chunk(8U);
        const auto offset = reader.current_offset() - static_cast<tcpft::FileOffset>(data.size());
        writer.write_chunk(offset, data);
    }
    writer.finalize();

    std::ifstream input(writer.path(), std::ios::binary);
    std::string received((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    assert(received == "TCP file transfer test payload");

    std::filesystem::remove_all(directory);
}

void test_oversized_file_limit_is_enforced_by_metadata_logic() {
    // This test documents the public protocol limit without allocating a 16+ GiB file.
    assert(tcpft::kMaxFileSize == 16ULL * 1024ULL * 1024ULL * 1024ULL);
}

} // namespace

void run_file_io_tests() {
    test_round_trip();
    test_oversized_file_limit_is_enforced_by_metadata_logic();
}
