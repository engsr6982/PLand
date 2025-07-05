#pragma once
#include "ll/api/data/KeyValueDB.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


class Player;
class BlockPos;

namespace land {

struct PlayerSettings {
    bool        showEnterLandTitle{true};     // 是否显示进入领地提示
    bool        showBottomContinuedTip{true}; // 是否持续显示底部提示
    std::string localeCode{"server"};         // 语言 system / server / xxx


    LDNDAPI static std::string SYSTEM_LOCALE_CODE();
    LDNDAPI static std::string SERVER_LOCALE_CODE();
};


class PLand final {
public:
    PLand()                        = default;
    PLand(const PLand&)            = delete;
    PLand& operator=(const PLand&) = delete;
    PLand(PLand&&)                 = delete;
    PLand& operator=(PLand&&)      = delete;

private:
    std::unique_ptr<ll::data::KeyValueDB>     mDB;                    // 领地数据库
    std::vector<UUIDs>                        mLandOperators;         // 领地操作员
    std::unordered_map<UUIDs, PlayerSettings> mPlayerSettings;        // 玩家设置
    std::atomic<LandID>                       mNextLandID{0};         // 下一个领地ID
    std::unordered_map<LandID, LandData_sptr> mLandCache;             // 领地缓存
    mutable std::shared_mutex                 mMutex;                 // 读写锁
    std::thread                               mThread;                // 线程
    std::atomic<bool>                         mThreadStopFlag{false}; // 线程停止标志

    // 维度 -> 区块 -> [领地] (快速查询领地)
    std::unordered_map<LandDimid, std::unordered_map<ChunkID, std::unordered_set<LandID>>> mLandMap;


private: //! private 方法非线程安全
    void _loadOperators();
    void _loadPlayerSettings();
    void _loadLands();

    void _openDBAndCheckVersion();
    void _checkAndAdaptBreakingChanges(nlohmann::json& landData); // 检查数据并尝试适配版本不兼容的变更

    void _initLandMap();

    void _updateLandMap(LandData_sptr const& ptr, bool add);
    void _refreshLandRange(LandData_sptr const& ptr);

    LandID getNextLandID();

    Result<bool> _removeLand(LandData_sptr const& ptr);

public:
    LDNDAPI static PLand& getInstance();

    LDAPI void init();
    LDAPI void save();
    LDAPI void stopThread();

public:
    LDNDAPI bool isOperator(UUIDs const& uuid) const;

    LDNDAPI bool addOperator(UUIDs const& uuid);

    LDNDAPI bool removeOperator(UUIDs const& uuid);

    LDNDAPI std::vector<UUIDs> const& getOperators() const;

    LDNDAPI bool hasPlayerSettings(UUIDs const& uuid) const;

    LDNDAPI PlayerSettings* getPlayerSettings(UUIDs const& uuid);

    LDAPI bool setPlayerSettings(UUIDs const& uuid, PlayerSettings settings);

    LDNDAPI bool hasLand(LandID id) const;

    LDAPI Result<bool> addLand(LandData_sptr land);

    LDAPI void refreshLandRange(LandData_sptr const& ptr); // 刷新领地范围 (_refreshLandRange)

    /**
     * @brief 移除领地
     * @deprecated 此接口已废弃，此接口实际调用 removeOrdinaryLand()，请使用 removeOrdinaryLand() 代替
     */
    [[deprecated("Please use removeOrdinaryLand() instead")]] LDAPI bool removeLand(LandID id);

    /**
     * @brief 移除普通领地
     */
    LDNDAPI Result<bool> removeOrdinaryLand(LandData_sptr const& ptr);

    /**
     * @brief 移除子领地
     */
    LDNDAPI Result<bool> removeSubLand(LandData_sptr const& ptr);

    /**
     * @brief 移除领地和其子领地
     */
    LDNDAPI Result<bool> removeLandAndSubLands(LandData_sptr const& ptr);

    /**
     * @brief 移除当前领地并提升子领地为普通领地
     */
    LDNDAPI Result<bool> removeLandAndPromoteSubLands(LandData_sptr const& ptr);

    /**
     * @brief 移除当前领地并移交子领地给当前领地的父领地
     */
    LDNDAPI Result<bool> removeLandAndTransferSubLands(LandData_sptr const& ptr);


public: // 领地查询API
    LDNDAPI LandData_wptr getLandWeakPtr(LandID id) const;
    LDNDAPI LandData_sptr getLand(LandID id) const;
    LDNDAPI std::vector<LandData_sptr> getLands() const;
    LDNDAPI std::vector<LandData_sptr> getLands(std::vector<LandID> const& ids) const;
    LDNDAPI std::vector<LandData_sptr> getLands(LandDimid dimid) const;
    LDNDAPI std::vector<LandData_sptr> getLands(UUIDs const& uuid, bool includeShared = false) const;
    LDNDAPI std::vector<LandData_sptr> getLands(UUIDs const& uuid, LandDimid dimid) const;
    LDNDAPI std::unordered_map<UUIDs, std::unordered_set<LandData_sptr>> getLandsByOwner() const;
    LDNDAPI std::unordered_map<UUIDs, std::unordered_set<LandData_sptr>> getLandsByOwner(LandDimid dimid) const;

    LDNDAPI LandPermType getPermType(UUIDs const& uuid, LandID id = 0, bool ignoreOperator = false) const;

    LDNDAPI LandData_sptr getLandAt(BlockPos const& pos, LandDimid dimid) const;

    LDNDAPI std::unordered_set<LandData_sptr> getLandAt(BlockPos const& center, int radius, LandDimid dimid) const;

    LDNDAPI std::unordered_set<LandData_sptr>
            getLandAt(BlockPos const& pos1, BlockPos const& pos2, LandDimid dimid) const;

public:
    LDAPI static ChunkID             EncodeChunkID(int x, int z);
    LDAPI static std::pair<int, int> DecodeChunkID(ChunkID id);

    static string DB_DIR_NAME();
    static string DB_KEY_OPERATORS();
    static string DB_KEY_PLAYER_SETTINGS();
    static string DB_KEY_VERSION();
};


} // namespace land
