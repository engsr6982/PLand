#pragma once
#include <cstdint>
#include <utility>

namespace land::internal {

using ChunkID = std::uint64_t;

struct ChunkEncoder {
    ChunkEncoder() = delete;

    inline static constexpr ChunkID encode(std::int32_t x, std::int32_t z) noexcept {
        return (static_cast<ChunkID>(static_cast<std::uint32_t>(x)) << 32) | static_cast<std::uint32_t>(z);
    }

    inline static constexpr std::pair<std::int32_t, std::int32_t> decode(ChunkID id) noexcept {
        return {static_cast<std::int32_t>(id >> 32), static_cast<std::int32_t>(id)};
    }
};


} // namespace land::internal