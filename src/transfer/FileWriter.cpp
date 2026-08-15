#include "transfer/FileWriter.hpp"

#include <algorithm>
#include <stdexcept>

namespace tcpft::transfer {
namespace {

bool is_safe_filename(const std::string& name) {
    if (name.empty() || name == "." || name == "..") return false;
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) return false;
    if (name.find("..") != std::string::npos) return false;
    return true;
}

} // namespace

FileWriter::FileWriter(const std::filesystem::path& directory, const FileInfo& info)
    : expected_size_(info.size) {
    if (!is_safe_filename(info.filename)) {
        throw std::runtime_error("unsafe destination filename");
    }
    if (info.size > kMaxFileSize) {
        throw std::runtime_error("file exceeds 16 GiB limit");
    }

    std::filesystem::create_directories(directory);
    path_ = directory / info.filename;
    stream_.open(path_, std::ios::binary | std::ios::trunc);
    if (!stream_) {
        throw std::runtime_error("failed to open destination file: " + path_.string());
    }
}

void FileWriter::write_chunk(FileOffset offset, const std::vector<std::uint8_t>& data) {
    if (data.empty() || data.size() > kMaxChunkSize) {
        throw std::invalid_argument("invalid chunk size");
    }
    if (offset != written_size_) {
        throw std::runtime_error("unexpected chunk offset");
    }
    if (data.size() > expected_size_ - written_size_) {
        throw std::runtime_error("chunk exceeds expected file size");
    }

    stream_.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    if (!stream_) {
        throw std::runtime_error("failed while writing destination file");
    }
    written_size_ += static_cast<FileSize>(data.size());
}

void FileWriter::finalize() {
    if (written_size_ != expected_size_) {
        throw std::runtime_error("received size does not match expected file size");
    }
    stream_.flush();
    if (!stream_) {
        throw std::runtime_error("failed to flush destination file");
    }
    stream_.close();
}

const std::filesystem::path& FileWriter::path() const noexcept {
    return path_;
}

} // namespace tcpft::transfer
