#include "crypto/Sha256.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void test_known_vector(const std::string& input, const std::string& expected) {
    tcpft::crypto::Sha256 hasher;
    hasher.update(input.data(), input.size());
    require(hasher.finalize_hex() == expected, "SHA-256 known vector mismatch");
}

void test_incremental_update() {
    tcpft::crypto::Sha256 hasher;
    const std::string first = "The quick brown ";
    const std::string second = "fox jumps over ";
    const std::string third = "the lazy dog";
    hasher.update(first.data(), first.size());
    hasher.update(second.data(), second.size());
    hasher.update(third.data(), third.size());
    require(hasher.finalize_hex() ==
                "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592",
            "incremental SHA-256 mismatch");
}

void test_hash_file() {
    const auto path = std::filesystem::temp_directory_path() / "tcpft_sha256_test.txt";
    {
        std::ofstream output(path, std::ios::binary);
        require(output.is_open(), "failed to create SHA-256 test file");
        output << "abc";
    }

    const auto digest = tcpft::crypto::hash_file(path);
    std::error_code error;
    std::filesystem::remove(path, error);
    require(!error, "failed to remove SHA-256 test file");
    require(tcpft::crypto::to_hex(digest) ==
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "file SHA-256 mismatch");
}

} // namespace

void run_sha256_tests() {
    test_known_vector("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    test_known_vector("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    test_incremental_update();
    test_hash_file();
}
