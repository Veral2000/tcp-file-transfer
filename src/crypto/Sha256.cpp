#include "crypto/Sha256.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace tcpft::crypto {
namespace {

constexpr std::array<std::uint32_t, 64> kRoundConstants = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

constexpr std::uint32_t rotr(std::uint32_t value, unsigned int amount) {
    return (value >> amount) | (value << (32U - amount));
}
constexpr std::uint32_t ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) ^ (~x & z); }
constexpr std::uint32_t maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
constexpr std::uint32_t big_sigma0(std::uint32_t x) { return rotr(x, 2U) ^ rotr(x, 13U) ^ rotr(x, 22U); }
constexpr std::uint32_t big_sigma1(std::uint32_t x) { return rotr(x, 6U) ^ rotr(x, 11U) ^ rotr(x, 25U); }
constexpr std::uint32_t small_sigma0(std::uint32_t x) { return rotr(x, 7U) ^ rotr(x, 18U) ^ (x >> 3U); }
constexpr std::uint32_t small_sigma1(std::uint32_t x) { return rotr(x, 17U) ^ rotr(x, 19U) ^ (x >> 10U); }

} // namespace

Sha256::Sha256()
    : state_{0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
             0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U} {}

void Sha256::transform(const std::uint8_t* block) {
    std::array<std::uint32_t, 64> schedule{};
    for (std::size_t i = 0; i < 16U; ++i) {
        const std::size_t p = i * 4U;
        schedule[i] = (static_cast<std::uint32_t>(block[p]) << 24U) |
                      (static_cast<std::uint32_t>(block[p + 1U]) << 16U) |
                      (static_cast<std::uint32_t>(block[p + 2U]) << 8U) |
                      static_cast<std::uint32_t>(block[p + 3U]);
    }
    for (std::size_t i = 16U; i < 64U; ++i) {
        schedule[i] = small_sigma1(schedule[i - 2U]) + schedule[i - 7U] +
                      small_sigma0(schedule[i - 15U]) + schedule[i - 16U];
    }
    auto a = state_[0]; auto b = state_[1]; auto c = state_[2]; auto d = state_[3];
    auto e = state_[4]; auto f = state_[5]; auto g = state_[6]; auto h = state_[7];
    for (std::size_t i = 0; i < 64U; ++i) {
        const auto t1 = h + big_sigma1(e) + ch(e, f, g) + kRoundConstants[i] + schedule[i];
        const auto t2 = big_sigma0(a) + maj(a, b, c);
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
    state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
}

void Sha256::update(const void* data, std::size_t size) {
    if (finalized_) throw std::logic_error("cannot update finalized SHA-256 context");
    if (size == 0U) return;
    if (data == nullptr) throw std::invalid_argument("SHA-256 data pointer is null");
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    bit_count_ += static_cast<std::uint64_t>(size) * 8ULL;
    while (size > 0U) {
        const std::size_t available = 64U - buffer_size_;
        const std::size_t count = size < available ? size : available;
        std::copy_n(bytes, count, buffer_.begin() + static_cast<std::ptrdiff_t>(buffer_size_));
        buffer_size_ += count; bytes += count; size -= count;
        if (buffer_size_ == 64U) { transform(buffer_.data()); buffer_size_ = 0U; }
    }
}

void Sha256::update(const std::vector<std::uint8_t>& data) { update(data.data(), data.size()); }

Sha256Digest Sha256::finalize() {
    if (finalized_) throw std::logic_error("SHA-256 context already finalized");
    finalized_ = true;
    const std::uint64_t original_bit_count = bit_count_;
    buffer_[buffer_size_++] = 0x80U;
    if (buffer_size_ > 56U) {
        std::fill(buffer_.begin() + static_cast<std::ptrdiff_t>(buffer_size_), buffer_.end(), 0U);
        transform(buffer_.data());
        buffer_size_ = 0U;
    }
    std::fill(buffer_.begin() + static_cast<std::ptrdiff_t>(buffer_size_), buffer_.begin() + 56, 0U);
    for (std::size_t i = 0; i < 8U; ++i) {
        buffer_[56U + i] = static_cast<std::uint8_t>(original_bit_count >> (56U - i * 8U));
    }
    transform(buffer_.data());
    Sha256Digest digest{};
    for (std::size_t i = 0; i < state_.size(); ++i) {
        digest[i * 4U] = static_cast<std::uint8_t>(state_[i] >> 24U);
        digest[i * 4U + 1U] = static_cast<std::uint8_t>(state_[i] >> 16U);
        digest[i * 4U + 2U] = static_cast<std::uint8_t>(state_[i] >> 8U);
        digest[i * 4U + 3U] = static_cast<std::uint8_t>(state_[i]);
    }
    return digest;
}

std::string Sha256::finalize_hex() { return to_hex(finalize()); }

Sha256Digest hash_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) throw std::runtime_error("failed to open file for SHA-256: " + path.string());
    Sha256 hasher;
    std::array<char, 64U * 1024U> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0) hasher.update(buffer.data(), static_cast<std::size_t>(count));
    }
    if (!input.eof()) throw std::runtime_error("failed while reading file for SHA-256: " + path.string());
    return hasher.finalize();
}

std::string to_hex(const Sha256Digest& digest) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const auto byte : digest) output << std::setw(2) << static_cast<unsigned int>(byte);
    return output.str();
}

} // namespace tcpft::crypto
