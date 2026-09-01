#pragma once

#include "ll/api/reflection/Deserialization.h"
#include "ll/api/reflection/Serialization.h"

#include <nlohmann/detail/value_t.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace land::json_util {

using json_t = nlohmann::ordered_json;

enum class MergeResult : std::uint8_t {
    Unchanged = 0, /// 数据未发生改变，无需更新/写入磁盘
    Modified  = 1  /// 数据已修补/更正（新增、删除或类型覆写），需要刷新写入磁盘
};

// ============================================================================
// Concepts
// ============================================================================

/// @brief 检查类型是否支持 LL 反射且包含可转为整型的 version 字段
template <typename T>
concept HasVersion =
    ll::reflection::Reflectable<T> && std::integral<std::remove_cvref_t<decltype(std::declval<T>().version)>>;

/// @brief 判定某个 JSON Pointer 路径对应的节点是否为自定义 Map (如 std::unordered_map)
template <typename F, typename J = json_t>
concept CustomMapPredicate = std::is_invocable_r_v<bool, F, std::string_view, const J&>;

// ============================================================================
// Internal Implementation Details
// ============================================================================

namespace detail {

/// @brief 默认谓词：将所有 JSON Object 视为固定结构体（自动清理多余的废弃 Key）
struct DefaultCustomMapPredicate {
    template <typename J>
    constexpr bool operator()(std::string_view, const J&) const noexcept {
        return false;
    }
};

} // namespace detail

static_assert(CustomMapPredicate<detail::DefaultCustomMapPredicate>);

// ============================================================================
// Basic Conversion Utilities
// ============================================================================

/// @brief 将 C++ 结构体序列化为 JSON 对象
template <typename T, typename J = json_t>
[[nodiscard]] inline J struct_to_json(const T& obj) {
    return ll::reflection::serialize<J>(obj).value();
}

/// @brief 从 JSON 对象反序列化填充 C++ 结构体
template <typename T, typename J = json_t>
inline auto json_to_struct(const J& json, T& obj) {
    return ll::reflection::deserialize(obj, json);
}

// ============================================================================
// Core Configuration Merging
// ============================================================================

/**
 * @brief 合并默认配置 (default_json) 到用户配置 (user_json)。
 *
 * 行为规则：
 * 1. [类型不匹配]：若 user_json 与 default_json 类型不符，判定 user_json 为非法输入，直接用 default_json 覆盖。
 * 2. [基础标量]：若 user_json 存在且类型匹配，保留 user_json 值；若缺失则从 default_json 补全。
 * 3. [数组处理]：保持 user_json 固有顺序在前，去重追加 default_json 中的新元素在后。
 * 4. [对象与自定义字典]：
 *    - 通过 is_custom_map(path, node) 判断当前对象是否为自定义 Map（如 std::unordered_map）。
 *    - 普通结构体对象：移除 user_json 中多余的非法/废弃 Key。
 *    - 自定义字典对象：保留用户在 user_json 中自由扩展的 Key。
 *
 * @return MergeResult::Modified 表示配置发生修改（需要落盘）；MergeResult::Unchanged 表示完全一致。
 */
