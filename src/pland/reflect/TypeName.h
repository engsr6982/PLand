#pragma once
#include <string_view>

namespace land::reflect {

/**
 * 提取函数签名
 * @param sig 原始函数签名 __FUNCSIG__ (...::funcName<Target>(...))
 * @return funcName<Target>
 */
consteval std::string_view extractFunctionSignature(std::string_view sig) {
    // 查找最后一个 '('
    const size_t params_start = sig.rfind('(');
    if (params_start == std::string_view::npos) return {};

    // 从 '(' 向前倒序扫描，找到函数名的起点
    size_t cursor = params_start;
    int    depth  = 0; // 用于处理嵌套的模板尖括号 <...>

    while (cursor > 0) {
        --cursor;
        const char c = sig[cursor];

        if (c == '>') {
            depth++;
        } else if (c == '<') {
            depth--;
        } else if (depth == 0) {
            // 当前不在模板参数内部，寻找分隔符
            // 情况 A: 遇到作用域符 "::"，说明函数名结束
            if (c == ':' && cursor > 0 && sig[cursor - 1] == ':') {
                // cursor 指向第二个 ':', cursor-1 是第一个 ':'
                // 我们需要截取的是 :: 之后的内容
                return sig.substr(cursor + 1, params_start - (cursor + 1));
            }
            // 情况 B: 遇到空格 " "，通常是返回类型或调用约定与函数名的分隔
            if (c == ' ') {
                return sig.substr(cursor + 1, params_start - (cursor + 1));
            }
        }
    }

    // 情况 C: 扫描到了字符串头部（无命名空间的全局函数）
    return sig.substr(0, params_start);
}

/**
 * 提取模板参数内容
 * @param sig 函数签名 __FUNCSIG__ (...::funcName<Target>(...))
 * @return 模板参数内容 (Target)
 */
consteval std::string_view extractTemplateInner(std::string_view sig) {
    // 使用 rfind 查找最后一个 '(', 锁定函数参数列表的起始位置
    // 避开 (shared_ptr<T>) 或 void(*)(int) 等造成的干扰
    const size_t params_start = sig.rfind('(');
    if (params_start == std::string_view::npos || params_start == 0) return {};

    // 检查 '(' 前面是否是模板结束符 '>'
    // MSVC 格式: func<arg>(params)
    // 如果不是 '>'，说明不是模板函数实例化
    size_t end_bracket = params_start - 1;
    while (end_bracket > 0 && sig[end_bracket] == ' ') {
        end_bracket--;
    }

    // 如果参数表前不是 '>'，说明这不是一个模板实例化函数
    if (sig[end_bracket] != '>') return {};

    // 从 '>' 开始向左倒序扫描，寻找匹配的 '<'
    size_t cursor = end_bracket;
    int    depth  = 0;

    // 循环直到字符串开头
    while (true) {
        const char c = sig[cursor];
        if (c == '>') {
            depth++;
        } else if (c == '<') {
            depth--;
            // 当深度归零时，说明找到了最外层对应的 '<'
            if (depth == 0) {
                // 内容起始：cursor + 1
                // 内容长度：end_bracket - (cursor + 1)
                // 注意：这里使用 end_bracket 而不是 params_start - 1，以防中间有空格
                return sig.substr(cursor + 1, end_bracket - (cursor + 1));
            }
        }
        if (cursor == 0) break;
        --cursor;
    }
    return {};
}

/**
 * 提取末尾名称
 * @param full_name 全名 如 &land::RolePerms::allowDestroy
 * @return allowDestroy
 */
consteval std::string_view extractLeafName(std::string_view full_name) {
    std::string_view result = full_name;

    // 移除开头的取地址符 '&' (如果有)
    if (result.starts_with('&')) {
        result.remove_prefix(1);
    }
    // 倒序查找最后的 "::"
    // std::string_view::rfind 在编译期是可用的
    size_t last_scope = result.rfind("::");
    if (last_scope != std::string_view::npos) {
        // 截取 "::" 之后的部分 (+2 是为了跳过 "::" 的长度)
        return result.substr(last_scope + 2);
    }
    // 如果没有 "::"，说明本身就是纯名称，直接返回
    return result;
}

consteval std::string_view extractTemplateInnerLeafName(std::string_view sig) {
    return extractLeafName(extractTemplateInner(sig));
}

template <auto T>
consteval std::string_view getTemplateInnerLeafName() {
    return extractTemplateInnerLeafName(__FUNCSIG__);
}

namespace checker {

inline static constexpr std::string_view psig =
    "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl "
    "land::reflect::getTemplateInnerLeafName<&land::RolePerms::allowPlace>(void)";
inline static constexpr std::string_view psig_result = extractTemplateInnerLeafName(psig);
static_assert(psig_result == "allowPlace");

} // namespace checker

} // namespace land::reflect


namespace land::reflect::test {

struct TestStruct { bool isActive; };

consteval auto test_extractFunctionSignature() {
    constexpr auto sig = std::string_view(__FUNCSIG__);
    auto full = extractFunctionSignature(sig);
    return extractLeafName(full);  // leaf name
}
static_assert(test_extractFunctionSignature() == "test_extractFunctionSignature");

template<typename T>
consteval auto test_extractTemplateInner() {
    constexpr auto sig = std::string_view(__FUNCSIG__);
    return extractTemplateInner(sig);
}
static_assert(test_extractTemplateInner<int>() == "int");
static_assert(test_extractTemplateInner<bool>() == "bool");

static_assert(extractLeafName("ns::func") == "func");
static_assert(extractLeafName("&ns::func") == "func");

template<auto Ptr>
consteval auto test_extractTemplateInnerLeafName() {
    constexpr auto sig = std::string_view(__FUNCSIG__);
    return extractTemplateInnerLeafName(sig);
}
static_assert(test_extractTemplateInnerLeafName<&TestStruct::isActive>() == "isActive");

static_assert(getTemplateInnerLeafName<&TestStruct::isActive>() == "isActive");

} // namespace land::reflect::test