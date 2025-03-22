#pragma once
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/i18n/I18n.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/platform/UUID.h"
#include <atomic>
#include <filesystem>
#include <unordered_map>

class Player;

#ifdef LDAPI_EXPORT
#define LDAPI __declspec(dllexport)
#else
#define LDAPI __declspec(dllimport)
#endif

#define LDNDAPI [[nodiscard]] LDAPI


namespace land {

// 全局类型定义
using LandID    = uint64_t;    // 领地ID
using ChunkID   = uint64_t;    // 区块ID
using LandDimid = int;         // 领地所在维度
using UUIDm     = mce::UUID;   // class
using UUIDs     = std::string; // string

enum class LandPermType : int {
    Operator = 0, // 领地操作员（管理）
    Owner,        // 领地主人
    Member,       // 领地成员
    Guest,        // 访客
};

extern std::atomic<bool> GlobalRepeatCoroTaskRunning;

extern std::unordered_map<std::string, std::string> GlobalPlayerLocaleCodeCached;
LDNDAPI extern std::string GetPlayerLocaleCodeFromSettings(Player& player); // PLand::getInstance().getPlayerLocaleCode


inline int constexpr GlobalSubLandMaxNestedLevel = 16; // 子领地最大嵌套层数


} // namespace land


// ""_trf(Player) => GetPlayerLocaleCodeFromSettings => PLand::getPlayerSettings
namespace ll::inline literals::inline i18n_literals {
template <LL_I18N_STRING_LITERAL_TYPE Fmt>
[[nodiscard]] constexpr auto operator""_trf() {
#ifdef LL_I18N_COLLECT_STRINGS
    static i18n::detail::TrStrOut<Fmt> e{};
#endif
    return [=]<class... Args>(Player& player, Args&&... args) {
        [[maybe_unused]] static constexpr auto checker = fmt::format_string<Args...>(Fmt.sv());
        return fmt::vformat(
            i18n::getInstance().get(Fmt.sv(), land::GetPlayerLocaleCodeFromSettings(player)),
            fmt::make_format_args(args...)
        );
    };
}
} // namespace ll::inline literals::inline i18n_literals


namespace land {
using string = std::string;
using ll::i18n_literals::operator""_tr;
using ll::i18n_literals::operator""_trf;    // 自定义 i18n 字符串格式化, 从玩家设置中获取语言代码
using ll::chrono_literals::operator""_tick; // 1s = 20_tick
namespace fs = std::filesystem;
} // namespace land
