#pragma once
#include "ll/api/i18n/I18n.h"


#ifdef LDAPI_EXPORT
#define LDAPI __declspec(dllexport)
#else
#define LDAPI __declspec(dllimport)
#endif

#define LDNDAPI [[nodiscard]] LDAPI

#define LD_DISABLE_COPY(CLASS)                                                                                         \
    CLASS(CLASS const&)            = delete;                                                                           \
    CLASS& operator=(CLASS const&) = delete;

#define LD_DISALLOW_MOVE(CLASS)                                                                                        \
    CLASS(CLASS&&)            = delete;                                                                                \
    CLASS& operator=(CLASS&&) = delete;

#define LD_DISABLE_COPY_AND_MOVE(CLASS) LD_DISABLE_COPY(CLASS) LD_DISALLOW_MOVE(CLASS)

#define STATIC_ASSERT_AGGREGATE(TYPE) static_assert(std::is_aggregate_v<TYPE>, #TYPE " must be an aggregate type")


namespace mce {
class UUID;
}

namespace land {

// 全局类型定义
using LandID    = int64_t; // 领地ID
using LandDimid = int;     // 领地所在维度

inline constexpr LandID INVALID_LAND_ID = -1; // 无效的领地ID / 默认值

inline int constexpr GlobalSubLandMaxNestedLevel = 16; // 子领地最大嵌套层数

// Minecraft UUID::fromString / canParse 仅允许 0-9, a-f, A-F
// 所以这里使用一个几乎不可能冲突的合法 16 进制字符串，作为领地系统账号
inline constexpr std::string_view SYSTEM_ACCOUNT_UUID_STR = "deadbeef-dead-beef-dead-beefdeadbeef";

LDAPI extern mce::UUID const SYSTEM_ACCOUNT_UUID;

} // namespace land

namespace land {
using ll::i18n_literals::operator""_tr;
using ll::operator""_trl;
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
