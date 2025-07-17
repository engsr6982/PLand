#pragma once
#include "pland/Global.h"
#include <memory>


namespace land {


struct GeoId {
    LDAPI virtual ~GeoId() = default;

    LDNDAPI virtual uint64 value() const                        = 0;
    LDNDAPI virtual bool   operator==(GeoId const& other) const = 0;

    template <typename T>
    [[nodiscard]] T* as() {
        return dynamic_cast<T*>(this);
    }
};

using GeoIdPtr = std::unique_ptr<GeoId>;


} // namespace land