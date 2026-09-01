#pragma once

#include "mc/platform/UUID.h"

#include "ll/api/reflection/Deserialization.h"
#include "ll/api/reflection/Serialization.h"

#include <nlohmann/json_fwd.hpp>

#include <string>

namespace mce {

template <typename T, typename J = nlohmann::json>
inline ll::Expected<> deserialize(T& uuid, J const& j) noexcept
    requires(std::same_as<T, UUID>)
{
    if (j.is_string()) {
        auto str = j.template get<std::string>();
        if (auto res = UUID::fromString(str); res != UUID::EMPTY()) {
            uuid = res;
            return {};
        }
        return ll::makeStringError("invalid uuid");
    }
    return ll::makeStringError("field must be a string");
}

} // namespace mce