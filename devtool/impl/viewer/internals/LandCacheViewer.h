#pragma once
#include "components/CodeEditor.h"
#include "components/IComponent.h"
#include "pland/Global.h"
#include "pland/land/Land.h"

namespace devtool::internals {

class LandEditor : public CodeEditor {
    land::WeakLand land_;

    friend class LandCacheViewerWindow;

public:
    explicit LandEditor(land::SharedLand land);

    void renderMenuElement() override;
};

class LandCacheViewerWindow : public IWindow {
    std::unordered_map<land::UUIDs, std::unordered_set<land::SharedLand>> lands_;     // 领地缓存
    std::unordered_map<land::UUIDs, std::string>                          realNames_; // 玩家名缓存
    std::unordered_map<land::UUIDs, bool>                                 isShow_;    // 是否显示该玩家的领地
    std::unordered_map<land::LandID, std::unique_ptr<LandEditor>>         editors_;   // 领地数据编辑器

    bool showAllPlayerLand_{true}; // 是否显示所有玩家的领地
    bool showOrdinaryLand_{true};  // 是否显示普通领地
    bool showParentLand_{true};    // 是否显示父领地
    bool showMixLand_{true};       // 是否显示混合领地
    bool showSubLand_{true};       // 是否显示子领地
    int  dimensionFilter_{-1};     // 维度过滤
    int  idFilter_{-1};            // 领地ID过滤

public:
    explicit LandCacheViewerWindow();

    enum Buttons {
        EditLand,  // 编辑领地数据
        ExportLand // 导出领地数据
    };
    void handleButtonClicked(Buttons bt, land::SharedLand land);

    void handleEditLand(land::SharedLand land);
    void handleExportLand(land::SharedLand land);

    void renderCacheLand(); // 渲染缓存的领地

    void renderToolBar(); // 渲染工具栏

    void preBuildData(); // 预处理数据

    void render() override;

    void tick() override;
};

class LandCacheViewer final : public IMenuElement {
    std::unique_ptr<LandCacheViewerWindow> window_;

public:
    explicit LandCacheViewer();

    bool* getSelectFlag() override;
    bool  isSelected() const override;

    void tick() override;
};


} // namespace devtool::internals