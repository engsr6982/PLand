#include "pland/Calculate.h"


#pragma warning(disable : 4702)
#include "exprtk.hpp"
#pragma warning(default : 4702)

namespace land {

Calculate::Calculate(int height, int width, int depth)
: mHeight((double)height),
  mWidth((double)width),
  mDepth((double)depth) {
    mSquare = mWidth * mDepth;
    mVolume = mHeight * mSquare;
}

int Calculate::eval(string code) {
    // 注册变量
    exprtk::symbol_table<double> vars;
    vars.add_variable("height", mHeight);
    vars.add_variable("width", mWidth);
    vars.add_variable("depth", mDepth);
    vars.add_variable("square", mSquare);
    vars.add_variable("volume", mVolume);

    // 解析表达式
    exprtk::expression<double> expr;
    expr.register_symbol_table(vars);

    // 编译表达式
    exprtk::parser<double> parser;
    if (!parser.compile(code, expr)) {
        return -1;
    }

    return (int)expr.value(); // 计算结果
}


} // namespace land