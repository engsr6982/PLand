#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include "../src/pland/reflect/TypeName.h"


namespace {

struct RolePerms {
    bool allowPlace;
    bool allowDestroy;
    bool useRepeater;
    bool useBeeNest;
};

struct TestStruct {
    bool isActive;
};

// -----------------------------------------------------------------------------
// 终端 ANSI 转义常量
// -----------------------------------------------------------------------------
constexpr std::string_view COLOR_RESET = "\033[0m";
constexpr std::string_view COLOR_PASS  = "\033[1;32m"; // 亮绿
constexpr std::string_view COLOR_FAIL  = "\033[1;31m"; // 亮红
constexpr std::string_view COLOR_CYAN  = "\033[0;36m"; // 青色

// -----------------------------------------------------------------------------
// 原始签名获取 Helper
// -----------------------------------------------------------------------------
template <auto Ptr>
constexpr std::string_view getRawSig() {
    return __FUNCSIG__;
}

template <typename T>
constexpr std::string_view getTypeRawSig() {
    return __FUNCSIG__;
}

void check(std::string_view test_name, std::string_view raw_sig, std::string_view actual, std::string_view expected) {
    bool pass = (actual == expected);

    std::string_view status_tag   = pass ? "[ PASS ]" : "[ FAIL ]";
    std::string_view status_color = pass ? COLOR_PASS : COLOR_FAIL;
    std::string_view display_raw  = raw_sig.empty() ? "(None)" : raw_sig;

    std::cout << "==================================================\n";
    std::cout << std::format("{}{}{} {}\n", status_color, status_tag, COLOR_RESET, test_name);
    std::cout << std::format("  [Raw __FUNCSIG__] : {}{}{}\n", COLOR_CYAN, display_raw, COLOR_RESET);
    std::cout << std::format("  [Actual Result]   : \"{}\"\n", actual);
    std::cout << std::format("  [Expected]        : \"{}\"\n", expected);

    if (!pass) {
        std::cout << std::format("{}  >>> DISCREPANCY DETECTED! <<<{}\n", COLOR_FAIL, COLOR_RESET);
    }
    std::cout << "\n";
}

} // namespace


int main() {
    using namespace land;

    std::cout << "\n>>>> Full Reflect Test Coverage <<<<\n\n";

    // -------------------------------------------------------------------------
    // 测试组 1：成员指针 (值模板参数 auto Ptr)
    // -------------------------------------------------------------------------
    {
        constexpr auto raw    = getRawSig<&RolePerms::allowPlace>();
        auto           actual = std::string{reflect::getTemplateInnerLeafName<&RolePerms::allowPlace>()};
        check("getTemplateInnerLeafName<&RolePerms::allowPlace>()", raw, actual, "allowPlace");
    }

    {
        constexpr auto raw    = getRawSig<&RolePerms::useRepeater>();
        auto           actual = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useRepeater>()};
        check("getTemplateInnerLeafName<&RolePerms::useRepeater>()", raw, actual, "useRepeater");
    }

    // -------------------------------------------------------------------------
    // 测试组 2：类型模板参数 (typename T)
    // -------------------------------------------------------------------------
    {
        constexpr auto raw    = getTypeRawSig<RolePerms>();
        auto           actual = std::string{reflect::getTypeTemplateInnerLeafName<RolePerms>()};
        check("getTypeTemplateInnerLeafName<RolePerms>()", raw, actual, "RolePerms");
    }

    {
        constexpr auto raw    = getTypeRawSig<TestStruct>();
        auto           actual = std::string{reflect::getTypeTemplateInnerLeafName<TestStruct>()};
        check("getTypeTemplateInnerLeafName<TestStruct>()", raw, actual, "TestStruct");
    }

    // -------------------------------------------------------------------------
    // 测试组 3：extractFunctionSignature 提取函数名
    // -------------------------------------------------------------------------
    {
        constexpr auto raw    = __FUNCSIG__;
        auto           actual = reflect::extractLeafName(reflect::extractFunctionSignature(raw));
        check("extractFunctionSignature(__FUNCSIG__) -> main", raw, actual, "main");
    }

    {
        constexpr std::string_view mock_ns_sig = "void __cdecl dummy_ns::dummy_function(void)";
        auto                       actual      = reflect::extractFunctionSignature(mock_ns_sig);
        check("extractFunctionSignature(\"dummy_ns::dummy_function\")", mock_ns_sig, actual, "dummy_function");
    }

    // -------------------------------------------------------------------------
    // 测试组 4：extractLeafName 边界字符匹配
    // -------------------------------------------------------------------------
    {
        auto actual = reflect::extractLeafName("RolePerms::allowPlace");
        check("extractLeafName(\"RolePerms::allowPlace\")", "", actual, "allowPlace");
    }

    {
        auto actual = reflect::extractLeafName("&RolePerms::allowPlace");
        check("extractLeafName(\"&RolePerms::allowPlace\")", "", actual, "allowPlace");
    }

    {
        auto actual = reflect::extractLeafName("struct RolePerms*");
        check("extractLeafName(\"struct RolePerms*\")", "", actual, "RolePerms");
    }

    return 0;
}