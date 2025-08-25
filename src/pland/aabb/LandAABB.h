#pragma once
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/aabb/LandPos.h"

namespace land {


/**
 * @brief 领地坐标范围
 */
class LandAABB {
public:
    LandPos min{}, max{};

    LDNDAPI static LandAABB make(BlockPos const& min, BlockPos const& max);

    LDAPI void fix(); // fix min/max

    LDNDAPI LandPos const& getMin() const;
    LDNDAPI LandPos const& getMax() const;

    /**
     * @brief 获取AABB体积深度(X轴)
     */
    LDNDAPI int getDepth() const;

    /**
     * @brief 获取AABB高度(Y轴)
     */
    LDNDAPI int getHeight() const;

    /**
     * @brief 获取AABB宽度(Z轴)
     */
    LDNDAPI int   getWidth() const;
    LDNDAPI llong getSquare() const; // (底面积) X * Z
    LDNDAPI llong getVolume() const; // (总体积) Z * X * Y

    LDNDAPI std::string toString() const;

    /**
     * @brief 获取 AABB 范围内的所有区块坐标
     */
    LDNDAPI std::unordered_set<ChunkPos> getChunks() const;

    /**
     * @brief 获取 AABB 区域的边框（立方体框）
     *
     * 等价于 Minecraft 中选区的“六面边线”，不包含内部内容，仅取 AABB 表面的边沿点。
     * 用于可视化选区或生成粒子框架。
     *
     * 示例（立方体视图）：
     *      y ↑
     *        |   _________
     *        |  /       /|
     *        | /_______/ |
     *        | |       | |
     *        | |_______|/      → x
     *       /
     *      z
     *
     * @return std::vector<BlockPos> 表示该立方体所有边线坐标点
     */
    LDNDAPI std::vector<BlockPos> getBorder() const;

    /**
     * @brief 获取 AABB 区域的水平平面边框（矩形框）
     *
     * 获取 AABB 的底面边框，仅包含四条边缘，不包括内部块坐标。
     * 用于 2D 可视化区域。
     *
     * 示例（俯视视图）：
     *
     *      z ↑
     *        │   ┌────────────┐
     *        │   │            │
     *        │   └────────────┘
     *        └────────────────────→ x
     *
     * @return std::vector<BlockPos> 表示底面矩形框的边线坐标
     */
    LDNDAPI std::vector<BlockPos> getRange() const;

    /**
     * @brief 获取 AABB 区域的顶点坐标 (4个角点，平面)
     */
    LDNDAPI std::array<Vec3, 4> getVertices() const;

    /**
     * @brief 获取 AABB 区域的顶点坐标 (8个角点，立方体)
     */
    LDNDAPI std::array<Vec3, 8> getCorners() const;

    /**
     * @brief 获取立方体 Box 的 12 条边线(每条边线有两个点)
     */
    LDNDAPI std::vector<std::pair<BlockPos, BlockPos>> getEdges() const;

    LDNDAPI bool hasPos(BlockPos const& pos, bool ignoreY = false) const;

    /**
     * @brief 扩展 AABB 边界
     */
    LDNDAPI LandAABB expanded(int spacing, bool ignoreY = false) const;

    /**
     * @brief 判断某个pos是否在领地内边界
     */
    LDNDAPI bool isOnInnerBoundary(BlockPos const& pos) const;

    /**
     * @brief 判断某个pos是否在领地外边界
     */
    LDNDAPI bool isOnOuterBoundary(BlockPos const& pos) const;

    /**
     * @brief 检查位置是否在领地上方（x/z 在领地范围内，且 y > max.y）
     */
    LDNDAPI bool isAboveLand(BlockPos const& pos) const;

    LDAPI bool operator==(LandAABB const& pos) const;

    /**
     * @brief 判断两个 AABB 是否有重叠部分
     */
    LDNDAPI static bool isCollision(LandAABB const& pos1, LandAABB const& pos2);

    /**
     * @brief 判断两个AABB是否满足最小间距要求
     */
    LDNDAPI static bool isComplisWithMinSpacing(LandAABB const& pos1, LandAABB const& pos2, int minSpacing);

    /**
     * @brief 判断一个 AABB 区域是否完整包含另一个 AABB 区域
     * 如果目标 AABB 在源 AABB 内，则返回 true，否则返回 false
     */
    LDNDAPI static bool isContain(LandAABB const& src, LandAABB const& dst);

    /**
     * @brief 获取两个 AABB 之间的最小间距(x/z 轴)
     * @return 返回实际间距（重叠则为实际重叠部分长度，负数）
     */
    LDNDAPI static int getMinSpacing(LandAABB const& a, LandAABB const& b);
};

STATIC_ASSERT_AGGREGATE(LandAABB);

} // namespace land