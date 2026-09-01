#pragma once
#include <cstdint>

namespace land {

enum class LandRole : uint8_t {
    Admin  = 0, // 管理员
    Owner  = 1, // 领地主人
    Member = 2, // 领地成员

    // Actor includes both non-member players and non-player entities (e.g., Mobs, TNT).
    Actor = 3, // 实体

    // 兼容旧数据
    Operator [[deprecated]] = 0, // 旧版操作员，映射到 Admin
    Guest [[deprecated]]    = 3, // 旧版访客，映射到 Actor
};

using LandPermRole [[deprecated("Use LandRole instead")]] = LandRole;
using LandPermType [[deprecated("Use LandRole instead")]] = LandRole;

} // namespace land