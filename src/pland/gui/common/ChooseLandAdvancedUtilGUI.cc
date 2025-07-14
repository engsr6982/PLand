#include "ChooseLandAdvancedUtilGUI.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/SimpleForm.h"
#include "pland/gui/form/BackPaginatedSimpleForm.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/gui/form/PaginatedSimpleForm.h"
#include <cassert>

#ifdef DEBUG
#include "pland/debug/FuckSharedResourceMemoryLeak.h"
#endif

namespace land {

using ll::form::CustomForm;
using ll::form::CustomFormResult;


class ChooseLandAdvancedUtilGUI::Impl : public std::enable_shared_from_this<Impl> {
    std::vector<SharedLand>                                 mLands{};                    // 领地数据
    ChooseCallback                                          mCallback{};                 // 回调
    BackSimpleForm<>::ButtonCallback                        mBackCallback{};             // 返回按钮回调
    std::optional<std::string>                              mFuzzyKeyword{std::nullopt}; // 模糊搜索关键字
    View                                                    mCurrentView{View::All};     // 当前视图
    std::map<View, BackSimpleForm<BackPaginatedSimpleForm>> mViews{};                    // 视图

    std::shared_ptr<Impl> mSelf{nullptr};

public:
    explicit Impl(std::vector<SharedLand> lands, ChooseCallback callback, BackSimpleForm<>::ButtonCallback back = {})
    : mLands(std::move(lands)),
      mCallback(std::move(callback)),
      mBackCallback(std::move(back)) {}

#ifdef DEBUG
    virtual ~Impl() { std::cout << "ChooseLandAdvancedUtilGUI::Impl::~Impl()" << std::endl; }
#else
    virtual ~Impl() = default;
#endif

    void freeSelf() { mSelf.reset(); }

    BackSimpleForm<>::ButtonCallback makeBackCallback() {
        return [thiz = std::weak_ptr(shared_from_this())](Player& self) {
            if (auto ins = thiz.lock()) {
                ins->mBackCallback(self);
                ins->freeSelf();
            }
        };
    }

    SimpleForm::ButtonCallback makeCallback(WeakLand wLand) {
        return [wLand, thiz = std::weak_ptr(shared_from_this())](Player& self) {
            auto ins  = thiz.lock();
            auto land = wLand.lock();
            if (ins && land) {
                ins->mCallback(self, land);
                ins->freeSelf();
            }
        };
    }

    PaginatedSimpleForm::FormCanceledCallback makeFormCanceledCallback() {
        return [thiz = std::weak_ptr(shared_from_this())](Player&) {
            if (auto ins = thiz.lock()) {
                ins->freeSelf();
            }
        };
    }

    void nextView(Player& player) {
        if (mCurrentView == View::OnlySub) {
            sendView(View::All, player); // 回到初始视图
            return;
        }
        sendView(static_cast<View>(static_cast<int>(mCurrentView) + 1), player);
    }

    SimpleForm::ButtonCallback makeNextViewCallback() {
        return [thiz = std::weak_ptr(shared_from_this())](Player& self) {
            if (auto ins = thiz.lock()) {
                ins->nextView(self);
            }
        };
    }

    SimpleForm::ButtonCallback makeFuzzySearchCallback() {
        return [thiz = std::weak_ptr(shared_from_this())](Player& self) {
            if (auto ins = thiz.lock()) {
                ins->sendFuzzySearch(self);
            }
        };
    }

