#include "StorageError.h"
#include "pland/utils/McUtils.h"

namespace land {


StorageError::StorageError(ErrorCode code, std::string message) : mCode(code), mMessage(std::move(message)) {}
std::string StorageError::message(std::string_view) const noexcept { return mMessage; }
bool        StorageError::hasError(ErrorCode errors, ErrorCode flag) { return (errors & flag) != ErrorCode::None; }


} // namespace land
