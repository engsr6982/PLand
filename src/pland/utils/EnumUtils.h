#pragma once
#include <concepts>
#include <type_traits>

namespace land::inline enum_util {

#define LD_IMPL_ENUM_OPERATOR(ENUM, OP)                                                                                \
    inline constexpr ENUM operator OP(ENUM lhs, ENUM rhs) {                                                            \
        using underlying = std::underlying_type_t<ENUM>;                                                               \
        return static_cast<ENUM>(static_cast<underlying>(lhs) OP static_cast<underlying>(rhs));                        \
    }


template <typename T>
    requires std::is_enum_v<T>
inline constexpr bool hasFlag(T value, T flag) {
    using U = std::underlying_type_t<T>;
    return (static_cast<U>(value) & static_cast<U>(flag)) == static_cast<U>(flag);
}

template <typename T, std::same_as<T>... F>
    requires std::is_enum_v<T> && (sizeof...(F) > 0)
inline constexpr bool hasAllFlags(T value, F... flags) {
    return (hasFlag(value, flags) && ...);
}

template <typename T, std::same_as<T>... F>
    requires std::is_enum_v<T> && (sizeof...(F) > 0)
inline constexpr bool hasAnyFlag(T value, F... flags) {
    return (hasFlag(value, flags) || ...);
}


} // namespace land::inline enum_util