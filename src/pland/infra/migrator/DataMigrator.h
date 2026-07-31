#pragma once
#include "pland/Global.h"

#include <ll/api/Expected.h>

namespace land ::infra {

enum class MigrateResult : uint8_t {
    Success,                     // 迁移成功
    SkipByNoAvailableMigrate,    // 没有可用的迁移器，跳过
    SkipByCurrentVersionTooHigh, // 当前版本高于目标版本，跳过
};

template <typename Accessor, typename Container, typename VersionT>
concept VersionAccessor = requires(Container& container, VersionT version) {
    { Accessor::getVersion(container) } -> std::same_as<VersionT>;
    { Accessor::setVersion(container, version) } -> std::same_as<void>;
};

/**
 * @class DataMigrator
 * @brief 数据迁移器，用于处理任意类型数据的向后兼容。
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
 *
 * @tparam Container 容器类型，例如 JSON
 * @tparam Accessor 版本访问器, 提供当前数据的版本号
 * @tparam VersionT 版本号数据类型，通常为 int
 */
template <typename Container, typename Accessor, typename VersionT = int32_t>
    requires VersionAccessor<Accessor, Container, VersionT>
class DataMigrator {
public:
    using container_t = Container;
    using version_t   = VersionT;
    using accessor_t  = Accessor;

    using MigrationUnit = bool (*)(container_t& data);

    DataMigrator()          = default;
    virtual ~DataMigrator() = default;

    LD_DISABLE_COPY_AND_MOVE(DataMigrator);

    /**
     * @brief 注册迁移器
     * @param version 目标版本号 (执行后数据将变为此版本)
     * @param executor 迁移逻辑
     * @param cover 是否覆盖已存在的版本
     */
    inline void registerMigrationUnit(version_t version, MigrationUnit executor, bool cover = false) {
        if (cover) {
            mMigrators.insert_or_assign(version, executor);
        } else if (!mMigrators.contains(version)) {
            mMigrators.emplace(version, std::move(executor));
        }
    }

    /**
     * @brief 批量注册范围迁移器（通常用于处理连续的、逻辑相同的中间版本）
     */
    inline void registerRangeMigrator(version_t from, version_t to, MigrationUnit executor, bool cover = false) {
        for (version_t v = from; v <= to; ++v) {
            registerMigrationUnit(v, executor, cover);
        }
    }

    [[nodiscard]] inline MigrationUnit getExecutor(version_t version) const {
        if (auto it = mMigrators.find(version); it != mMigrators.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief 核心迁移函数
     * @param data 待迁移的数据对象
     * @param targetVersion 最终要达到的目标版本
     * @param allowVersionGap 是否允许版本断层。
     *        - 若为 true：从 v1 迁移到 v15 时，即使中间没有 2-14 的迁移器也会继续。
     *        - 若为 false：要求每一步版本提升必须连续 (N -> N+1)。
     */
    [[nodiscard]] inline ll::Expected<MigrateResult>
    migrate(container_t& data, version_t targetVersion, bool allowVersionGap = true) const {
        if (mMigrators.empty()) return MigrateResult::SkipByNoAvailableMigrate;

        // 获取当前版本
        version_t currentVersion = Accessor::getVersion(data);

        // 如果已经达到或超过目标版本，直接返回
        if (currentVersion >= targetVersion) {
            return MigrateResult::SkipByCurrentVersionTooHigh;
        }

        // 循环寻找“下一个可用的迁移器”
        // 使用 std::map::upper_bound(V) 寻找 key > V 的第一个元素
        auto it = mMigrators.upper_bound(currentVersion);

        while (it != mMigrators.end() && it->first <= targetVersion) {
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
                assert(executor != nullptr);
                if (!executor(data)) {
                    return ll::makeStringError(
                        fmt::format(
                            "Failed to execute migration unit '{}', current version: '{}'",
                            nextRegisteredVersion,
                            currentVersion
                        )
                    );
                }
            } catch (std::exception const& e) {
                return ll::makeStringError(
                    fmt::format("Exception during migration to v{}: {}", nextRegisteredVersion, e.what())
                );
            }

            // 更新当前版本号到数据中
            currentVersion = nextRegisteredVersion;
            Accessor::setVersion(data, currentVersion);

            // 继续寻找下一个大于当前版本的迁移器
            it = mMigrators.upper_bound(currentVersion);
        }

        if (!allowVersionGap && currentVersion < targetVersion) {
            return ll::makeStringError(fmt::format("Migration failed to reach target v{}", targetVersion));
        }

        return MigrateResult::Success;
    }

    [[nodiscard]] inline std::optional<version_t> getMinVersion() const {
        if (mMigrators.empty()) {
            return std::nullopt;
        }
        return mMigrators.begin()->first;
    }

    [[nodiscard]] inline std::optional<version_t> getMaxVersion() const {
        if (mMigrators.empty()) {
            return std::nullopt;
        }
        return mMigrators.rbegin()->first;
    }


protected:
    std::map<version_t, MigrationUnit> mMigrators;
};

} // namespace land::infra
