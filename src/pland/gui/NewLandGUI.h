#pragma once
#include "pland/Global.h"

class Player;

namespace land {


/**
 * @brief 新建领地GUI
 */
class NewLandGUI {
public:
    NewLandGUI() = delete;

    // 选择领地维度(3D/2D)
    LDAPI static void sendChooseLandDim(Player& player);

    // 确认选区Y轴范围
    LDAPI static void sendConfirmPrecinctsYRange(Player& player, std::string const& exception = "");
};


} // namespace land