#pragma once

#include "common/Types.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace tcpft::transfer {

class FileReader {
public:
    explicit FileReader(const std::filesystem::path& path);

    const FileInfo& info() const noexcept;
    bool has_more() const noexcept;
    std::vector<std::uint8_t> read_chunk(std::size_t max_size);
    ChunkIndex current_chunk() const noexcept;
    FileOffset current_offset() const noexcept;

private:
    std::filesystem::path path_;
    FileInfo info_;
    std::ifstream stream_;
    ChunkIndex chunk_index_{0};
    FileOffset offset_{0};
};

} // namespace tcpft::transfer
