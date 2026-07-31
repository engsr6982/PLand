#pragma once
#include "repo/LandContext.h"

#include <shared_mutex>


namespace land {


class LandTemplatePermTable {
public:
    LDAPI explicit LandTemplatePermTable(LandPermTable permTable);

    /// @brief 获取模板权限表的副本（线程安全）
    LDAPI LandPermTable get() const;

    /// @brief 设置模板权限表（线程安全）
    LDAPI void set(LandPermTable const& permTable);

    LDAPI bool isDirty() const;
    LDAPI void markDirty();
    LDAPI void resetDirty();

private:
    mutable std::shared_mutex mMutex;
    std::atomic_bool          mDirty{false};
    LandPermTable             mTemplatePermTable;
};


} // namespace land
