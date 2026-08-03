#include "LandDatabase.h"

#include "ll/api/data/KeyValueDB.h"

namespace land ::internal {

/// static

std::filesystem::path LandDatabase::getDatabasePath(std::filesystem::path dataDir) {
    return dataDir / kDatabaseDirName;
}

std::string LandDatabase::buildLandContextKey(LandID id) { return fmt::format("{}{}", kLandContextPrefix, id); }

bool LandDatabase::isLandContext(std::string_view key) { return key.starts_with(kLandContextPrefix); }


/// helper
namespace {
constexpr uint64_t fnv1a(std::span<std::byte const> data) noexcept {
    static constexpr uint64_t kOffsetBasis = 14695981039346656037ULL; // 0xcbf29ce484222325ULL
    static constexpr uint64_t kPrime       = 1099511628211ULL;        // 0x100000001b3ULL

    uint64_t hash = kOffsetBasis;
    for (std::byte b : data) {
        hash ^= std::to_integer<uint64_t>(b);
        hash *= kPrime;
    }
    return hash;
}
uint64_t fnv1a(std::string_view sv) noexcept {
    // C++20 的 constexpr 不允许 reinterpret_cast,故仅 span 版本保持 constexpr
    return fnv1a(std::span<std::byte const>{reinterpret_cast<std::byte const*>(sv.data()), sv.size()});
}

using checksum_t                = uint64_t;
constexpr size_t kChecksumBytes = sizeof(checksum_t);
static_assert(kChecksumBytes == 8);

[[maybe_unused]] constexpr checksum_t byteswap64(checksum_t v) noexcept {
    return ((v & 0x00000000000000FFULL) << 56) | ((v & 0x000000000000FF00ULL) << 40)
         | ((v & 0x0000000000FF0000ULL) << 24) | ((v & 0x00000000FF000000ULL) << 8) | ((v & 0x000000FF00000000ULL) >> 8)
         | ((v & 0x0000FF0000000000ULL) >> 24) | ((v & 0x00FF000000000000ULL) >> 40)
         | ((v & 0xFF00000000000000ULL) >> 56);
}

void writeUint64LE(checksum_t val, std::span<std::byte, kChecksumBytes> dst) noexcept {
    // 如果当前环境是大端序，在写入前翻转为小端序
    if constexpr (std::endian::native == std::endian::big) {
        val = byteswap64(val);
    }

    auto srcBytes = std::bit_cast<std::array<std::byte, kChecksumBytes>>(val);
    std::copy_n(srcBytes.begin(), kChecksumBytes, dst.begin());
}
checksum_t readUint64LE(std::span<std::byte const, kChecksumBytes> src) noexcept {
    std::array<std::byte, kChecksumBytes> bytesBuffer;
    std::copy_n(src.begin(), kChecksumBytes, bytesBuffer.begin());

    auto val = std::bit_cast<checksum_t>(bytesBuffer);

    if constexpr (std::endian::native == std::endian::big) {
        val = byteswap64(val);
    }
    return val;
}
} // namespace


/// instance

LandDatabase::LandDatabase(std::filesystem::path const& path) : mDb(std::make_unique<ll::data::KeyValueDB>(path)) {}
LandDatabase::~LandDatabase() = default;

ll::Expected<int64_t> LandDatabase::snapshotTo(ll::data::KeyValueDB& other) {
    int64_t counter = 0;

    auto iter = mDb->iter();
    for (auto [k, v] : iter) {
        other.set(k, v);
        counter++;
    }
    return counter;
}

std::optional<int32_t> LandDatabase::getVersion() const {
    auto raw = mDb->get(kDatabaseVersionKey);
    if (!raw) return std::nullopt;

    try {
        return std::stoi(*raw);
    } catch (...) {
        [[unlikely]] return std::nullopt;
    }
}

bool LandDatabase::setVersion(int32_t version) { return mDb->set(kDatabaseVersionKey, std::to_string(version)); }

bool LandDatabase::empty() const { return mDb->empty(); }

bool LandDatabase::has(std::string_view key) const { return mDb->has(key); }

bool LandDatabase::del(std::string_view key) { return mDb->del(key); }

bool LandDatabase::set(std::string_view key, std::string_view val) { return mDb->set(key, val); }

std::optional<std::string> LandDatabase::get(std::string_view key) const { return mDb->get(key); }

ll::coro::Generator<LandDatabase::DataView> LandDatabase::iter(std::string_view prefix) const {
    auto iter = mDb->iter();
    for (auto [k, v] : iter) {
        if (k.starts_with(prefix)) {
            co_yield DataView{v};
        }
    }
}

bool LandDatabase::saveBinary(std::string_view key, std::string buffer) {
    appendChecksum(buffer);
    return mDb->set(key, buffer);
}

std::optional<std::string> LandDatabase::readBinary(std::string_view key) const {
    auto raw = mDb->get(key);
    if (!raw) return std::nullopt;

    if (auto striped = stripChecksum(*raw)) {
        return std::string{*striped};
    }
    return std::nullopt;
}

void LandDatabase::appendChecksum(std::string& buffer) {
    checksum_t checksum = fnv1a(buffer);

    std::array<std::byte, kChecksumBytes> headerBuffer{};
    writeUint64LE(checksum, headerBuffer);

    buffer.insert(0, reinterpret_cast<char const*>(headerBuffer.data()), kChecksumBytes);
}

std::optional<std::string_view> LandDatabase::stripChecksum(std::string_view raw) noexcept {
    if (raw.size() < kChecksumBytes) {
        return std::nullopt;
    }

    auto dynamicBytes = std::as_bytes(std::span{raw.data(), raw.size()});
    auto headerBytes  = dynamicBytes.first<kChecksumBytes>();

    checksum_t expectedChecksum = readUint64LE(headerBytes);

    std::string_view payloadView = raw.substr(kChecksumBytes);

    checksum_t actualChecksum = fnv1a(payloadView);
    if (expectedChecksum != actualChecksum) {
        [[unlikely]] return std::nullopt;
    }
    return payloadView;
}


} // namespace land::internal