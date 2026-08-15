#include "transfer/FileReader.hpp"

#include <algorithm>
#include <stdexcept>

namespace tcpft::transfer {

FileReader::FileReader(const std::filesystem::path& path)
    : path_(path) {
    if (!std::filesystem::exists(path_) || !std::filesystem::is_regular_file(path_)) {
        throw std::runtime_error("source is not a regular file: " + path_.string());
    }

    const auto size = std::filesystem::file_size(path_);
    if (size > kMaxFileSize) {
        throw std::runtime_error("file exceeds 16 GiB limit");
    }

    info_.filename = path_.filename().string();
    info_.size = static_cast<FileSize>(size);
    stream_.open(path_, std::ios::binary);
    if (!stream_) {
        throw std::runtime_error("failed to open source file: " + path_.string());
    }
}

const FileInfo& FileReader::info() const noexcept {
    return info_;
}

bool FileReader::has_more() const noexcept {
    return offset_ < info_.size;
}

std::vector<std::uint8_t> FileReader::read_chunk(std::size_t max_size) {
    if (max_size == 0U || max_size > kMaxChunkSize) {
        throw std::invalid_argument("invalid chunk size");
    }
    if (!has_more()) {
        return {};
    }

    const auto remaining = info_.size - offset_;
    const auto requested = std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(max_size));
    std::vector<std::uint8_t> data(static_cast<std::size_t>(requested));
    stream_.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    const auto actual = stream_.gcount();
    if (actual != static_cast<std::streamsize>(data.size())) {
        throw std::runtime_error("failed while reading source file");
    }

    offset_ += static_cast<FileOffset>(data.size());
    ++chunk_index_;
    return data;
}

ChunkIndex FileReader::current_chunk() const noexcept {
    return chunk_index_ == 0U ? 0U : chunk_index_ - 1U;
}

FileOffset FileReader::current_offset() const noexcept {
    return offset_;
}

} // namespace tcpft::transfer
