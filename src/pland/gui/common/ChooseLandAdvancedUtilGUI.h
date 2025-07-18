#pragma once
#include "pland/Global.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/land/Land.h"
#include <memory>


namespace land {


class ChooseLandAdvancedUtilGUI {
    class Impl;
    std::shared_ptr<Impl> impl_;

public:
    using ChooseCallback = std::function<void(Player&, SharedLand choosedLand)>;

    enum class View {
        All = 0,      // 所有领地视图
        OnlyOrdinary, // 普通领地视图
        OnlyParent,   // 父领地视图
        OnlyMix,      // 混合领地视图
        OnlySub,      // 子领地视图
    };

    LDAPI explicit ChooseLandAdvancedUtilGUI(
        std::vector<SharedLand>          lands,
        ChooseCallback                   callback,
        BackSimpleForm<>::ButtonCallback back = {}
    );

    LDAPI void sendTo(Player& player);

    template <typename... Args>
    static void sendTo(Player& player, Args&&... args) {
        ChooseLandAdvancedUtilGUI{std::forward<Args>(args)...}.sendTo(player);
    }
};


} // namespace land