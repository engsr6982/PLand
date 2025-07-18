#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/land/LandPerm.h"
#include <vector>


namespace land {


// ! 注意：如果 LandContext 有更改，则必须递增 LandContextVersion，否则导致加载异常
constexpr int LandContextVersion = 20;
struct LandContext {
    int                 version{LandContextVersion};           // 版本号
    LandAABB            mPos;                                  // 领地对角坐标
    LandPos             mTeleportPos;                          // 领地传送坐标
    LandID              mLandID{LandID(-1)};                   // 领地唯一ID  (由 LandRegistry::addLand() 时分配)
    LandDimid           mLandDimid;                            // 领地所在维度
    bool                mIs3DLand;                             // 是否为3D领地
    LandPermTable       mLandPermTable;                        // 领地权限
    UUIDs               mLandOwner;                            // 领地主人(默认UUID,其余情况看mOwnerDataIsXUID)
    std::vector<UUIDs>  mLandMembers;                          // 领地成员
    std::string         mLandName{"Unnamed territories"_tr()}; // 领地名称
    std::string         mLandDescribe{"No description"_tr()};  // 领地描述
    int                 mOriginalBuyPrice{0};                  // 原始购买价格
    bool                mIsConvertedLand{false};               // 是否为转换后的领地(其它插件创建的领地)
    bool                mOwnerDataIsXUID{false};   // 领地主人数据是否为XUID (如果为true，则主人上线自动转换为UUID)
    LandID              mParentLandID{LandID(-1)}; // 父领地ID
    std::vector<LandID> mSubLandIDs;               // 子领地ID
};


} // namespace land
