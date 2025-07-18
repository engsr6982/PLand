#pragma once
#include "fmt/format.h"
#include <memory>
#include <vector>


namespace land {

#ifdef DEBUG
template <typename T>
struct FuckSharedResourceMemoryLeak {
    std::vector<std::weak_ptr<T>> resources{};

    FuckSharedResourceMemoryLeak<T>() = default;
    ~FuckSharedResourceMemoryLeak() {
        for (auto& w : resources) {
            auto p = w.lock();
            if (p) {
                assert(false);
                fmt::print("[Leak] Resource not released: {:p}\n", (void*)p.get());
            }
        }
    }

    void add(std::shared_ptr<T> resource) {
        resources.push_back(resource);
        fmt::print("[Leak] Added resource: {:p}\n", (void*)resource.get());
    }
};

template <typename T>
FuckSharedResourceMemoryLeak<T> GlobalFuckSharedResourceMemoryLeak{};
#endif

} // namespace land