#include "DebugShape.h"
#include "mc/_HeaderOutputPredefine.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/Dimension.h"

namespace ScriptModuleDebugUtilities {

// LNK2019
PacketShapeData::PacketShapeData()                             = default;
PacketShapeData::PacketShapeData(PacketShapeData const& other) = default;

} // namespace ScriptModuleDebugUtilities


namespace land::debug_shape {


uint64_t DebugShape::getNextId() {
    static uint64_t next = {INT_MAX};
    return next++;
}
ShapePacketData const& DebugShape::getPacketData() const { return mPacketData; }

DebugShape::DebugShape(DebugShapeType type, Vec3 const& loc) {
    mPacketData.mShapeType = type;
    mPacketData.mNetworkId = getNextId();
    mPacketData.mLocation  = loc;
}

uint64_t                  DebugShape::getId() const { return mPacketData.mNetworkId; }
DebugShapeType            DebugShape::getType() const { return mPacketData.mShapeType->value(); }
std::optional<Vec3>       DebugShape::getPosition() const { return mPacketData.mLocation; }
void                      DebugShape::setPosition(Vec3 const& loc) { mPacketData.mLocation = loc; }
std::optional<Vec3>       DebugShape::getRotation() const { return mPacketData.mRotation; }
void                      DebugShape::setRotation(std::optional<Vec3> rot) { mPacketData.mRotation = rot; }
std::optional<float>      DebugShape::getScale() const { return mPacketData.mScale; }
void                      DebugShape::setScale(std::optional<float> s) { mPacketData.mScale = s; }
std::optional<mce::Color> DebugShape::getColor() const { return mPacketData.mColor; }
void                      DebugShape::setColor(std::optional<mce::Color> c) { mPacketData.mColor = c; }
std::optional<float>      DebugShape::getTotalTimeLeft() const { return mPacketData.mTimeLeftTotalSec; }
void                      DebugShape::setTotalTimeLeft(std::optional<float> t) { mPacketData.mTimeLeftTotalSec = t; }
bool                      DebugShape::hasDuration() const { return mPacketData.mTimeLeftTotalSec->value_or(0) > 0; }


DebugSphere::DebugSphere(Vec3 const& loc) : DebugShape(DebugShapeType::Sphere, loc) {}

DebugText::DebugText(Vec3 const& loc, const std::string& text) : DebugShape(DebugShapeType::Text, loc) {
    mPacketData.mText = text;
}
std::optional<std::string> DebugText::getText() const { return mPacketData.mText; }
void                       DebugText::setText(const std::string& txt) { mPacketData.mText = txt; }

DebugCircle::DebugCircle(Vec3 const& center) : DebugShape(DebugShapeType::Circle, center) {}

DebugBox::DebugBox(Vec3 const& loc) : DebugShape(DebugShapeType::Box, loc) {}
std::optional<Vec3> DebugBox::getBound() const { return mPacketData.mBoxBound; }
void                DebugBox::setBound(Vec3 const& bound) { mPacketData.mBoxBound = bound; }

DebugLine::DebugLine(Vec3 const& start, Vec3 const& end) : DebugShape(DebugShapeType::Line, start) {
    mPacketData.mEndLocation = end;
}
std::optional<Vec3> DebugLine::getEndPosition() const { return mPacketData.mEndLocation; }
void                DebugLine::setEndPosition(std::optional<Vec3> loc) { mPacketData.mEndLocation = loc; }

DebugArrow::DebugArrow(Vec3 const& start, Vec3 const& end) : DebugLine(start, end) {
    mPacketData.mShapeType = DebugShapeType::Arrow;
}
std::optional<float>   DebugArrow::getHeadLength() const { return mPacketData.mArrowHeadLength; }
void                   DebugArrow::setHeadLength(std::optional<float> l) { mPacketData.mArrowHeadLength = l; }
std::optional<float>   DebugArrow::getHeadRadius() const { return mPacketData.mArrowHeadRadius; }
void                   DebugArrow::setHeadRadius(std::optional<float> r) { mPacketData.mArrowHeadRadius = r; }
std::optional<uint8_t> DebugArrow::getHeadSegments() const { return mPacketData.mNumSegments; }
void                   DebugArrow::setHeadSegments(std::optional<uint8_t> s) { mPacketData.mNumSegments = s; }


// impl
void DebugShape::draw(int dimensionId) const {
    auto pkt = _buildPacket();
    auto loc = mPacketData.mLocation->value_or(BlockPos{});
    pkt->sendTo(loc, dimensionId);
}
void DebugShape::draw(Player& player) const {
    auto pkt = _buildPacket();
    pkt->sendTo(player);
}

void DebugShape::remove() const {
    ShapePacketData data;
    data.mNetworkId = mPacketData.mNetworkId;

    auto pkt = _buildEmptyPacket();
    pkt->mShapes->push_back(data);
    pkt->sendToClients();
}
void DebugShape::remove(Player& player) const {
    ShapePacketData data;
    data.mNetworkId = mPacketData.mNetworkId;

    auto pkt = _buildEmptyPacket();
    pkt->mShapes->push_back(data);
    pkt->sendTo(player);
}

std::unique_ptr<ShapeDrawerPacket> DebugShape::_buildPacket() const {
    auto pkt = _buildEmptyPacket();
    pkt->mShapes->push_back(getPacketData());
    return std::move(pkt);
}
std::unique_ptr<ShapeDrawerPacket> DebugShape::_buildEmptyPacket() { return std::make_unique<ShapeDrawerPacket>(); }

} // namespace land::debug_shape