#pragma once
#include "pland/Global.h"
#include "pland/enums/LandRole.h"

#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Player;
class BlockPos;
namespace mce {
class UUID;
}

namespace land {

class Land;
class LandContext;
class PLand;

struct PlayerSettings {
    bool showEnterLandTitle{true};     // 是否显示进入领地提示
    bool showBottomContinuedTip{true}; // 是否持续显示底部提示
};

class LandTemplatePermTable;

class LandRegistry final {
    struct Impl;
    std::unique_ptr<Impl> impl;

    friend class TransactionContext;

    LandID _allocateNextId();

public:
    LD_DISABLE_COPY_AND_MOVE(LandRegistry);
    explicit LandRegistry(PLand& mod);
    ~LandRegistry();

    /**
     * 创建数据库快照
     * @param dirName 快照文件夹名称，如果为空，则使用当前时间戳
     * @note 创建的快照会被写入磁盘, SnapshotDir/<dirName.value_or(timestamp)>
     * @note 此任务为异步任务，如果任务未完成，文件夹下会存在 .incomplete 文件
     */
    LDAPI void createSnapshot(std::optional<std::string> const& dirName = std::nullopt);

    LDAPI void save();

    LDAPI bool save(std::shared_ptr<Land> const& land, bool force = false) const;

public:
    LDNDAPI bool isOperator(mce::UUID const& uuid) const;

    LDNDAPI bool addOperator(mce::UUID const& uuid);

    LDNDAPI bool removeOperator(mce::UUID const& uuid);

    LDNDAPI std::vector<mce::UUID> const& getOperators() const;

    LDNDAPI PlayerSettings& getOrCreatePlayerSettings(mce::UUID const& uuid);

    LDNDAPI LandTemplatePermTable& getLandTemplatePermTable() const;

    LDNDAPI bool hasLand(LandID id) const;

    LDAPI void refreshLandRange(std::shared_ptr<Land> const& ptr); // 刷新领地范围

    LDNDAPI ll::Expected<> addOrdinaryLand(std::shared_ptr<Land> const& land);

    LDNDAPI ll::Expected<> removeOrdinaryLand(std::shared_ptr<Land> const& ptr);


    /**
     * 子领地事务执行器
     * @note 在执行前，Registry 会对当前领地数据进行快照，如果任务返回 false，则回滚数据到快照
     * @note 在回调内，禁止再次访问 Registry，这会造成死锁
     */
    using TransactionCallback = std::function<bool(TransactionContext& ctx)>;

    LDNDAPI ll::Expected<> executeTransaction(
        std::unordered_set<std::shared_ptr<Land>> const& participants,
        TransactionCallback const&                       executor
    );


public: // 领地查询API
    LDNDAPI std::shared_ptr<Land> getLand(LandID id) const;

    LDNDAPI std::vector<std::shared_ptr<Land>> getLands() const;

    LDNDAPI std::vector<std::shared_ptr<Land>> getLands(std::vector<LandID> const& ids) const;

    LDNDAPI std::vector<std::shared_ptr<Land>> getLands(LandDimid dimid) const;

    LDNDAPI std::vector<std::shared_ptr<Land>> getLands(mce::UUID const& uuid, bool includeShared = false) const;

    LDNDAPI std::vector<std::shared_ptr<Land>> getLands(mce::UUID const& uuid, LandDimid dimid) const;

    LDNDAPI std::unordered_map<mce::UUID, std::unordered_set<std::shared_ptr<Land>>> getLandsByOwner() const;

    LDNDAPI LandPermType getPermType(mce::UUID const& uuid, LandID id = 0, bool includeOperator = true) const;

    LDNDAPI std::shared_ptr<Land> getLandAt(BlockPos const& pos, LandDimid dimid) const;

    LDNDAPI std::unordered_set<std::shared_ptr<Land>>
            getLandAt(BlockPos const& center, int radius, LandDimid dimid) const;

    LDNDAPI std::unordered_set<std::shared_ptr<Land>>
            getLandAt(BlockPos const& pos1, BlockPos const& pos2, LandDimid dimid) const;

    using CustomFilter = std::function<bool(std::shared_ptr<Land> const&)>;
    LDNDAPI std::vector<std::shared_ptr<Land>> getLandsWhere(CustomFilter const& filter) const;

public:
    static constexpr auto DatabaseDir = "db";        // 数据库目录名
    static constexpr auto SnapshotDir = "snapshots"; // 快照目录名

    static constexpr auto DbVersionKey           = "__version__";     // 数据库版本键
    static constexpr auto DbOperatorDataKey      = "operators";       // 操作员数据键
    static constexpr auto DbPlayerSettingDataKey = "player_settings"; // 玩家设置数据键
    static constexpr auto DbTemplatePermKey      = "template_perm";   // 领地模板权限表数据键
    static bool           isLandData(std::string_view key);           // 判断键是否为领地数据键
};


} // namespace land
