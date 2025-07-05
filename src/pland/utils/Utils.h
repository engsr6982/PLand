#pragma once
#include <regex>
#include <string>
#include <vector>

namespace land {


template <typename T>
[[nodiscard]] inline bool some(std::vector<T>& vec, T const& val) {
    return std::find(vec.begin(), vec.end(), val) != vec.end();
}


// 是否是数字 (整数或浮点数、正数或负数)
[[nodiscard]] inline bool isNumber(std::string const& s) {
    static const std::regex number_regex("^[-+]?(?:\\d+(?:\\.\\d*)?|\\.\\d+)(?:[eE][-+]?\\d+)?$");
    return std::regex_match(s, number_regex);
}

// 是否是正数 (整数或浮点数)
[[nodiscard]] inline bool isPositiveNumber(std::string const& s) {
    static const std::regex positive_number_regex("^\\+?(?:(?:0|[1-9]\\d*)(?:\\.\\d*)?|\\.\\d+)(?:[eE][-+]?\\d+)?$");
    return std::regex_match(s, positive_number_regex);
}

// 是否是负数 (整数或浮点数)
[[nodiscard]] inline bool isNegativeNumber(std::string const& s) {
    static const std::regex negative_number_regex("^-(?:(?:0|[1-9]\\d*)(?:\\.\\d*)?|\\.\\d+)(?:[eE][-+]?\\d+)?$");
    return std::regex_match(s, negative_number_regex);
}

// 是否是整数 (正整数或负整数)
[[nodiscard]] inline bool isInteger(std::string const& s) {
    static const std::regex integer_regex("^[-+]?[0-9]+$");
    return std::regex_match(s, integer_regex);
}

// 是否是负整数 (整数)
[[nodiscard]] inline bool isNegativeInteger(std::string const& s) {
    static const std::regex negative_integer_regex("^-[0-9]+$");
    return std::regex_match(s, negative_integer_regex);
}

// 是否是正整数 (整数)
[[nodiscard]] inline bool isPositiveInteger(std::string const& s) {
    static const std::regex positive_integer_regex("^\\+?[0-9]+$");
    return std::regex_match(s, positive_integer_regex);
}

// 是否超出 int 范围
[[nodiscard]] inline bool isOutOfRange(std::string const& s) {
    try {
        long long value = std::stoll(s);
        return value < INT_MIN || value > INT_MAX;
    } catch (const std::exception&) {
        return true; // 转换失败,超出范围
    }
}

} // namespace land