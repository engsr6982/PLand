#pragma once
#include "nlohmann/json_fwd.hpp"
#include "pland/Global.h"
#include "pland/LandData.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>


namespace land {


class DataConverter {
    bool const mClearDb; // 是否清空数据库
    bool       mIsCleanedDb = false;

public:
    DataConverter()                                = delete;
    DataConverter(DataConverter&&)                 = delete;
    DataConverter(DataConverter const&)            = delete;
    DataConverter& operator=(DataConverter const&) = delete;
    DataConverter& operator=(DataConverter&&)      = delete;

    explicit DataConverter(bool clearDb = false);
    virtual ~DataConverter() = default;

    std::unique_ptr<nlohmann::json> loadJson(fs::path const& file) const;

    void writeToDb(LandData_sptr const& data);

    void writeToDb(std::vector<LandData_sptr> const& data);

    template <class T>
    T reflection(nlohmann::json const& json) const;

    template <typename... Args>
    void printProgress(size_t progress, size_t total, fmt::format_string<Args...> fmt, Args&&... args);

    virtual bool execute() = 0;
};


class iLandConverter : public DataConverter {
public:
    // relationship.json
    struct RawRelationShip {
        int                                             version;
        std::unordered_map<string, std::vector<string>> Owner; // key: xuid value: landid[]
    };

    // data.json
    struct RawData {
        struct iLandData {
            struct {
                std::vector<int> start_position;
                std::vector<int> end_position;
                int              dimid;
            } range;

            struct {
                string              nickname;
                std::vector<int>    teleport;
                bool                signtother;
                bool                ev_farmland_decay;
                string              describe;
                bool                signbuttom;
                bool                ev_explode;
                bool                signtome;
                bool                ev_piston_push;
                bool                ev_fire_spread;
                std::vector<string> share;
                bool                ev_redstone_update;
            } settings;

            struct {
                bool use_dispenser;
                bool use_door;
                bool allow_dropitem;
                bool allow_pickupitem;
                bool allow_place;
                bool use_fence_gate;
                bool use_pressure_plate;
                bool use_blast_furnace;
                bool use_firegen;
                bool use_campfire;
                bool use_barrel;
                bool use_furnace;
                bool use_stonecutter; // x
                bool allow_throw_potion;
                bool use_beacon;
                bool use_daylight_detector;
                bool allow_attack_player;
                bool allow_entity_destroy;
                bool use_lectern;
                bool allow_shoot; // x
                bool use_enchanting_table;
                bool use_fishing_hook;
                bool use_anvil;
                bool use_lever;
                bool use_button;
                bool allow_attack_mobs;
                bool eat; // x
                bool use_composter;
                bool allow_ride_entity;
                bool use_smithing_table;
                bool use_noteblock;
                bool use_grindstone;
                bool use_bucket;
                bool allow_destroy;
                bool use_hopper;
                bool use_smoker;
                bool use_respawn_anchor;
                bool use_jukebox;
                bool use_shulker_box;
                bool allow_open_chest;
                bool use_bed;
                bool use_item_frame;
                bool use_brewing_stand;
                bool use_loom;
                bool use_trapdoor;
                bool use_crafting_table;
                bool use_armor_stand;
                bool allow_ride_trans;
                bool use_dropper;
                bool use_cauldron;
                bool use_cartography_table;
                bool use_bell;
                bool allow_attack_animal;
            } permissions;
        };

    public:
        int                                   version;
        std::unordered_map<string, iLandData> Lands; // key: landid value: iLandData
    };

public:
    string mRelationShipPath;
    string mDataPath;

    RawRelationShip mRelationShip;
    RawData         mData;

public:
    explicit iLandConverter(const string& relationShipPath, const string& dataPath, bool clear_db = false);

    LandData_sptr convert(RawData::iLandData const& raw, string const& xuid, std::optional<UUIDs> uuids);

    bool execute() override;
};


} // namespace land
