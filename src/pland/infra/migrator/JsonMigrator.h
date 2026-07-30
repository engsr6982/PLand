#pragma once
#include "pland/Global.h"

#include <functional>

#include <ll/api/Expected.h>

#include <mc/deps/core/utility/optional_ref.h>

#include <nlohmann/adl_serializer.hpp>


namespace land ::infra {

enum class MigrateResult : uint8_t {
    Success,                     // 迁移成功
    SkipByNoAvailableMigrate,    // 没有可用的迁移器，跳过
    SkipByCurrentVersionTooHigh, // 当前版本高于目标版本，跳过
};

/**
 * @class BasicJsonMigrator
 * @brief 数据迁移器，用于处理 JSON 数据的跨版本升级。
 *
 * 行为：
 * 1. 版本号 N 代表的是“目标版本”。
 *    即：注册为版本 15 的 Executor，其职责是将数据从“小于 15 的任意状态”转换为“符合版本 15 规范的状态”。
 *
 * 2. 迁移路径是“跳跃式”的。
 *    如果当前数据是 v1，系统中有注册 [15, 26] 的迁移器：
 *    - 第一次迭代：直接寻找 > 1 的最小迁移器，找到 15。执行 v1 -> v15。
 *    - 第二次迭代：寻找 > 15 的最小迁移器，找到 26。执行 v15 -> v26。
 *    - 中间缺失的版本 (2-14, 16-25) 会被自动跳过。
 */
template <typename J>
class BasicJsonMigrator {
public:
    using json_t    = J;
    using version_t = int32_t;

    using Executor = std::function<ll::Expected<>(json_t& data)>;

    BasicJsonMigrator()          = default;
    virtual ~BasicJsonMigrator() = default;

    LD_DISABLE_COPY_AND_MOVE(BasicJsonMigrator);

    /**
     * @brief 注册迁移器
     * @param version 目标版本号 (执行后数据将变为此版本)
     * @param executor 迁移逻辑
     * @param cover 是否覆盖已存在的版本
     */
    inline void registerMigrator(version_t version, Executor executor, bool cover = false) {
        if (cover) {
            mMigrators_.insert_or_assign(version, executor);
        } else if (!mMigrators_.contains(version)) {
            mMigrators_.emplace(version, std::move(executor));
        }
    }

    /**
     * @brief 批量注册范围迁移器（通常用于处理连续的、逻辑相同的中间版本）
     */
    inline void registerRangeMigrator(version_t from, version_t to, Executor executor, bool cover = false) {
        for (version_t v = from; v <= to; ++v) {
            registerMigrator(v, executor, cover);
        }
    }

    [[nodiscard]] inline optional_ref<const Executor> getExecutor(version_t version) const {
        if (auto it = mMigrators_.find(version); it != mMigrators_.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief 核心迁移函数
     * @param data 待迁移的 JSON 对象
     * @param targetVersion 最终要达到的目标版本
     * @param allowVersionGap 是否允许版本断层。
     *        - 若为 true：从 v1 迁移到 v15 时，即使中间没有 2-14 的迁移器也会继续。
     *        - 若为 false：要求每一步版本提升必须连续 (N -> N+1)。
     */
    [[nodiscard]] inline ll::Expected<MigrateResult>
    migrate(json_t& data, version_t targetVersion, bool allowVersionGap = true) const {
        if (mMigrators_.empty()) return MigrateResult::SkipByNoAvailableMigrate;

        // 获取当前版本，如果 JSON 中没有版本字段，默认视为 0
        version_t currentVersion = data.value(kVersionKey, version_t(0));

        // 如果已经达到或超过目标版本，直接返回
        if (currentVersion >= targetVersion) {
            return MigrateResult::SkipByCurrentVersionTooHigh;
        }

        // 循环寻找“下一个可用的迁移器”
        // 使用 std::map::upper_bound(V) 寻找 key > V 的第一个元素
        auto it = mMigrators_.upper_bound(currentVersion);

        while (it != mMigrators_.end() && it->first <= targetVersion) {
            version_t nextRegisteredVersion = it->first;
            auto&     executor              = it->second;

            // 校验版本连续性
            if (!allowVersionGap && nextRegisteredVersion != currentVersion + 1) {
                return ll::makeStringError(
                    fmt::format(
                        "Data migration gap detected: current v{}, next available v{}. But gap is not allowed.",
                        currentVersion,
                        nextRegisteredVersion
                    )
                );
            }

            // 执行具体的迁移函数
            try {
                if (auto res = std::invoke(executor, data); !res) {
                    return ll::makeStringError(res.error().message()); // 迁移器内部返回错误，中止迁移
                }
            } catch (std::exception const& e) {
                return ll::makeStringError(
                    fmt::format("Exception during migration to v{}: {}", nextRegisteredVersion, e.what())
                );
            }

            // 更新当前版本号到数据中
            currentVersion    = nextRegisteredVersion;
            data[kVersionKey] = currentVersion;

            // 继续寻找下一个大于当前版本的迁移器
            it = mMigrators_.upper_bound(currentVersion);
        }

        if (!allowVersionGap && currentVersion < targetVersion) {
            return ll::makeStringError(fmt::format("Migration failed to reach target v{}", targetVersion));
        }

        return MigrateResult::Success;
    }

    [[nodiscard]] inline std::optional<version_t> getMinVersion() const {
        if (mMigrators_.empty()) {
            return std::nullopt;
        }
        return mMigrators_.begin()->first;
    }

    [[nodiscard]] inline std::optional<version_t> getMaxVersion() const {
        if (mMigrators_.empty()) {
            return std::nullopt;
        }
        return mMigrators_.rbegin()->first;
    }

    inline static constexpr std::string_view kVersionKey = "version";

protected:
    struct Route {
        const char* src;
        const char* dst;
    };

    /**
     * @brief 将JSON数据中的路径从src映射到dst
     *
     * @param root JSON数据的引用，将被修改以包含路径映射信息
     * @param src 源路径字符串视图，表示原始路径
     * @param dst 目标路径字符串视图，表示映射后的目标路径
     */
    inline static void mapPath(json_t& root, std::string_view src, std::string_view dst) {
        // 辅助函数：把 "land.bought.xxx" 变成 "/land/bought/xxx"
        auto toPtr = [](std::string_view s) {
            std::string res = "/";
            for (char c : s) {
                res += (c == '.' ? '/' : c);
            }
            return typename json_t::json_pointer(res);
        };

        auto srcPtr = toPtr(src);
        if (root.contains(srcPtr)) {
            root[toPtr(dst)] = std::move(root[srcPtr]);

            // 擦除旧数据的数据痕迹
            auto parentPtr = srcPtr.parent_pointer();
            if (root.contains(parentPtr)) {
                root[parentPtr].erase(srcPtr.back());
            }
        }
    }

    inline static void mapPath(json_t& root, Route const& route) { mapPath(root, route.src, route.dst); }

    std::map<version_t, Executor> mMigrators_;
};

using JsonMigrator = BasicJsonMigrator<nlohmann::json>;

} // namespace land::infra
