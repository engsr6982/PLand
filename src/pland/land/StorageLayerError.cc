#include "pland/land/StorageLayerError.h"
#include "pland/utils/McUtils.h"

namespace land {


std::string StorageLayerError::getErrorMessage(Player& player, Error error) {
    switch (error) {
    case Error::Unknown:
        return "未知错误"_trf(player);
    case Error::STLMapError:
        return "操作STL Map时发生错误"_trf(player);
    case Error::DBError:
        return "数据库错误"_trf(player);
    case Error::InvalidLand:
        return "无效的领地数据"_trf(player);
    case Error::AssignLandIdFailed:
        return "分配领地ID失败"_trf(player);
    case Error::LandRangeIllegal:
        return "领地范围不合法"_trf(player);
    case Error::LandTypeWithRequireTypeNotMatch:
        return "领地类型与所需类型不匹配"_trf(player);
    default:
        return "未定义错误"_trf(player);
    }
}

void StorageLayerError::sendErrorMessage(Player& player, Error error) {
    mc_utils::sendText<mc_utils::LogLevel::Error>(player, getErrorMessage(player, error));
}


} // namespace land