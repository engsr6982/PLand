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

    template <typename... Args>
        requires std::constructible_from<LandAABB, Args...>
    static LandAABB make(Args&&... args) {
        auto aabb = LandAABB(std::forward<Args>(args)...);
        aabb.canonicalize();
        return aabb;
    }

    [[deprecated("Use `canonicalize` instead")]] LDAPI void fix(); // fix min/max

    /**
     * Ensures the AABB is in canonical form (min <= max per axis).
     * Does not alter the actual bounding volume.
     */
    LDAPI void canonicalize();

    LDNDAPI LandPos&       getMin();
    LDNDAPI LandPos const& getMin() const;
    LDNDAPI LandPos&       getMax();
    LDNDAPI LandPos const& getMax() const;

    LDNDAPI int getSpanX() const; // X 轴坐标跨度 (depth)
    LDNDAPI int getSpanZ() const; // Z 轴坐标跨度 (width)
    LDNDAPI int getSpanY() const; // Y 轴坐标跨度 (height)

    // AABB 区域内各轴包含的方块数量
    LDAPI int getBlockCountX() const;
    LDAPI int getBlockCountY() const;
    LDAPI int getBlockCountZ() const;

    /**
     * @brief 获取AABB底面积（X-Z平面方块总数）
     * @note 基于 getBlockCountX() * getBlockCountZ()
     */
    LDNDAPI llong getSquare() const;

    /**
     * @brief 获取AABB总体积（总方块数）
     * @note 基于 getBlockCountX() * getBlockCountZ() * getBlockCountY()
     */
    LDNDAPI llong getVolume() const;

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

    LDNDAPI bool hasPos(BlockPos const& pos, bool includeY = true) const;

    /**
     * @brief 扩展 AABB 边界
     */
    LDNDAPI LandAABB expanded(int spacing, bool includeY = true) const;

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

    // 计算两个 AABB 之间的最短距离的平方（避免开根号，高性能）
    LDAPI static long long getDistanceSq(LandAABB const& a, LandAABB const& b, bool includeY);

    /**
     * @brief 判断两个AABB是否满足最小间距要求
     */
    LDNDAPI static bool
    isComplisWithMinSpacing(LandAABB const& pos1, LandAABB const& pos2, int minSpacing, bool includeY = true);

    /**
     * @brief 判断一个 AABB 区域是否完整包含另一个 AABB 区域
     * 如果目标 AABB 在源 AABB 内，则返回 true，否则返回 false
     */
    LDNDAPI static bool isContain(LandAABB const& src, LandAABB const& dst);

    /**
     * @brief 获取两个 AABB 之间的最小间距(x/z 轴)
     * @return 返回实际间距（重叠则为实际重叠部分长度，负数）
     */
    LDNDAPI static int getMinSpacing(LandAABB const& a, LandAABB const& b, bool includeY);
};

STATIC_ASSERT_AGGREGATE(LandAABB);

} // namespace land