#pragma once

#include "common/Types.hpp"

#include <filesystem>
#include <fstream>

namespace tcpft::transfer {

class FileWriter {
public:
    FileWriter(const std::filesystem::path& directory, const FileInfo& info);

    void write_chunk(FileOffset offset, const std::vector<std::uint8_t>& data);
    void finalize();
    const std::filesystem::path& path() const noexcept;

private:
    std::filesystem::path path_;
    std::ofstream stream_;
    FileSize expected_size_{0};
    FileSize written_size_{0};
};

} // namespace tcpft::transfer
