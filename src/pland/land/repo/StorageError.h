#pragma once
#include "pland/Global.h"

#include "ll/api/Expected.h"

class Player;

namespace land {


struct StorageError : public ll::ErrorInfoBase {
    enum class ErrorCode : uint32_t {
        None                 = 0,      // 无错误
        Unknown              = 1 << 0, // 未知错误
        DatabaseError        = 1 << 1, // 数据库错误DD
        CacheMapError        = 1 << 2, // 缓存表错误
        InvalidLand          = 1 << 3, // 无效的领地
        LandTypeMismatch     = 1 << 4, // 领地类型不匹配
        LandRangeIllegal     = 1 << 5, // 领地范围不合法
        DataConsistencyError = 1 << 6, // 数据一致性错误
        TransactionError     = 1 << 7, // 事务错误
    };

    ErrorCode   mCode{ErrorCode::None};
    std::string mMessage; // for debug

    LDNDAPI StorageError(ErrorCode code, std::string message);

    LDNDAPI std::string message(std::string_view locale) const noexcept override;

    LDNDAPI static bool hasError(ErrorCode errors, ErrorCode flag);

    template <typename... Args>
    [[nodiscard]] static inline auto make(Args... args) {
        return ll::makeError<StorageError>(std::forward<Args>(args)...);
    }
};


} // namespace land

inline land::StorageError::ErrorCode operator|(land::StorageError::ErrorCode a, land::StorageError::ErrorCode b) {
    return static_cast<land::StorageError::ErrorCode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline land::StorageError::ErrorCode operator&(land::StorageError::ErrorCode a, land::StorageError::ErrorCode b) {
    return static_cast<land::StorageError::ErrorCode>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
