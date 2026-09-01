#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/drawer/DrawerType.h"
#include "pland/economy/EconomySystem.h"
#include "pland/enums/LeaseRecycleMode.h"

#include <unordered_set>
#include <vector>


namespace land {


// 禁止创建领地的范围结构体
struct ForbiddenRange {
    LandAABB aabb;        // 领地坐标范围
    int      dimensionId; // 维度ID
};


struct ConfigData {
    inline static constexpr int SchemaVersion = 35;

    int version{SchemaVersion};

    EconomyConfig economy;

    struct SelectionConfig {
        std::string item{"minecraft:stick"}; // 选区使用的物品
        std::string alias{"木棍"};           // 显示在 UI 中的别名
    } selector;

    struct FeaturesConfig {
        bool landTeleport{true}; // 领地传送

        struct DrawConfig {
            bool       enabled{false};                  // 领地绘制
            int        range{64};                       // 绘制范围(以玩家为中心, 半径)
            DrawerType backend{DrawerType::DebugShape}; // 绘制实现

            struct ColorConfig {
                std::string onSelectorConfirm{"#FF00FF"};             // 选区确认后的颜色
                std::string onResizeLandDrawOldRange{"#FF0000"};      // 调整领地大小时绘制旧范围的颜色
                std::string onCreateSubLandDrawParentLand{"#FFFF00"}; // 创建子领地时绘制父领地的颜色
                std::string onUseCommandDrawCurrentLand{"#00FF00"};   // 使用命令时绘制当前领地的颜色
                std::string onUseCommandDrawNearLand{"#00FFFF"};      // 使用命令时绘制附近领地的颜色
            } color;
        } draw;

        struct NotificationsConfig {
            bool enterLandTip{true};        // 进入领地提示
            bool bottomContinuousTip{true}; // 底部持续提示
            int  bottomTipCycle{1};         // 底部提示刷新周期(秒)
        } notifications;

    } features;

    /**
     * 全局约束配置
     * - 此配置影响所有玩家、领地
     */
    struct ConstraintsConfig {
        int maxLandsPerPlayer{20}; // 每个玩家最大领地数量

        struct NameRule {
            size_t minLen{1};           // 领地名最小长度
            size_t maxLen{32};          // 领地名最大长度
            bool   allowNewline{false}; // 是否允许换行符
        } nameRule;

        struct SizeConfig {
            int minSideLength{4};     // 最小领地边长(X或Z轴的最小跨度)
            int maxSideLength{60000}; // 最大领地边长 (X或Z轴的最大跨度)
            int minHeight{1};         // 最小领地高度 (Y轴跨度)
        } size;

        struct SpacingConfig {
            int  minDistance{16}; // 独立领地之间的最小间距
            bool includeY{true};  // 间距计算是否包含 Y 轴
        } spacing;

        std::vector<ForbiddenRange> forbiddenRanges; // 禁止创建领地的范围

        std::vector<ForbiddenRange> leaseOnlyRanges; // 仅允许租赁的领地范围

    } constraints;

    struct BusinessConfig {

        /**
         * 维度价格倍率
         * - key: 维度 ID value: 倍率
         * - 默认倍率为 1.0，即 1 乘以任何数都等于它本身
         * - 在价格计算时，PLand 会将上一步的价格乘以这个倍率交给下一步继续计算
         */
        std::unordered_map<std::string, double> dimensionalPriceMultiplier;

        /**
         * 折扣率
         * @note 此配置将在价格计算的最后一个环节对价格进行相乘
         * @note 有效范围为 0.01 ~ 1.0
         */
        double discountRate{1};

        using DimensionWhitelist = std::vector<LandDimid>;

        /**
         * 空间形态计费配置 (用于 3D立体 / 2D平面)
         */
        struct PricingModeConfig {
            bool        enabled; // 是否允许创建该形态的领地
            std::string formula; // 对应的价格/租金计算公式
        };

        struct BoughtConfig {
            // bool enabled{false}; // 是否启用购买功能 // todo: impl

            /**
             * 退款率
             * @note 有效范围为 0.01 ~ 1.0
             * @note 退款率越高，玩家退款金额越多
             */
            double refundRate{0.9};

            DimensionWhitelist allowDimensions{0, 1, 2}; // 允许购买的土地维度列表

            PricingModeConfig mode3D{true, "square * 8 + height * 20"}; // 3D 立体领地购买配置
            PricingModeConfig mode2D{true, "square * 25"};              // 2D 平面领地购买配置

            struct SubLandConfig {
                bool        enabled{false};                      // 是否启用子领地特性
                int         maxNestedDepth{6};                   // 子领地最大嵌套深度(最大16)
                int         maxSubLandsPerLand{6};               // 每个领地最多可创建多少个子领地
                int         minSpacing{8};                       // 子领地之间的最小间距
                bool        minSpacingIncludeY{true};            // 子领地之间的最小间距是否包含Y轴
                std::string formula{"square * 8 + height * 20"}; // 价格公式
            } subLand;

        } bought;

        struct LeasingConfig {
            bool               enabled{false};           // 是否启用租赁功能
            DimensionWhitelist allowDimensions{0, 1, 2}; // 允许租赁的土地维度列表

            PricingModeConfig mode3D{true, "(square * 2 + height * 5)"}; // 3D 立体领地租金配置
            PricingModeConfig mode2D{true, "(square * 8)"};              // 2D 平面领地租金配置

