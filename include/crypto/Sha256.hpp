#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace tcpft::crypto {

using Sha256Digest = std::array<std::uint8_t, 32>;

class Sha256 {
public:
    Sha256();

    void update(const void* data, std::size_t size);
    void update(const std::vector<std::uint8_t>& data);

    Sha256Digest finalize();
    std::string finalize_hex();

private:
    void transform(const std::uint8_t* block);

    std::array<std::uint32_t, 8> state_{};
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t buffer_size_{0};
    std::uint64_t bit_count_{0};
    bool finalized_{false};
};

Sha256Digest hash_file(const std::filesystem::path& path);
std::string to_hex(const Sha256Digest& digest);

} // namespace tcpft::crypto
