#include "pland/gui/form/PaginatedSimpleForm.h"
#include <cstddef>
#include <memory>
#include <utility>


namespace land {


// Factory
PaginatedSimpleFormFactory::PaginatedSimpleFormFactory(int pageButtons) : mPageButtons(pageButtons) {}

PaginatedSimpleFormFactory::PaginatedSimpleFormFactory(std::string title, int pageButtons)
: mTitle(std::move(title)),
  mPageButtons(pageButtons) {}

PaginatedSimpleFormFactory::PaginatedSimpleFormFactory(std::string title, std::string content, int pageButtons)
: mTitle(std::move(title)),
  mContent(std::move(content)),
  mPageButtons(pageButtons) {}

PaginatedSimpleFormFactory& PaginatedSimpleFormFactory::setTitle(std::string title) {
    mTitle = std::move(title);
    return *this;
}

PaginatedSimpleFormFactory& PaginatedSimpleFormFactory::setContent(std::string content) {
    mContent = std::move(content);
    return *this;
}

PaginatedSimpleFormFactory& PaginatedSimpleFormFactory::appendButton(
    std::string                text,
    std::string                imageData,
    std::string                imageType,
    SimpleForm::ButtonCallback callback
) {
    mButtons.emplace_back(ButtonData{
        .mText      = std::move(text),
        .mImageData = std::move(imageData),
        .mImageType = std::move(imageType),
        .mCallback  = std::move(callback)
    });
    return *this;
}

PaginatedSimpleFormFactory&
PaginatedSimpleFormFactory::appendButton(std::string text, SimpleForm::ButtonCallback callback) {
    mButtons.emplace_back(
        ButtonData{.mText = std::move(text), .mImageData = "", .mImageType = "", .mCallback = std::move(callback)}
    );
    return *this;
}

void PaginatedSimpleFormFactory::buildAndSendTo(Player& player) {
    // 计算总页数，向上取整
    int pageCount = static_cast<int>(mButtons.size()) / mPageButtons;
    if (mButtons.size() % mPageButtons != 0) {
        pageCount++;
    }

    if (pageCount == 0) {
        return;
    }

    auto paginatedForm = std::make_shared<PaginatedSimpleForm>();

    {
        auto& paginatedData = paginatedForm->mPaginatedData;
        auto& pages         = paginatedData->mPages;

        paginatedData->mTitle   = std::move(mTitle); // 移交标题
        paginatedData->mContent = std::move(mContent);

        auto const& title   = paginatedData->mTitle;
        auto const& content = paginatedData->mContent;

        // 创建页
        for (int i = 1; i <= pageCount; i++) {
            std::unique_ptr<SimpleForm> _fm;

            if (!title.empty() && !content.empty()) {
                _fm = std::make_unique<SimpleForm>(title, content);
            } else if (!title.empty()) {
                _fm = std::make_unique<SimpleForm>(title);
            } else {
                _fm = std::make_unique<SimpleForm>();
            }

            auto page = std::make_pair<std::unique_ptr<SimpleForm>, std::vector<ButtonData>>(
                std::move(_fm),
                std::vector<ButtonData>()
            );
            pages.push_back(std::move(page));
        }
    }

    // 将按钮分配到页
    int counter    = 1;
    int pageNumber = 1;
    for (auto& button : mButtons) {
        if (counter == 1) {
            beginBuildPage(paginatedForm, player, pageNumber, pageCount);
        }

        // 转移按钮到页
        auto& page = paginatedForm->getPage(pageNumber);

        if (button.mImageData.empty() && button.mImageType.empty()) {
            page.first->appendButton(button.mText, std::move(button.mCallback));
        } else {
            page.first->appendButton(button.mText, button.mImageData, button.mImageType, std::move(button.mCallback));
        }
        page.second.push_back(std::move(button));

        ++counter;
        if (counter > mPageButtons) {
            endBuildPage(paginatedForm, player, pageNumber, pageCount);
            pageNumber++;
            counter = 1;
        }
    }

    if (counter > 1 && counter < mPageButtons) {
        endBuildPage(paginatedForm, player, pageNumber, pageCount); // 最后一页按钮数量不足，补齐
    }

    paginatedForm->sendTo(player);
}

// 分页功能按钮添加规则：
//   第一页：有下一页按钮(end)
//   中间页：有上一页(begin)和下一页按钮(end)
//   最后一页：有上一页按钮(begin)
//  每页都有第一页和最后一页的跳转按钮(end)
void PaginatedSimpleFormFactory::beginBuildPage(
    std::shared_ptr<PaginatedSimpleForm> fm,
    Player&                              player,
    int                                  pageNumber,
    int                                  pageSize
) {
    if (pageNumber == 1 || pageSize == 1) {
        return; // 第一页不需要添加上一页按钮 或者 只有一页
    }
    auto data = ButtonData{
        .mText      = "上一页\n页码: {}/{}"_trf(player, pageNumber, pageSize),
        .mImageData = "textures/ui/book_pageleft_default",
        .mImageType = "path"
    };

    auto& page = fm->getPage(pageNumber);
    page.first->appendButton(data.mText, data.mImageData, data.mImageType, [thiz = fm](Player& self) {
        if (thiz) {
            thiz->sendPrevPage(self);
        }
    });
    page.second.push_back(std::move(data));
}
void PaginatedSimpleFormFactory::endBuildPage(
    std::shared_ptr<PaginatedSimpleForm> fm,
    Player&                              player,
    int                                  pageNumber,
    int                                  pageSize
) {
    auto& page = fm->getPage(pageNumber);

    if (pageNumber != pageSize) {
        // 不是最后一页，添加下一页按钮
        auto data = ButtonData{
            .mText      = "下一页\n页码: {}/{}"_trf(player, pageNumber, pageSize),
            .mImageData = "textures/ui/book_pageright_default",
            .mImageType = "path"
        };
        page.first->appendButton(data.mText, data.mImageData, data.mImageType, [thiz = fm](Player& self) {
            if (thiz) {
                thiz->sendNextPage(self);
            }
        });
        page.second.push_back(std::move(data));
    }

    if (pageNumber != 1) {
        auto first = ButtonData{
            .mText      = "跳转到第一页"_trf(player),
            .mImageData = "textures/ui/book_shiftleft_hover",
            .mImageType = "path"
        };
        page.first->appendButton(first.mText, first.mImageData, first.mImageType, [thiz = fm](Player& self) {
            if (thiz) {
                thiz->sendFirstPage(self);
            }
        });
        page.second.push_back(std::move(first));
    }

    if (pageNumber != pageSize) {
        auto last = ButtonData{
            .mText      = "跳转到最后一页"_trf(player),
            .mImageData = "textures/ui/book_shiftright_hover",
            .mImageType = "path"
        };
        page.first->appendButton(last.mText, last.mImageData, last.mImageType, [thiz = fm](Player& self) {
            if (thiz) {
                thiz->sendLastPage(self);
            }
        });
        page.second.push_back(std::move(last));
    }
}


// PaginatedSimpleForm
PaginatedSimpleForm::PaginatedSimpleForm()
: mPaginatedData(std::make_unique<PaginatedSimpleFormFactory::PaginatedData>()) {}

PaginatedSimpleForm::~PaginatedSimpleForm() = default;

// mPageNumber 从 1 开始
PaginatedSimpleFormFactory::Page& PaginatedSimpleForm::getPage(int pageNumber) {
    int pageSize = static_cast<int>(mPaginatedData->mPages.size());
    if (pageNumber > pageSize || pageNumber <= 0) {
        throw std::runtime_error("Invalid page number: " + std::to_string(pageNumber));
    }
    return mPaginatedData->mPages[pageNumber - 1];
}


SimpleForm* PaginatedSimpleForm::getPrevPage() {
    if (mPageNumber <= 1) {
        return getFirstPage(); // 循环
    }
    return getPage(mPageNumber - 1).first.get();
}
SimpleForm* PaginatedSimpleForm::getNextPage() {
    if (mPageNumber >= static_cast<int>(mPaginatedData->mPages.size())) {
        return getLastPage(); // 循环
    }
    return getPage(mPageNumber + 1).first.get();
}
SimpleForm* PaginatedSimpleForm::getCurrentPage() { return getPage(mPageNumber).first.get(); }
SimpleForm* PaginatedSimpleForm::getFirstPage() { return mPaginatedData->mPages.begin()->first.get(); }
SimpleForm* PaginatedSimpleForm::getLastPage() { return mPaginatedData->mPages.rbegin()->first.get(); }


void PaginatedSimpleForm::sendPrevPage(Player& player) {
    if (auto p = getPrevPage()) {
        sendPage(player, p);
        mPageNumber--;
    }
}
void PaginatedSimpleForm::sendNextPage(Player& player) {
    if (auto p = getNextPage()) {
        sendPage(player, p);
        mPageNumber++;
    }
}
void PaginatedSimpleForm::sendFirstPage(Player& player) {
    if (auto p = getFirstPage()) {
        sendPage(player, p);
        mPageNumber = 1;
    }
}
void PaginatedSimpleForm::sendLastPage(Player& player) {
    if (auto p = getLastPage()) {
        sendPage(player, p);
        mPageNumber = static_cast<int>(mPaginatedData->mPages.size());
    }
}


void PaginatedSimpleForm::sendPage(Player& player, SimpleForm* page) {
    if (!page) {
        throw std::runtime_error("Invalid page");
    }
    page->sendTo(player);
}
void PaginatedSimpleForm::sendTo(Player& player) { sendFirstPage(player); }


} // namespace land