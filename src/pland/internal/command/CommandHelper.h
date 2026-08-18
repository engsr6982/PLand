#pragma once

#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/utils/EnumUtils.h"

#include "ll/api/command/Command.h"
#include "ll/api/i18n/I18n.h"

#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"

#include "magic_enum/magic_enum.hpp"

#include "fmt/compile.h"
#include "fmt/format.h"

#include <concepts>
#include <type_traits>


namespace land::internal {


template <CommandOriginType... AcceptOrigins>
struct LandCommandAcceptOrigin {
    inline static constexpr size_t                                kCount   = sizeof...(AcceptOrigins);
    inline static constexpr std::array<CommandOriginType, kCount> kOrigins = {AcceptOrigins...};

    inline static constexpr bool kAcceptDedicatedServer = isAccepted(CommandOriginType::DedicatedServer);
    inline static constexpr bool kAcceptPlayer          = isAccepted(CommandOriginType::Player);

    inline static constexpr bool isAccepted(CommandOriginType type) {
        if constexpr (kCount == 0) return false;
        else return ((type == AcceptOrigins) || ...);
    }

    inline static std::string getAcceptedOrigins() {
        if constexpr (kCount == 0) return "None";

        std::string result;
        auto        append = [&](CommandOriginType type) {
            if (!result.empty()) fmt::format_to(std::back_inserter(result), ", ");
            fmt::format_to(std::back_inserter(result), "{}", magic_enum::enum_name(type));
        };

        (append(AcceptOrigins), ...);
        return result;
    }
};

enum class LandCommandPermission {
    kNone              = 0,
    kMinecraftOperator = 1 << 0,
    kLandAdmin         = 1 << 1,
};


namespace detail {


template <typename AcceptOrigin = LandCommandAcceptOrigin<>, LandCommandPermission P = LandCommandPermission::kNone>
inline bool handleOrigin(CommandOrigin const& ori, CommandOutput& out) {
    constexpr bool requiresPermission = (P != LandCommandPermission::kNone);
    static_assert(
        !requiresPermission || AcceptOrigin::kAcceptPlayer,
        "Command with permission requirement MUST accept Player origin!"
    );

    auto    type   = ori.getOriginType();
    Player* player = nullptr;

    auto localeCode = std::string{ll::i18n::getDefaultLocaleCode()};
    if (type == CommandOriginType::Player) {
        if (auto actor = ori.getEntity(); actor && actor->isPlayer()) {
            player     = static_cast<Player*>(actor);
            localeCode = player->getLocaleCode();
        }
    }

    if (!AcceptOrigin::isAccepted(type)) {
        out.error("Command can only be executed by '{}'"_trl(localeCode, AcceptOrigin::getAcceptedOrigins()));
        return false;
    }

    if constexpr (!requiresPermission) {
        return true;
    }

    if (!player) {
        out.error("This command requires player permissions and cannot be executed by non-players."_trl(localeCode));
        return false;
    }

    if (hasFlag(P, LandCommandPermission::kMinecraftOperator)) {
        if (!player->isOperator()) {
            out.error("Requires Minecraft Operator permission."_trl(localeCode));
            return false;
        }
    }

    if (hasFlag(P, LandCommandPermission::kLandAdmin)) {
        auto& registry = PLand::getInstance().getLandRegistry();
        if (!registry.isOperator(player->getUuid())) {
            out.error("Requires Land Admin permission."_trl(localeCode));
            return false;
        }
    }

    return true;
}


// Command Handler 参数类型推导 (DeduceParamsT)
template <typename T>
struct HandlerTraits;

// 成员函数指针 (Lambda operator())
template <typename C, typename R, typename A1, typename A2, typename A3>
struct HandlerTraits<R (C::*)(A1, A2, A3) const> {
    using Params = std::remove_cvref_t<A3>;
};

template <typename C, typename R, typename A1, typename A2, typename A3>
struct HandlerTraits<R (C::*)(A1, A2, A3)> {
    using Params = std::remove_cvref_t<A3>;
};

template <typename C, typename R, typename A1, typename A2>
struct HandlerTraits<R (C::*)(A1, A2) const> {
    using Params = ll::command::EmptyParam;
};

template <typename C, typename R, typename A1, typename A2>
struct HandlerTraits<R (C::*)(A1, A2)> {
    using Params = ll::command::EmptyParam;
};

// 普通函数指针
template <typename R, typename A1, typename A2, typename A3>
struct HandlerTraits<R (*)(A1, A2, A3)> {
    using Params = std::remove_cvref_t<A3>;
};

template <typename R, typename A1, typename A2>
struct HandlerTraits<R (*)(A1, A2)> {
    using Params = ll::command::EmptyParam;
};

// Lambda / 仿函数类
template <typename T>
    requires std::is_class_v<std::remove_cvref_t<T>>
struct HandlerTraits<T> : HandlerTraits<decltype(&std::remove_cvref_t<T>::operator())> {};

// 指向 Lambda 对象的指针
template <typename T>
    requires(
        std::is_pointer_v<std::remove_cvref_t<T>> && std::is_class_v<std::remove_pointer_t<std::remove_cvref_t<T>>>
    )
struct HandlerTraits<T> : HandlerTraits<std::remove_pointer_t<std::remove_cvref_t<T>>> {};

template <typename T>
using DeduceParamsT = typename HandlerTraits<std::remove_cvref_t<T>>::Params;

} // namespace detail


// wrapCommandHandler<&foo, AcceptOrigin, Permission>()
template <
    auto Fn,
    typename AcceptOrigin   = LandCommandAcceptOrigin<>,
    LandCommandPermission P = LandCommandPermission::kNone>
decltype(auto) wrapCommandHandler() {
    using RawFnType = decltype(Fn);
    using Params    = detail::DeduceParamsT<RawFnType>;

    auto invoke_fn = [](auto&&... args) {
        if constexpr (std::is_pointer_v<RawFnType>) {
            return std::invoke(*Fn, std::forward<decltype(args)>(args)...);
        } else {
            return std::invoke(Fn, std::forward<decltype(args)>(args)...);
        }
    };

    if constexpr (std::is_same_v<Params, ll::command::EmptyParam>) {
        return [invoke_fn](CommandOrigin const& ori, CommandOutput& out) {
            if (detail::handleOrigin<AcceptOrigin, P>(ori, out)) {
                invoke_fn(ori, out);
            }
        };
    } else {
        return [invoke_fn](CommandOrigin const& ori, CommandOutput& out, Params const& params) {
            if (detail::handleOrigin<AcceptOrigin, P>(ori, out)) {
                invoke_fn(ori, out, params);
            }
        };
    }
}

//  wrapCommandHandler<AcceptOrigin, Permission>([](...) {})
template <
    typename AcceptOrigin   = LandCommandAcceptOrigin<>,
    LandCommandPermission P = LandCommandPermission::kNone,
    typename Fn>
decltype(auto) wrapCommandHandler(Fn&& fn) {
    using Params = detail::DeduceParamsT<std::decay_t<Fn>>;

    if constexpr (std::is_same_v<Params, ll::command::EmptyParam>) {
        return [fn = std::forward<Fn>(fn)](CommandOrigin const& ori, CommandOutput& out) mutable {
            if (detail::handleOrigin<AcceptOrigin, P>(ori, out)) {
                std::invoke(fn, ori, out);
            }
        };
    } else {
        return [fn = std::forward<Fn>(fn)](CommandOrigin const& ori, CommandOutput& out, Params const& params) mutable {
            if (detail::handleOrigin<AcceptOrigin, P>(ori, out)) {
                std::invoke(fn, ori, out, params);
            }
        };
    }
}


} // namespace land::internal

LD_IMPL_ENUM_OPERATOR(land::internal::LandCommandPermission, |)
LD_IMPL_ENUM_OPERATOR(land::internal::LandCommandPermission, &)