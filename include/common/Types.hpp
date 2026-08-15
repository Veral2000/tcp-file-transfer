#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace tcpft {

using FileSize = std::uint64_t;
using FileOffset = std::uint64_t;
using ChunkIndex = std::uint64_t;

constexpr std::size_t kDefaultChunkSize = 4U * 1024U * 1024U;
constexpr std::size_t kMaxChunkSize = 16U * 1024U * 1024U;
constexpr FileSize kMaxFileSize = 16ULL * 1024ULL * 1024ULL * 1024ULL;

struct FileInfo {
    std::string filename;
    FileSize size{0};
};

} // namespace tcpft