    void buildForms(Player& player) {
        if (mViews.empty()) {
            mViews.emplace(View::All, BackSimpleForm<BackPaginatedSimpleForm>::make(makeBackCallback()));
            mViews.emplace(View::OnlyOrdinary, BackSimpleForm<BackPaginatedSimpleForm>::make(makeBackCallback()));
            mViews.emplace(View::OnlyParent, BackSimpleForm<BackPaginatedSimpleForm>::make(makeBackCallback()));
            mViews.emplace(View::OnlyMix, BackSimpleForm<BackPaginatedSimpleForm>::make(makeBackCallback()));
            mViews.emplace(View::OnlySub, BackSimpleForm<BackPaginatedSimpleForm>::make(makeBackCallback()));

            for (auto& [view, form] : mViews) {
                form.setTitle("选择领地"_trf(player));
                form.setContent("请选择一个领地:"_trf(player));
                form.appendButton("模糊搜索", "textures/ui/magnifyingGlass", "path", makeFuzzySearchCallback());
                form.onFormCanceled(makeFormCanceledCallback());

                switch (view) {
                case View::All:
                    form.appendButton(
                        "过滤: >全部领地<"_trf(player),
                        "textures/ui/store_sort_icon",
                        "path",
                        makeNextViewCallback()
                    );
                    break;
                case View::OnlyOrdinary:
                    form.appendButton(
                        "过滤: >普通领地<"_trf(player),
                        "textures/ui/store_sort_icon",
                        "path",
                        makeNextViewCallback()
                    );
                    break;
                case View::OnlyParent:
                    form.appendButton(
                        "过滤: >父领地<"_trf(player),
                        "textures/ui/store_sort_icon",
                        "path",
                        makeNextViewCallback()
                    );
                    break;
                case View::OnlyMix:
                    form.appendButton(
                        "切换: >混合领地<"_trf(player),
                        "textures/ui/store_sort_icon",
                        "path",
                        makeNextViewCallback()
                    );
                    break;
                case View::OnlySub:
                    form.appendButton(
                        "过滤: >子领地<"_trf(player),
                        "textures/ui/store_sort_icon",
                        "path",
                        makeNextViewCallback()
                    );
                    break;
                }
            }
        }

        auto thiz = std::weak_ptr(shared_from_this());
        for (auto const& land : mLands) {
            if (mFuzzyKeyword && land->getName().find(*mFuzzyKeyword) == std::string::npos) {
                continue; // 模糊搜索
            }

            auto wLand = std::weak_ptr(land);

            std::string text =
                "{}\n维度: {} | ID: {}"_trf(player, land->getName(), land->getDimensionId(), land->getId());

            mViews.at(View::All).appendButton(text, "textures/ui/icon_recipe_nature", "path", makeCallback(wLand));

            switch (land->getType()) {
            case Land::Type::Ordinary:
                mViews.at(View::OnlyOrdinary)
                    .appendButton(text, "textures/ui/icon_recipe_nature", "path", makeCallback(wLand));
                break;
            case Land::Type::Parent:
                mViews.at(View::OnlyParent)
                    .appendButton(text, "textures/ui/icon_recipe_nature", "path", makeCallback(wLand));
                break;
            case Land::Type::Mix:
                mViews.at(View::OnlyMix)
                    .appendButton(text, "textures/ui/icon_recipe_nature", "path", makeCallback(wLand));
                break;
            case Land::Type::Sub:
                mViews.at(View::OnlySub)
                    .appendButton(text, "textures/ui/icon_recipe_nature", "path", makeCallback(wLand));
                break;
            }
        }
    }

    void sendFuzzySearch(Player& player) {
        CustomForm fm;
        fm.setTitle(PLUGIN_NAME + " | 模糊搜索领地"_trf(player));
        fm.appendInput("name", "请输入领地名称"_trf(player), "string", mFuzzyKeyword.value_or(""));
        fm.sendTo(player, [thiz = std::weak_ptr(shared_from_this())](Player& self, CustomFormResult const& res, auto) {
            auto ins = thiz.lock();
            if (!ins) {
                return;
            }
            if (!res) {
                ins->freeSelf();
                return;
            }
            auto name = std::get<string>(res->at("name"));
            if (name.empty()) {
                return ins->sendFuzzySearch(self); // 重新发送
            }
            ins->mFuzzyKeyword = name;
            ins->mViews.clear();
            ins->buildForms(self);
            ins->sendView(ins->mCurrentView, self);
        });
    }

    void sendView(View view, Player& player) {
        mCurrentView = view;
        mViews.at(view).sendTo(player);
        mSelf = shared_from_this();
    }

    void sendTo(Player& player) {
        buildForms(player);
        sendView(View::All, player);
    }
};


// ChooseLandAdvancedUtilGUI
ChooseLandAdvancedUtilGUI::ChooseLandAdvancedUtilGUI(
    std::vector<SharedLand>          lands,
    ChooseCallback                   callback,
    BackSimpleForm<>::ButtonCallback back
) {
    impl_ = std::make_shared<Impl>(std::move(lands), std::move(callback), std::move(back));
}

void ChooseLandAdvancedUtilGUI::sendTo(Player& player) {
    impl_->sendTo(player);

#ifdef DEBUG
    GlobalFuckSharedResourceMemoryLeak<Impl>.add(impl_);
#endif
}


} // namespace land