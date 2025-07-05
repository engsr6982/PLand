#pragma once
#include "ll/api/reflection/Deserialization.h"
#include "ll/api/reflection/Serialization.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include <algorithm>
#include <iostream>
#include <string_view>


namespace land {


template <class T>
concept HasVersion =
    ll::reflection::Reflectable<T> && std::integral<std::remove_cvref_t<decltype((std::declval<T>().version))>>;


class JSON {
public:
    JSON()                       = delete;
    ~JSON()                      = delete;
    JSON(JSON&&)                 = delete;
    JSON(const JSON&)            = delete;
    JSON& operator=(JSON&&)      = delete;
    JSON& operator=(const JSON&) = delete;

    [[nodiscard]] static nlohmann::json parse(std::string const& str) { return nlohmann::json::parse(str); }
    [[nodiscard]] static nlohmann::json parse(std::string_view const& str) { return nlohmann::json::parse(str); }
    [[nodiscard]] static std::string    stringify(nlohmann::json const& j, int indent = -1) { return j.dump(indent); }

    // class/vector/struct -> json
    template <class T>
    [[nodiscard]] static nlohmann::ordered_json structTojson(T& obj) {
        return ll::reflection::serialize<nlohmann::ordered_json>(obj).value();
    }


    // 直接反序列化
    template <class T, class J = nlohmann::ordered_json>
    static void jsonToStructNoMerge(J& j, T& obj) {
        ll::reflection::deserialize(obj, j).value();
    }

    // 尝试合并patch再反序列化
    template <HasVersion T, class J = nlohmann::ordered_json, class F = bool(T&, J&)>
    static void jsonToStruct(J& j, T& obj, F&& fixer = _tryMergePatch<T, J>) {
        bool noNeedMerge = true;
        if (!j.contains("version") || (int64)(j["version"]) != obj.version) {
            noNeedMerge = false;
        }
        if (noNeedMerge || std::invoke(std::forward<F>(fixer), obj, j)) {
            ll::reflection::deserialize(obj, j).value();
        }
    }

    template <class T, class J = nlohmann::ordered_json>
    static void jsonToStructTryPatch(J& json, T& obj) {
        auto  patch      = ll::reflection::serialize<J>(obj); // obj -> patch
        auto& patchValue = patch.value();
        patchValue.merge_patch(json);         // patch -> json
        jsonToStructNoMerge(patchValue, obj); // json -> obj
    }

    template <HasVersion T, class J>
    static bool _tryMergePatch(T& obj, J& data) {
        data.erase("version");
        auto patch = ll::reflection::serialize<J>(obj);
        patch.value().merge_patch(data);
        data = *std::move(patch);
        return true;
    }
};


} // namespace land