            struct DurationConfig {
                int minPeriod{7};      // 最小租期（天）
                int maxPeriod{30};     // 最大租期（天）
                int renewalAdvance{3}; // 续约提前天数
            } duration;

            struct FreezeConfig {
                int    days{7};                 // 冻结持续天数
                double penaltyRatePerDay{0.05}; // 每日罚金比例（5%）
            } freeze;

            struct RecycleConfig {
                LeaseRecycleMode mode{LeaseRecycleMode::TransferToSystem}; // 回收模式
                bool             keepMembers{false};                       // 是否保留领地成员
            } recycle;

            struct NotificationsConfig {
                bool loginTip{true}; // 登录时是否显示提示
                bool enterTip{true}; // 进入土地时是否显示提示
            } notifications;

        } leasing;

    } business;

    struct SystemConfig {
        bool telemetry{true}; // 是否启用遥测
        bool devTools{false}; // 开发者工具
    } system;
};


struct ConfigProvider {
    ConfigProvider() = delete;

    using Impl = ConfigData;
    LDAPI static Impl cfg;

    inline static constexpr std::string_view FILE_NAME = "Config.json";

    LDNDAPI static std::filesystem::path _filePath(const std::filesystem::path& baseDir);

    LDNDAPI static ll::Expected<> load(const std::filesystem::path& baseDir);

    LDNDAPI static ll::Expected<> save(const std::filesystem::path& baseDir);

    // inline API

    inline static auto& getDrawConfig() { return cfg.features.draw; }

    inline static bool isTelemetryEnabled() { return cfg.system.telemetry; }

    inline static bool isDevToolsEnabled() { return cfg.system.devTools; }

    inline static bool isLeasingModelEnabled() { return cfg.business.leasing.enabled; }

    inline static bool isLeasingDimensionAllowed(int dimensionId) {
        auto& allowed = cfg.business.leasing.allowDimensions;
        return std::find(allowed.begin(), allowed.end(), dimensionId) != allowed.end();
    }

    inline static bool isLandTeleportEnabled() { return cfg.features.landTeleport; }

    inline static bool isEconomySystemEnabled() { return cfg.economy.enabled; }

    inline static auto& getLeasingConfig() { return cfg.business.leasing; }

    inline static auto& getSelectionConfig() { return cfg.selector; }

    inline static auto& getSubLandConfig() { return cfg.business.bought.subLand; }

    inline static auto& getNotificationsConfig() { return cfg.features.notifications; }

    inline static auto& getConstraintsConfig() { return cfg.constraints; }

    inline static auto& getBoughtConfig() { return cfg.business.bought; }

    inline static std::string const& getLeasingFormula(bool is3D) {
        return is3D ? cfg.business.leasing.mode3D.formula : cfg.business.leasing.mode2D.formula;
    }

    inline static double getDiscountRate() { return cfg.business.discountRate; }

    inline static std::optional<double> getDimensionPriceMultiplier(LandDimid dimid) {
        auto& multipliers = cfg.business.dimensionalPriceMultiplier;
        if (auto it = multipliers.find(std::to_string(dimid)); it != multipliers.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    inline static bool isDimensionAllowedForPurchase(LandDimid dimensionId) {
        auto& allowed = cfg.business.bought.allowDimensions;
        return std::find(allowed.begin(), allowed.end(), dimensionId) != allowed.end();
    }
    inline static bool isPurchaseModeEnabled(bool is3D) {
        return is3D ? cfg.business.bought.mode3D.enabled : cfg.business.bought.mode2D.enabled;
    }
    inline static std::string const& getPurchasePriceFormula(bool is3D) {
        return is3D ? cfg.business.bought.mode3D.formula : cfg.business.bought.mode2D.formula;
    }
    inline static bool               isSubLandEnabled() { return cfg.business.bought.subLand.enabled; }
    inline static std::string const& getSubLandPriceFormula() { return cfg.business.bought.subLand.formula; }

    inline static bool isLeasingEnabled() { return cfg.business.leasing.enabled; }
    inline static bool isDimensionAllowedForLeasing(LandDimid dimensionId) {
        auto& allowed = cfg.business.leasing.allowDimensions;
        return std::find(allowed.begin(), allowed.end(), dimensionId) != allowed.end();
    }
};

struct [[deprecated("Use ConfigProvider instead")]] Config : ConfigProvider {
    [[deprecated("Use `ConfigProvider::isDimensionAllowedForPurchase` instead")]]
    LDAPI static bool ensureDimensionAllowed(int dimensionId);

    [[deprecated("Use `ConfigProvider::isSubLandEnabled` instead")]]
    LDAPI static bool ensureSubLandFeatureEnabled();

    [[deprecated("Use `ConfigProvider::isPurchaseModeEnabled` instead")]]
    LDAPI static bool ensureOrdinaryLandEnabled(bool is3D);

    [[deprecated("Use `ConfigProvider::isLeasingEnabled` (or `isLeasingModelEnabled`) instead")]]
    LDAPI static bool ensureLeasingEnabled();

    [[deprecated("Use `ConfigProvider::isDimensionAllowedForLeasing` instead")]]
    LDAPI static bool ensureLeasingDimensionAllowed(int dimensionId);

    [[deprecated("Use `ConfigProvider::getPurchasePriceFormula` instead")]]
    LDAPI static std::string const& getLandPriceCalculateFormula(bool is3D);

    [[deprecated("Use `ConfigProvider::getSubLandPriceFormula` instead")]]
    LDAPI static std::string const& getSubLandPriceCalculateFormula();
};

} // namespace land
