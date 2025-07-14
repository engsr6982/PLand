#pragma once
#include "pland/Global.h"

namespace land {


/**
 * @brief Require<T> RAII 资源访问类
 */
template <typename T>
class Require {
public:
    LD_DISALLOW_COPY_AND_MOVE(Require);

    LDAPI explicit Require<T>() = default;
    LDAPI ~Require<T>()         = default;


    LDNDAPI T* operator->() {
        static_assert(sizeof(T) == 0, "Require<T> is not implemented for this type.");
        return nullptr;
    }

    T* get() = delete;
};

#define LD_ERR_RAII_RESOURCE_EMPTY(T) "Require<" #T "> is empty."

#define LD_DECLARE_REQUIRE(T)                                                                                          \
    template <>                                                                                                        \
    class Require<T> {                                                                                                 \
    public:                                                                                                            \
        LDAPI explicit Require<T>() = default;                                                                         \
        LDAPI ~Require<T>()         = default;                                                                         \
        LDNDAPI T* operator->();                                                                                       \
    }


} // namespace land