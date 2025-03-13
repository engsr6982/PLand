#include "pland/DataConverter.h"
#include "fmt/ostream.h"
#include "ll/api/service/PlayerInfo.h"
#include "mod/MyMod.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
#include "pland/PLand.h"
#include "pland/utils/JSON.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <fstream>


namespace land {


DataConverter::DataConverter(bool clearDb) : mClearDb(clearDb) {}

std::unique_ptr<nlohmann::json> DataConverter::loadJson(fs::path const& file) const {
    if (!fs::exists(file)) {
        throw std::runtime_error("File does not exist: " + file.string());
    }
    if (file.extension() != ".json") {
        throw std::runtime_error("File is not a JSON file: " + file.string());
    }
    std::ifstream ifs(file);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open file: " + file.string());
    }

    try {
        auto json = std::make_unique<nlohmann::json>();
        ifs >> (*json);
        ifs.close();
        return json;
    } catch (nlohmann::json::parse_error& e) {
        ifs.close();
        throw std::runtime_error("Failed to parse JSON file: " + file.string() + " Error: " + e.what());
    } catch (...) {
        ifs.close();
        throw std::runtime_error("Unknown error occurred while parsing JSON file: " + file.string());
    }
}

void DataConverter::writeToDb(LandData_sptr const& data) {
    auto& db = PLand::getInstance();
    if (mClearDb && !mIsCleanedDb) {
        mIsCleanedDb = true;
        for (auto& land : db.getLands()) {
            db.removeLand(land->getLandID());
        }
    }
    db.addLand(data);
}

void DataConverter::writeToDb(std::vector<LandData_sptr> const& data) {
    for (auto& land : data) {
        writeToDb(land);
    }
}

template <class T>
T DataConverter::reflection(nlohmann::json const& json) const {
    T    obj;
    auto mutableJson = json; // 创建可修改的副本
    JSON::jsonToStructTryPatch(mutableJson, obj);
    return obj;
}

template <typename... Args>
void DataConverter::printProgress(size_t progress, size_t total, fmt::format_string<Args...> fmt, Args&&... args) {
    static constexpr size_t MAX_MESSAGE_LENGTH = 100;
    static constexpr size_t ELLIPSIS_LENGTH    = 3;
    static const char*      bars[]             = {"/", "-", "\\", "|"};
    static size_t           barIndex           = 0;

    std::string message = fmt::vformat(fmt.get(), fmt::make_format_args(args...));
    // 安全的UTF-8字符串截断
    if (message.length() > MAX_MESSAGE_LENGTH) {
        size_t len   = 0;
        size_t bytes = 0;
        while (bytes < message.length()) {
            unsigned char c = message[bytes];
            if ((c & 0x80) == 0) {
                bytes += 1;
            } else if ((c & 0xE0) == 0xC0) {
                bytes += 2;
            } else if ((c & 0xF0) == 0xE0) {
                bytes += 3;
            } else if ((c & 0xF8) == 0xF0) {
                bytes += 4;
            } else {
                // 无效的UTF-8序列
                bytes += 1;
            }

            if (len + 1 > MAX_MESSAGE_LENGTH - ELLIPSIS_LENGTH) {
                break;
            }
            len++;
        }
        message = message.substr(0, bytes) + "...";
    }

    fmt::println("\r{} [{} / {}] {}", bars[barIndex++ % 4], progress, total, message);
    printf("\033[A");
    printf("\033[K");
}


// iLandConverter
iLandConverter::iLandConverter(const string& relationShipPath, const string& dataPath, bool clear_db)
: DataConverter{clear_db},
  mRelationShipPath{relationShipPath},
  mDataPath{dataPath} {}

