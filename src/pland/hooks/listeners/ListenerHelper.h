
#pragma once

#include "mc/world/level/block/BlockProperty.h"

#include "pland/PLand.h"
#include "pland/land/Land.h"
#include "pland/land/LandRegistry.h"

// 这些宏依赖于一个名为 'ev' 的事件变量存在于其作用域中
#define CANCEL_EVENT_AND_RETURN                                                                                        \
    ev.cancel();                                                                                                       \
    return;

#define CANCEL_AND_RETURN_IF(COND)                                                                                     \
    if (COND) {                                                                                                        \
        CANCEL_EVENT_AND_RETURN                                                                                        \
    }

namespace land {

// 共享的权限检查辅助函数
inline bool PreCheckLandExistsAndPermission(SharedLand const& ptr, UUIDs const& uuid = "") {
    if (!ptr ||                                                       // 无领地
        (PLand::getInstance().getLandRegistry()->isOperator(uuid)) || // 管理员
        (ptr->getPermType(uuid) != LandPermType::Guest)               // 主人/成员
    ) {
        return true;
    }
    return false;
}

// 修复 BlockProperty 的 operator&
inline BlockProperty operator&(BlockProperty a, BlockProperty b) {
    return static_cast<BlockProperty>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

} // namespace land
