#pragma once
#include <concepts>
#include <string>

class BlockPos;
class Player;

namespace land {

class AbstractSelector {
public:
    AbstractSelector(AbstractSelector const&)            = delete;
    AbstractSelector& operator=(AbstractSelector const&) = delete;
    AbstractSelector(AbstractSelector&&)                 = default;
    AbstractSelector& operator=(AbstractSelector&&)      = default;

    AbstractSelector() = default;

    virtual ~AbstractSelector() = default;

    virtual Player* getPlayer() const = 0;

    virtual bool isSpecifiedSelectTool(std::string const& typeName) const = 0;

    virtual void selectNext(Player& player, BlockPos const& p) = 0;

    virtual void tick() = 0;

    template <std::derived_from<AbstractSelector> T>
    T* as() {
        return dynamic_cast<T*>(this);
    }
};

} // namespace land