template <typename J = json_t, CustomMapPredicate<J> Pred = detail::DefaultCustomMapPredicate>
inline MergeResult merge(const J& default_json, J& user_json, Pred&& is_custom_map = {}) {
    using type_t = nlohmann::detail::value_t;

    MergeResult result = MergeResult::Unchanged;

    struct StackNode {
        J const*                 s_node;
        typename J::json_pointer path;
    };

    std::vector<StackNode> stack;
    stack.reserve(32);
    stack.push_back({&default_json, typename J::json_pointer("")});

    while (!stack.empty()) {
        auto [s_ptr, path] = std::move(stack.back());
        stack.pop_back();

        J const& s = *s_ptr;
        J&       t = user_json[path];

        type_t stype = s.type();
        type_t ttype = t.type();

        // 数值类型的编码形态差异 (number_integer / number_unsigned / number_float) 不是类型冲突。
        // 例如: 从 CBOR 解码的非负整数是 number_unsigned, 而 C++ 默认值序列化出来是 number_integer,
        // 若按"类型不匹配"处理会拿默认值覆盖用户值, 导致数据丢失
        // (典型: mLandID:0 被覆盖成默认 -1, 所有未达当前版本的库记录升级后被跳过)。
        // 只有真正的类型冲突 (如 number vs string) 才回退默认值。
        constexpr auto isNumeric = [](type_t t) {
            return t == type_t::number_integer || t == type_t::number_unsigned || t == type_t::number_float;
        };

        // 类型不匹配，优先参考 s (default_json)，覆盖非法输入
        if (stype != ttype && !(isNumeric(stype) && isNumeric(ttype))) {
            t      = s;
            result = MergeResult::Modified;
            continue;
        }

        switch (stype) {
        case type_t::boolean:
        case type_t::number_integer:
        case type_t::number_unsigned:
        case type_t::number_float:
        case type_t::string:
        case type_t::null:
            // user_json 已有有效值且类型匹配，保留不修改
            break;

        case type_t::array: {
            // 数组去重 + 保持顺序 (user_json 在前，default_json 追加在后)
            for (const auto& s_elem : s) {
                if (std::find(t.begin(), t.end(), s_elem) == t.end()) {
                    t.push_back(s_elem);
                    result = MergeResult::Modified;
                }
            }
            break;
        }

        case type_t::object: {
            std::string path_str = path.to_string();
            const bool  is_map   = is_custom_map(path_str, s);

            // 若不是自定义 map，剥离 user_json 中多余的非法/废弃 Key
            if (!is_map) {
                std::vector<std::string> keys_to_remove;
                for (auto it = t.begin(); it != t.end(); ++it) {
                    if (!s.contains(it.key())) {
                        keys_to_remove.push_back(it.key());
                    }
                }
                if (!keys_to_remove.empty()) {
                    for (const auto& k : keys_to_remove) {
                        t.erase(k);
                    }
                    result = MergeResult::Modified;
                }
            }

            // 补全缺失 Key 并收集嵌套节点
            std::vector<StackNode> pending_children;
            pending_children.reserve(s.size());

            for (auto& [k, v] : s.items()) {
                if (!t.contains(k)) {
                    t[k]   = v; // 补全默认配置
                    result = MergeResult::Modified;
                } else {
                    pending_children.push_back({&v, path / k});
                }
            }

            for (auto it = pending_children.rbegin(); it != pending_children.rend(); ++it) {
                stack.push_back(std::move(*it));
            }
            break;
        }

        default:
            break;
        }
    }

    return result;
}

// ============================================================================
// High-Level Configuration Patching & Deserialization
// ============================================================================

/**
 * @brief 先用 obj 中的默认配置对 user_json 进行差异合并/修补，再反序列化回 obj。
 * @param user_json 需要修补的用户 JSON 配置对象
 * @param obj 默认配置结构体输入，同时作为最终反序列化的输出目标
 * @param is_custom_map 判断某个路径节点是否为自定义字典的谓词
 * @return MergeResult::Modified 表示配置被修改；MergeResult::Unchanged 表示配置无变化。
 */
template <typename T, typename J = json_t, CustomMapPredicate<J> Pred = detail::DefaultCustomMapPredicate>
inline MergeResult merge_and_deserialize(J& user_json, T& obj, Pred&& is_custom_map = {}) {
    auto        default_json = struct_to_json<T, J>(obj);
    MergeResult result       = merge(default_json, user_json, is_custom_map);
    json_to_struct(user_json, obj);
    return result;
}

/**
 * @brief 带版本检查的配置修补与反序列化。
 *        当 user_json 中的 version 不匹配或 force_patch 为 true 时触发合并，最后反序列化回 obj。
 * @param user_json 需要修补的用户 JSON 配置对象
 * @param obj 包含 version 字段的默认配置结构体，同时作为最终反序列化的输出目标
 * @param force_patch 是否强制开启合并，忽略版本号检查
 * @param is_custom_map 判断某个路径节点是否为自定义字典的谓词
 * @return MergeResult::Modified 表示配置发生变更（含版本号更新或字段变更）；MergeResult::Unchanged 表示无需刷盘。
 */
template <HasVersion T, typename J = json_t, CustomMapPredicate<J> Pred = detail::DefaultCustomMapPredicate>
inline MergeResult
merge_versioned_and_deserialize(J& user_json, T& obj, bool force_patch = false, Pred&& is_custom_map = {}) {
    MergeResult result = MergeResult::Unchanged;

    bool need_merge = force_patch;

    // 检查 version 字段是否缺失或不一致
    if (!user_json.contains("version") || !user_json["version"].is_number_integer()
        || user_json["version"].template get<std::int64_t>() != static_cast<std::int64_t>(obj.version)) {
        need_merge = true;
    }

    if (need_merge) {
        user_json["version"] = static_cast<std::int64_t>(obj.version);

        auto default_json = struct_to_json<T, J>(obj);
        result            = merge(default_json, user_json, is_custom_map);
    }

    json_to_struct(user_json, obj);
    return result;
}

} // namespace land::json_util