LandData_sptr iLandConverter::convert(RawData::iLandData const& raw, string const& xuid, std::optional<UUIDs> uuids) {
    LandData_sptr ptr = LandData::make();

    // pos
    {
        auto& rawPosA    = raw.range.start_position;
        auto& rawPosB    = raw.range.end_position;
        ptr->mPos.mMin_A = PosBase{rawPosA[0], rawPosA[1], rawPosA[2]};
        ptr->mPos.mMax_B = PosBase{rawPosB[0], rawPosB[1], rawPosB[2]};
        ptr->mPos.fix();
        ptr->mLandDimid = raw.range.dimid;
        ptr->mIs3DLand  = true; // TODO: 由于iLand没有3D数据，所以这里暂时默认为true
    }

    // base info
    {
        ptr->mIsConvertedLand = true;
        ptr->mOwnerDataIsXUID = !uuids.has_value();
        ptr->mLandOwner       = uuids.value_or(xuid);
        // ptr->mLandMembers = raw.settings.share; // TODO
        ptr->mLandName     = raw.settings.nickname;
        ptr->mLandDescribe = raw.settings.describe;
    }

    // permission
    {
        auto& tab = ptr->mLandPermTable;
        // settings
        tab.allowFarmDecay      = raw.settings.ev_farmland_decay;
        tab.allowExplode        = raw.settings.ev_explode;
        tab.allowPistonPush     = raw.settings.ev_piston_push;
        tab.allowFireSpread     = raw.settings.ev_fire_spread;
        tab.allowRedstoneUpdate = raw.settings.ev_redstone_update;
        // permissions
        auto& p                 = raw.permissions;
        tab.useDispenser        = p.use_dispenser;
        tab.useDoor             = p.use_door;
        tab.allowDropItem       = p.allow_dropitem;
        tab.allowPickupItem     = p.allow_pickupitem;
        tab.allowPlace          = p.allow_place;
        tab.useFenceGate        = p.use_fence_gate;
        tab.usePressurePlate    = p.use_pressure_plate;
        tab.useBlastFurnace     = p.use_blast_furnace;
        tab.useFiregen          = p.use_firegen;
        tab.useCampfire         = p.use_campfire;
        tab.useBarrel           = p.use_barrel;
        tab.useFurnace          = p.use_furnace;
        tab.useStonecutter      = p.use_stonecutter;
        tab.allowThrowPotion    = p.allow_throw_potion;
        tab.useBeacon           = p.use_beacon;
        tab.useDaylightDetector = p.use_daylight_detector;
        tab.allowAttackPlayer   = p.allow_attack_player;
        // tab.allowDestroy        = p.allow_entity_destroy; // allow_destroy
        tab.useLectern          = p.use_lectern;
        tab.allowShoot          = p.allow_shoot;
        tab.useEnchantingTable  = p.use_enchanting_table;
        tab.useFishingHook      = p.use_fishing_hook;
        tab.useAnvil            = p.use_anvil;
        tab.useLever            = p.use_lever;
        tab.useButton           = p.use_button;
        tab.allowAttackMonster  = p.allow_attack_mobs;
        tab.useComposter        = p.use_composter;
        tab.allowRideEntity     = p.allow_ride_entity;
        tab.useSmithingTable    = p.use_smithing_table;
        tab.useNoteBlock        = p.use_noteblock;
        tab.useGrindstone       = p.use_grindstone;
        tab.useBucket           = p.use_bucket;
        tab.allowDestroy        = p.allow_destroy;
        tab.useHopper           = p.use_hopper;
        tab.useSmoker           = p.use_smoker;
        tab.useRespawnAnchor    = p.use_respawn_anchor;
        tab.useJukebox          = p.use_jukebox;
        tab.useShulkerBox       = p.use_shulker_box;
        tab.allowOpenChest      = p.allow_open_chest;
        tab.useBed              = p.use_bed;
        tab.useItemFrame        = p.use_item_frame;
        tab.useBrewingStand     = p.use_brewing_stand;
        tab.useLoom             = p.use_loom;
        tab.useTrapdoor         = p.use_trapdoor;
        tab.useCraftingTable    = p.use_crafting_table;
        tab.useArmorStand       = p.use_armor_stand;
        tab.allowRideTrans      = p.allow_ride_trans;
        tab.useDropper          = p.use_dropper;
        tab.useCauldron         = p.use_cauldron;
        tab.useCartographyTable = p.use_cartography_table;
        tab.useBell             = p.use_bell;
        tab.allowAttackAnimal   = p.allow_attack_animal;
    }

    return ptr;
}

bool iLandConverter::execute() {
    auto rawRelationShipJSON = loadJson(mRelationShipPath);
    auto rawDataJSON         = loadJson(mDataPath);
    if (!rawRelationShipJSON || !rawDataJSON) {
        return false;
    }

    auto& logger = my_mod::MyMod::getInstance().getSelf().getLogger();
    // 反射
    mRelationShip = reflection<RawRelationShip>(*rawRelationShipJSON);
    mData         = reflection<RawData>(*rawDataJSON);
    if (mRelationShip.version != 284 || mData.version != 284) {
        logger.warn(
            "The version of the data file does not match the current version, the conversion may not be accurate"
        );
    }

    auto const& data  = mData.Lands;
    auto&       infos = ll::service::PlayerInfo::getInstance();
    logger.info("Start the data transformation...");

    size_t                     total = 0, progress = 0;
    std::vector<LandData_sptr> result;
    result.reserve(data.size()); // reserve space to avoid reallocation
    for (auto& [xuid, lands] : mRelationShip.Owner) {
        progress  = 0;
        total     = lands.size();
        auto info = infos.fromXuid(xuid);
        auto name = info ? info->name : xuid;

        for (auto& land : lands) {
            printProgress(progress++, total, "Converting player \"{}\"'s land \"{}\"", name, land);

            auto iter = data.find(land);
            if (iter == data.end()) {
                logger.warn("No land '{}' data owned by player '{}' was found", land, name);
                continue;
            }

            // 刷新领地名称
            printProgress(
                progress,
                total,
                "Converting player \"{}\"'s land \"{}\"",
                name,
                iter->second.settings.nickname
            );

            // 转换数据
            LandData_sptr landData;
            if (info.has_value()) landData = convert(iter->second, xuid, info->uuid.asString());
            else landData = convert(iter->second, xuid, std::nullopt);
            if (!landData) {
                logger.warn(
                    "Failed to convert land '{}' data owned by player '{}'",
                    iter->second.settings.nickname,
                    name
                );
                continue;
            }

            result.push_back(landData);
        }
    }

    logger.info("Data transformation completed, total {} lands", result.size());
    writeToDb(result);

    return true;
}


} // namespace land