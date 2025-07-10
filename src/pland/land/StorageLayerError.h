#pragma once
#include "pland/Global.h"

class Player;

namespace land {


struct StorageLayerError {
    enum class Error {
        Unknown              = 0, // 未知错误
        STLMapError          = 1, // STL Map 操作失败
        DBError              = 2, // 数据库操作失败
        DataConsistencyError = 3, // 数据一致性错误

        // addLand
        InvalidLand        = 100, // 无效的领地数据
        AssignLandIdFailed = 101, // 分配领地ID失败

        // removeLand
        LandTypeWithRequireTypeNotMatch = 200, // 领地类型与要求类型不匹配
    };

    LDNDAPI static std::string getErrorMessage(Player& player, Error error);
    LDAPI static void          sendErrorMessage(Player& player, Error error);
};


} // namespace land