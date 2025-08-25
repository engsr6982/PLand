#pragma once


namespace land {


enum class DrawHandleBackend {
    DefaultParticle                 = 0, // 默认粒子系统
    BedrockServerClientInterfaceMod = 1, // BSCI 模组
    MinecraftDebugShape             = 2  // Minecraft 调试形状
};


}