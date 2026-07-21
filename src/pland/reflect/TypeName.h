#pragma once
#include <string_view>

namespace land::reflect {

/**
 * 提取函数名称
 * @param sig 原始函数签名 __FUNCSIG__
 */
consteval std::string_view extractFunctionSignature(std::string_view sig) {
    const size_t params_start = sig.rfind('(');
    if (params_start == std::string_view::npos) return {};

    size_t cursor = params_start;
    int    depth  = 0;

    while (cursor > 0) {
        --cursor;
        const char c = sig[cursor];

        if (c == '>') {
            depth++;
        } else if (c == '<') {
            depth--;
        } else if (depth == 0) {
            if (c == ':' && cursor > 0 && sig[cursor - 1] == ':') {
                return sig.substr(cursor + 1, params_start - (cursor + 1));
            }
            if (c == ' ') {
                return sig.substr(cursor + 1, params_start - (cursor + 1));
            }
        }
    }
    return sig.substr(0, params_start);
}

/**
 * 提取模板参数内容（同时兼容 MSVC `<...>` 和 Clang/Clang-cl `[...]`）
 * @param sig 函数签名 __FUNCSIG__
 */
consteval std::string_view extractTemplateInner(std::string_view sig) {
    if (sig.empty()) return {};

    // -------------------------------------------------------------
    // 情况 A：Clang / Clang-cl 格式
    // 格式如：...func(void) [Ptr = &(anonymous namespace)::RolePerms::allowPlace]
    // -------------------------------------------------------------
    if (sig.ends_with(']')) {
        size_t bracket_start = sig.rfind('[');
        if (bracket_start != std::string_view::npos) {
            std::string_view inner  = sig.substr(bracket_start + 1, sig.length() - bracket_start - 2);
            size_t           eq_pos = inner.find('=');
            if (eq_pos != std::string_view::npos) {
                inner = inner.substr(eq_pos + 1);
                while (!inner.empty() && inner.front() == ' ') {
                    inner.remove_prefix(1);
                }
                return inner;
            }
        }
    }

    // -------------------------------------------------------------
    // 情况 B：MSVC 格式
    // 格式如：...func<&RolePerms::allowPlace>(void)
    // -------------------------------------------------------------
    const size_t params_start = sig.rfind('(');
    if (params_start == std::string_view::npos || params_start == 0) return {};

    size_t end_bracket = params_start - 1;
    while (end_bracket > 0 && sig[end_bracket] == ' ') {
        end_bracket--;
    }

    if (sig[end_bracket] != '>') return {};

    size_t cursor = end_bracket;
    int    depth  = 0;

    while (true) {
        const char c = sig[cursor];
        if (c == '>') {
            depth++;
        } else if (c == '<') {
            depth--;
            if (depth == 0) {
                return sig.substr(cursor + 1, end_bracket - (cursor + 1));
            }
        }
        if (cursor == 0) break;
        --cursor;
    }
    return {};
}

/**
 * 提取末尾纯名称（去除 '&', '*', 'struct/class', 'const' 及作用域前缀）
 * @param full_name 全名（如 "&land::RolePerms::allowDestroy" 或 "struct RolePerms*"）
 */
consteval std::string_view extractLeafName(std::string_view full_name) {
    std::string_view result = full_name;

    // 1. 去除开头的取地址符 '&'
    if (result.starts_with('&')) {
        result.remove_prefix(1);
    }

    // 2. 去除末尾的指针/引用/空格修饰符（如 "RolePerms*" -> "RolePerms"）
    while (!result.empty() && (result.back() == '*' || result.back() == '&' || result.back() == ' ')) {
        result.remove_suffix(1);
    }

    // 3. 截取最后一个 "::" 之后的部分（可直接去除前置作用域/匿名命名空间）
    size_t last_scope = result.rfind("::");
    if (last_scope != std::string_view::npos) {
        result = result.substr(last_scope + 2);
    }

    // 4. 处理没有 "::" 但带前缀关键字的情况（如 "struct RolePerms" -> "RolePerms"）
    size_t last_space = result.rfind(' ');
    if (last_space != std::string_view::npos) {
        result = result.substr(last_space + 1);
    }

    return result;
}

consteval std::string_view extractTemplateInnerLeafName(std::string_view sig) {
    return extractLeafName(extractTemplateInner(sig));
}

template <auto T>
consteval std::string_view getTemplateInnerLeafName() {
    return extractTemplateInnerLeafName(__FUNCSIG__);
}

template <typename T>
consteval std::string_view getTypeTemplateInnerLeafName() {
    return extractTemplateInnerLeafName(__FUNCSIG__);
}

} // namespace land::reflect


namespace {

struct GuardDummy {
    int dummyField;
};

consteval std::string_view testSelfSigHelper() { return land::reflect::extractFunctionSignature(__FUNCSIG__); }

static_assert(
    land::reflect::getTemplateInnerLeafName<&GuardDummy::dummyField>() == "dummyField",
    "[reflect error] Failed to reflect member pointer 'dummyField'!"
);

static_assert(
    land::reflect::getTypeTemplateInnerLeafName<GuardDummy>() == "GuardDummy",
    "[reflect error] Failed to reflect type 'GuardDummy'!"
);

static_assert(
    testSelfSigHelper() == "testSelfSigHelper",
    "[reflect error] Failed to extract function signature from live __FUNCSIG__!"
);

} // namespace