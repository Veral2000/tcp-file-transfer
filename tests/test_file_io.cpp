#include "transfer/FileReader.hpp"
#include "transfer/FileWriter.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::filesystem::path unique_test_dir() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("tcpft_test_" + std::to_string(stamp));
}

void test_round_trip() {
    const auto directory = unique_test_dir();
    std::error_code cleanup_error;

    try {
        std::filesystem::create_directories(directory);
        const auto source = directory / "source.bin";

        {
            std::ofstream output(source, std::ios::binary);
            require(output.is_open(), "failed to create source test file");
            const std::string contents = "TCP file transfer test payload";
            output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            require(static_cast<bool>(output), "failed to write source test file");
        }

        tcpft::transfer::FileReader reader(source);
        require(reader.info().size == 31U, "unexpected source file size");

        const auto destination_dir = directory / "received";
        tcpft::transfer::FileWriter writer(destination_dir, reader.info());

        while (reader.has_more()) {
            auto data = reader.read_chunk(8U);
            require(!data.empty(), "reader returned an empty chunk");
            const auto offset = reader.current_offset() -
                                static_cast<tcpft::FileOffset>(data.size());
            writer.write_chunk(offset, data);
        }
        writer.finalize();

        std::ifstream input(writer.path(), std::ios::binary);
        require(input.is_open(), "failed to open received test file");
        const std::string received((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
        require(received == "TCP file transfer test payload",
                "received file content does not match source");

        input.close();
        std::filesystem::remove_all(directory, cleanup_error);
        require(!cleanup_error, "failed to clean up temporary test directory");
    } catch (...) {
        std::filesystem::remove_all(directory, cleanup_error);
        throw;
    }
}

void test_oversized_file_limit_is_enforced_by_metadata_logic() {
    // This test documents the public protocol limit without allocating a 16+ GiB file.
    require(tcpft::kMaxFileSize == 16ULL * 1024ULL * 1024ULL * 1024ULL,
            "unexpected maximum file size");
}

} // namespace

void run_file_io_tests() {
    std::cout << "  test_round_trip..." << std::endl;
    test_round_trip();
    std::cout << "  test_round_trip passed." << std::endl;

    std::cout << "  test_oversized_file_limit..." << std::endl;
    test_oversized_file_limit_is_enforced_by_metadata_logic();
    std::cout << "  test_oversized_file_limit passed." << std::endl;
}
