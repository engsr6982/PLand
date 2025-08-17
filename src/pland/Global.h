#pragma once
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/i18n/I18n.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/platform/UUID.h"
#include <atomic>
#include <expected>
#include <filesystem>
#include <optional>
#include <type_traits>
#include <unordered_map>


class Player;

#ifdef LDAPI_EXPORT
#define LDAPI __declspec(dllexport)
#else
#define LDAPI __declspec(dllimport)
#endif

#define LDNDAPI [[nodiscard]] LDAPI

#define LD_DISALLOW_COPY(CLASS)                                                                                        \
    CLASS(CLASS const&)            = delete;                                                                           \
    CLASS& operator=(CLASS const&) = delete;

#define LD_DISALLOW_MOVE(CLASS)                                                                                        \
    CLASS(CLASS&&)            = delete;                                                                                \
    CLASS& operator=(CLASS&&) = delete;

#define LD_DISALLOW_COPY_AND_MOVE(CLASS) LD_DISALLOW_COPY(CLASS) LD_DISALLOW_MOVE(CLASS)

#define STATIC_ASSERT_AGGREGATE(TYPE) static_assert(std::is_aggregate_v<TYPE>, #TYPE " must be an aggregate type")


namespace land {

// 全局类型定义
using LandID    = int64_t;     // 领地ID
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

extern std::unordered_map<std::string, std::string> GlobalPlayerLocaleCodeCached;
LDNDAPI extern std::string                          GetPlayerLocaleCodeFromSettings(Player& player
                         ); // PLand::getInstance().getLandRegistry()->getPlayerLocaleCode


inline int constexpr GlobalSubLandMaxNestedLevel = 16; // 子领地最大嵌套层数


template <typename T, typename E = std::string>
using Result = std::expected<T, E>;

} // namespace land


// ""_trf(Player) => GetPlayerLocaleCodeFromSettings => LandRegistry::getPlayerSettings
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


#if defined(PLAND_I18N_COLLECT_STRINGS) && defined(LL_I18N_COLLECT_STRINGS) && defined(LL_I18N_COLLECT_STRINGS_CUSTOM)
namespace ll::i18n::detail {
template <LL_I18N_STRING_LITERAL_TYPE str>
struct TrStrOut {
    static inline std::string escape_for_print(std::string_view input) {
        std::string output;
        output.reserve(input.size() * 2);
        for (char c : input) {
            switch (c) {
            case '\\':
                output += "\\\\";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            case '"':
                output += "\\\"";
                break;
            default:
                output += c;
                break;
            }
        }
        return output;
    }

    static inline int _ = [] {
        fmt::print("\"{0}\": \"{0}\",\n", escape_for_print(str.sv()));
        return 0;
    }();
};
} // namespace ll::i18n::detail
#endif
