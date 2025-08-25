#pragma once
#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/network/packet/PacketShapeData.h"
#include "mc/network/packet/ServerScriptDebugDrawerPacket.h"
#include "mc/scripting/modules/minecraft/debugdrawer/ScriptDebugShapeType.h"
#include "pland/Global.h"
#include <optional>

namespace land::debug_shape {

using DebugShapeType    = ScriptModuleDebugUtilities::ScriptDebugShapeType;
using ShapePacketData   = ScriptModuleDebugUtilities::PacketShapeData;
using ShapeDrawerPacket = ScriptModuleDebugUtilities::ServerScriptDebugDrawerPacket;

/**
 * 调试形状
 * @see
 * https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/debug-utilities/debugshape?view=minecraft-bedrock-experimental
 */
class DebugShape {
protected:
    ShapePacketData mPacketData{};

private:
    ShapePacketData const& getPacketData() const;
    friend class DebugShapeDrawer;

    static inline uint64_t getNextId();

public:
    LDAPI explicit DebugShape(DebugShapeType type, Vec3 const& loc);
    virtual ~DebugShape() = default;

    LDNDAPI uint64_t getId() const; // 唯一id

    LDNDAPI DebugShapeType getType() const; // 形状类型

    LDNDAPI std::optional<Vec3> getPosition() const; // 位置
    LDAPI void                  setPosition(Vec3 const& loc);

    LDNDAPI std::optional<Vec3> getRotation() const; // 旋转
    LDAPI void                  setRotation(std::optional<Vec3> rot);

    LDNDAPI std::optional<float> getScale() const; // 缩放
    LDAPI void                   setScale(std::optional<float> s);

    LDNDAPI std::optional<mce::Color> getColor() const; // 颜色
    LDAPI void                        setColor(std::optional<mce::Color> c);

    LDNDAPI bool hasDuration() const;                      // 是否有持续时间
    LDNDAPI std::optional<float> getTotalTimeLeft() const; // 剩余时间
    LDAPI void                   setTotalTimeLeft(std::optional<float> t);

    LDAPI virtual void draw(int dimensionId) const;
    LDAPI virtual void draw(Player& player) const;

    LDAPI virtual void remove() const;
    LDAPI virtual void remove(Player& player) const;

    LDNDAPI virtual std::unique_ptr<ShapeDrawerPacket> _buildPacket() const;
    LDAPI static std::unique_ptr<ShapeDrawerPacket>    _buildEmptyPacket();
};


/**
 * 调试球
 * @see
 * https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/debug-utilities/debugsphere?view=minecraft-bedrock-experimental
 */
class DebugSphere : public DebugShape {
public:
    LDAPI explicit DebugSphere(Vec3 const& loc);
};

/**
 * 调试文本
 * @see
 * https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/debug-utilities/debugtext?view=minecraft-bedrock-experimental
 */
class DebugText : public DebugShape {
public:
    LDAPI explicit DebugText(Vec3 const& loc, const std::string& text);

    LDNDAPI std::optional<std::string> getText() const;

    LDAPI void setText(const std::string& txt);
};

/**
 * 调试圈
 * @see
 * https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/debug-utilities/debugcircle?view=minecraft-bedrock-experimental
 */
class DebugCircle : public DebugShape {
public:
    LDAPI explicit DebugCircle(Vec3 const& center);
};

/**
 * 调试立方体
 * @see
 * https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/debug-utilities/debugbox?view=minecraft-bedrock-experimental
 */
class DebugBox : public DebugShape {
public:
    LDAPI explicit DebugBox(Vec3 const& loc);

    LDNDAPI std::optional<Vec3> getBound() const; // 获取边界

    LDAPI void setBound(Vec3 const& bound);
};

/**
 * 调试线段
 * @see
 * https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/debug-utilities/debugline?view=minecraft-bedrock-experimental
 */
class DebugLine : public DebugShape {
public:
    LDAPI explicit DebugLine(Vec3 const& start, Vec3 const& end);

    LDNDAPI std::optional<Vec3> getEndPosition() const;

    LDAPI void setEndPosition(std::optional<Vec3> loc);
};

/**
 * 调试箭头
 * @see
 * https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/debug-utilities/debugarrow?view=minecraft-bedrock-experimental
 */
class DebugArrow : public DebugLine {
public:
    LDAPI explicit DebugArrow(Vec3 const& start, Vec3 const& end);

    LDNDAPI std::optional<float> getHeadLength() const; // 箭头长度

    LDAPI void setHeadLength(std::optional<float> l);

    LDNDAPI std::optional<float> getHeadRadius() const; // 箭头半径

    LDAPI void setHeadRadius(std::optional<float> r);

    LDNDAPI std::optional<uint8_t> getHeadSegments() const; // 箭头分段数

    LDAPI void setHeadSegments(std::optional<uint8_t> s);
};


} // namespace land::debug_shape
