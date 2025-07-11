#pragma once
#include "pland/Global.h"
#include <atomic>

namespace land {

class DirtyCounter {
private:
    std::atomic<unsigned int> mCounter{0};

public:
    LDAPI DirtyCounter();

    LDNDAPI bool isDirty() const; // 是否为脏数据

    LDNDAPI int getCounter() const; // 获取计数器

    LDAPI void increment(); // 增加计数器

    LDAPI void reset(); // 重置计数器
};

} // namespace land
