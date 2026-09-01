#pragma once
#include "DataMigrator.h"

#include <nlohmann/json.hpp>

namespace land::infra {

template <typename J>
struct JsonVersionAccessor {
    inline static constexpr std::string_view kVersionKey = "version";
    inline static int32_t                    getVersion(J const& j) { return j.value(kVersionKey, 0); }
    inline static void                       setVersion(J& j, int32_t version) { j[kVersionKey] = version; }
};

template <typename J = nlohmann::json>
class JsonMigrator : public DataMigrator<J, JsonVersionAccessor<J>> {
public:
    using Base = DataMigrator<J, JsonVersionAccessor<J>>;

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
    inline static void mapPath(Base::container_t& root, std::string_view src, std::string_view dst) {
        // 辅助函数：把 "land.bought.xxx" 变成 "/land/bought/xxx"
        auto toPtr = [](std::string_view s) {
            std::string res = "/";
            for (char c : s) {
                res += (c == '.' ? '/' : c);
            }
            return typename Base::container_t::json_pointer(res);
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

    inline static void mapPath(Base::container_t& root, Route const& route) { mapPath(root, route.src, route.dst); }
};

} // namespace land::infra