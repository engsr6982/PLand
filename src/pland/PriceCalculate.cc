#include "pland/PriceCalculate.h"
#include "pland/math/LandAABB.h"


#pragma warning(disable : 4702)
#include "exprtk.hpp"
#pragma warning(default : 4702)

namespace land {

PriceCalculate::Variable::Variable() = default;
PriceCalculate::Variable::Impl*       PriceCalculate::Variable::operator->() { return &mImpl; }
PriceCalculate::Variable::Impl&       PriceCalculate::Variable::get() { return mImpl; }
PriceCalculate::Variable::Impl const& PriceCalculate::Variable::get() const { return mImpl; }

template <typename T>
decltype(auto) PriceCalculate::Variable::operator[](T&& key) {
    return mImpl[std::forward<T>(key)];
}

PriceCalculate::Variable PriceCalculate::Variable::make(LandAABB const& landPos, int dimensionId) {
    PriceCalculate::Variable result;
    result["height"]      = landPos.getHeight();
    result["width"]       = landPos.getWidth();
    result["depth"]       = landPos.getDepth();
    result["square"]      = landPos.getSquare();
    result["volume"]      = landPos.getVolume();
    result["dimensionId"] = (double)dimensionId; // 添加维度ID
    return result;
}

PriceCalculate::Variable PriceCalculate::Variable::make(int height, int width, int depth, int dimensionId) {
    PriceCalculate::Variable result;
    result["height"]      = height;
    result["width"]       = width;
    result["depth"]       = depth;
    result["square"]      = width * depth;
    result["volume"]      = height * width * depth;
    result["dimensionId"] = (double)dimensionId; // 添加维度ID
    return result;
}


double PriceCalculate::eval(string const& code, Variable const& variables) {
    exprtk::symbol_table<double> symbols;

    for (auto const& [key, value] : variables.get()) {
        auto& t = *const_cast<double*>(&value);
        symbols.add_constant(key, t);
    }

    symbols.add_function("random_num", &internals::random_num);
    symbols.add_function("random_num_range", &internals::random_num_range);

    // 解析表达式
    exprtk::expression<double> expr;
    expr.register_symbol_table(symbols);

    // 编译表达式
    exprtk::parser<double> parser;
    if (!parser.compile(code, expr)) {
        return 0;
    }

    return expr.value(); // 计算结果
}


int PriceCalculate::calculateDiscountPrice(double originalPrice, double discountRate) {
    // discountRate为1时表示原价，为0.9时表示打9折
    return (int)(originalPrice * discountRate);
}

int PriceCalculate::calculateRefundsPrice(double originalPrice, double refundRate) {
    // refundRate为1时表示全额退款，为0.9时表示退还90%
    return (int)(originalPrice * refundRate);
}


namespace internals {

double random_num() {
    static std::random_device               rd;
    static std::mt19937                     gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
}

double random_num_range(double min, double max) {
    static std::random_device        rd;
    static std::mt19937              gen(rd());
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

} // namespace internals


} // namespace land
