#include "pland/gui/form/PaginatedSimpleForm.h"
#include "ll/api/form/CustomForm.h"
#include "mc/world/actor/player/Player.h"
#include <cstddef>
#include <memory>
#include <utility>


namespace land {


PaginatedSimpleForm::ButtonData::ButtonData(std::string text, SimpleForm::ButtonCallback callback)
: mText(std::move(text)),
  mCallback(std::move(callback)) {}

PaginatedSimpleForm::ButtonData::ButtonData(
    std::string                text,
    std::string                imageData,
    std::string                imageType,
    SimpleForm::ButtonCallback callback
)
: mText(std::move(text)),
  mImageData(std::move(imageData)),
  mImageType(std::move(imageType)),
  mCallback(std::move(callback)) {}

PaginatedSimpleForm::PaginatedSimpleForm() = default;

PaginatedSimpleForm::PaginatedSimpleForm(Options options) : mOptions(options) {}

PaginatedSimpleForm::PaginatedSimpleForm(std::string title, Options options)
: mOptions(options),
  mTitle(std::move(title)) {}

PaginatedSimpleForm::PaginatedSimpleForm(std::string title, std::string content, Options options)
: mOptions(options),
  mTitle(std::move(title)),
  mContent(std::move(content)) {}

#ifdef DEBUG
PaginatedSimpleForm::~PaginatedSimpleForm() { std::cout << "PaginatedSimpleForm::~PaginatedSimpleForm()" << std::endl; }
#else
PaginatedSimpleForm::~PaginatedSimpleForm() = default;
#endif

PaginatedSimpleForm& PaginatedSimpleForm::setTitle(std::string title) {
    mTitle   = std::move(title);
    mIsDirty = true;
    return *this;
}

PaginatedSimpleForm& PaginatedSimpleForm::setContent(std::string content) {
    mContent = std::move(content);
    mIsDirty = true;
    return *this;
}

PaginatedSimpleForm& PaginatedSimpleForm::appendButton(
    std::string                text,
    std::string                imageData,
    std::string                imageType,
    SimpleForm::ButtonCallback callback
) {
    mButtons.emplace_back(std::move(text), std::move(imageData), std::move(imageType), std::move(callback));
    mIsDirty = true;
    return *this;
}

PaginatedSimpleForm& PaginatedSimpleForm::appendButton(std::string text, SimpleForm::ButtonCallback callback) {
    mButtons.emplace_back(std::move(text), std::move(callback));
    mIsDirty = true;
    return *this;
}

void PaginatedSimpleForm::sendTo(Player& player) {
    buildSpecialButtons(player);
    buildPages();
    sendFirstPage(player);
}


// impl
PaginatedSimpleForm::Page::Page(std::unique_ptr<SimpleForm> form, std::map<int, ButtonData const&> indexMap)
: mForm(std::move(form)),
  mIndexMap(std::move(indexMap)) {}
void PaginatedSimpleForm::Page::sendTo(Player& player, SimpleForm::Callback cb) const {
    mForm->sendTo(player, std::move(cb));
}
void PaginatedSimpleForm::Page::inovkeCallback(Player& player, int index) const {
    mIndexMap.at(index).mCallback(player);
}

void PaginatedSimpleForm::buildSpecialButtons(Player& player) {
    if (!mSpecialButtons.empty()) {
        return;
    }
    mSpecialButtons.emplace(
        SpecialButton::PrevPage,
        ButtonData{"上一页"_trf(player), "textures/ui/book_pageleft_default", "path"}
    );
    mSpecialButtons.emplace(
        SpecialButton::NextPage,
        ButtonData{"下一页"_trf(player), "textures/ui/book_pageright_default", "path"}
    );
    mSpecialButtons.emplace(
        SpecialButton::Special,
        ButtonData{"跳转到指定页码"_trf(player), "textures/ui/mashup_PaintBrush", "path"}
    );
    mSpecialButtons.emplace(
        SpecialButton::JumpToFirstPage,
        ButtonData{"跳转到第一页"_trf(player), "textures/ui/book_shiftleft_hover", "path"}
    );
    mSpecialButtons.emplace(
        SpecialButton::JumpToLastPage,
        ButtonData{"跳转到最后一页"_trf(player), "textures/ui/book_shiftright_hover", "path"}
    );
}
PaginatedSimpleForm::ButtonData const& PaginatedSimpleForm::getSpecialButton(SpecialButton specialButton) {
    return mSpecialButtons.at(specialButton);
}

PaginatedSimpleForm::Page& PaginatedSimpleForm::getPage(int pageNumber) {
    int pageSize = static_cast<int>(mPages.size());
    if (pageNumber > pageSize || pageNumber <= 0) {
        throw std::runtime_error("Invalid page number: " + std::to_string(pageNumber));
    }
    return mPages[pageNumber - 1];
}

void PaginatedSimpleForm::_countPages() {
    mTotalPages = static_cast<int>(mButtons.size()) / mOptions.pageButtons;
    if (mButtons.size() % mOptions.pageButtons != 0) {
        mTotalPages++; // 如果按钮数量不能被页按钮数量整除，则增加一页(向上取整)
    }
}

void PaginatedSimpleForm::buildPages() {
    if (!mIsDirty) {
        return;
    }
    mIsDirty = false;
    mPages.clear();

    _countPages();

    if (mTotalPages == 0) {
        return;
    }

    // 创建页
    for (int i = 1; i <= mTotalPages; i++) {
        mPages.emplace_back(
            std::make_unique<SimpleForm>(mTitle + "[{}/{}]"_tr(i, mTotalPages), mContent),
            std::map<int, ButtonData const&>()
        );
    }

    // 将按钮分配到页
    int counter     = 1;
    int pageNumber  = 1;
    int buttonIndex = 0;
    for (auto const& button : mButtons) {
        auto& page = getPage(pageNumber);

        if (counter == 1) {
            _beginBuild(page, pageNumber, buttonIndex);
        }

        page.mForm->appendButton(button.mText, button.mImageData, button.mImageType);
        page.mIndexMap.emplace(buttonIndex++, button); // 记录按钮索引

        ++counter;
        if (counter > mOptions.pageButtons) {
            _endBuild(page, pageNumber, buttonIndex);
            pageNumber++;
            counter     = 1;
            buttonIndex = 0;
        }
    }

    if (counter > 1 && counter < mOptions.pageButtons) {
        _endBuild(getPage(pageNumber), pageNumber, buttonIndex);
    }
}
void PaginatedSimpleForm::_beginBuild(Page& page, int pageNumber, int& buttonIndex) {
    if (pageNumber == 1 || mTotalPages == 1) {
        auto& prev = getSpecialButton(SpecialButton::PrevPage);
        page.mForm->appendButton(prev.mText, prev.mImageData, prev.mImageType);
        page.mIndexMap.emplace(buttonIndex++, prev); // 记录按钮索引
    }
}
void PaginatedSimpleForm::_endBuild(Page& page, int pageNumber, int& buttonIndex) {
    if (pageNumber != mTotalPages) {
        // 不是最后一页，添加下一页按钮
        auto& next = getSpecialButton(SpecialButton::NextPage);
        page.mForm->appendButton(next.mText, next.mImageData, next.mImageType);
        page.mIndexMap.emplace(buttonIndex++, next); // 记录按钮索引
    }

    if (mOptions.enableJumpFirstOrLast) {
        if (pageNumber != 1) {
            auto& first = getSpecialButton(SpecialButton::JumpToFirstPage);
            page.mForm->appendButton(first.mText, first.mImageData, first.mImageType);
            page.mIndexMap.emplace(buttonIndex++, first); // 记录按钮索引
        }

        if (mOptions.enableJumpSpecial) {
            auto& jump = getSpecialButton(SpecialButton::Special);
            page.mForm->appendButton(jump.mText, jump.mImageData, jump.mImageType);
            page.mIndexMap.emplace(buttonIndex++, jump); // 记录按钮索引
        }

        if (pageNumber != mTotalPages) {
            auto& last = getSpecialButton(SpecialButton::JumpToLastPage);
            page.mForm->appendButton(last.mText, last.mImageData, last.mImageType);
            page.mIndexMap.emplace(buttonIndex++, last); // 记录按钮索引
        }
    }
}


PaginatedSimpleForm::Page const& PaginatedSimpleForm::getPrevPage() {
    if (mCurrentPageNumber <= 1) {
        return getPage(mCurrentPageNumber);
    }
    return getPage(mCurrentPageNumber - 1);
}
PaginatedSimpleForm::Page const& PaginatedSimpleForm::getNextPage() {
    if (mCurrentPageNumber >= static_cast<int>(mPages.size())) {
        return getPage(mCurrentPageNumber);
    }
    return getPage(mCurrentPageNumber + 1);
}
PaginatedSimpleForm::Page const& PaginatedSimpleForm::getCurrentPage() { return getPage(mCurrentPageNumber); }
PaginatedSimpleForm::Page const& PaginatedSimpleForm::getFirstPage() { return *mPages.begin(); }
PaginatedSimpleForm::Page const& PaginatedSimpleForm::getLastPage() { return *mPages.rbegin(); }

SimpleForm::Callback PaginatedSimpleForm::makeCallback() {
    return [thiz = shared_from_this()](Player& self, int index, auto) {
        if (index == -1) {
#ifdef DEBUG
            std::cout << "PaginatedSimpleForm::makeCallback: form canceled, ref count = " << thiz.use_count()
                      << std::endl;
#endif
            return;
        }
        thiz->getPage(thiz->mCurrentPageNumber).inovkeCallback(self, index);
    };
}
void PaginatedSimpleForm::sendPrevPage(Player& player) {
    getPrevPage().sendTo(player, makeCallback());
    mCurrentPageNumber--;
}
void PaginatedSimpleForm::sendNextPage(Player& player) {
    getNextPage().sendTo(player, makeCallback());
    mCurrentPageNumber++;
}
void PaginatedSimpleForm::sendFirstPage(Player& player) {
    getFirstPage().sendTo(player, makeCallback());
    mCurrentPageNumber = 1;
}
void PaginatedSimpleForm::sendLastPage(Player& player) {
    getLastPage().sendTo(player, makeCallback());
    mCurrentPageNumber = static_cast<int>(mPages.size());
}
void PaginatedSimpleForm::sendSpecialPage(Player& player, int pageNumber) {
    getPage(pageNumber).sendTo(player, makeCallback());
    mCurrentPageNumber = pageNumber;
}
void PaginatedSimpleForm::sendChoosePageForm(Player& player) {
    auto f = std::make_unique<ll::form::CustomForm>();
    f->setTitle("跳转到指定页码"_trf(player));
    f->appendSlider("page", "页码", 1, static_cast<double>(mPages.size()), 1.0, mCurrentPageNumber);
    f->setSubmitButton("跳转"_trf(player));
    f->sendTo(player, [thiz = shared_from_this()](Player& self, ll::form::CustomFormResult const& res, auto) {
        if (!res) {
#ifdef DEBUG
            std::cout << "PaginatedSimpleForm::sendChoosePageForm: form canceled, ref count = " << thiz.use_count()
                      << std::endl;
#endif
            return; // 表单取消后，不在有地方持有 thiz 的引用，这里的 thiz 会自动释放
        }
        auto page = static_cast<int>(std::get<double>(res->at("page")));
        thiz->sendSpecialPage(self, page);
    });
}


} // namespace land