#include "BackPaginatedSimpleForm.h"
#include "pland/gui/form/PaginatedSimpleForm.h"

namespace land {


BackPaginatedSimpleForm::BackPaginatedSimpleForm()
: mFactory(std::make_unique<PaginatedSimpleFormFactory>(PaginatedSimpleFormFactory::Options{})) {}

BackPaginatedSimpleForm& BackPaginatedSimpleForm::setTitle(std::string title) {
    mFactory->setTitle(std::move(title));
    return *this;
}

BackPaginatedSimpleForm& BackPaginatedSimpleForm::setContent(std::string content) {
    mFactory->setContent(std::move(content));
    return *this;
}


BackPaginatedSimpleForm& BackPaginatedSimpleForm::appendButton(std::string text, ButtonCallback callback) {
    mFactory->appendButton(std::move(text), std::move(callback));
    return *this;
}

BackPaginatedSimpleForm& BackPaginatedSimpleForm::appendButton(
    std::string    text,
    std::string    imageData,
    std::string    imageType,
    ButtonCallback callback
) {
    mFactory->appendButton(std::move(text), std::move(imageData), std::move(imageType), std::move(callback));
    return *this;
}


BackPaginatedSimpleForm& BackPaginatedSimpleForm::sendTo(Player& player) {
    injectBackButton();
    mFactory->buildAndSendTo(player);
    return *this;
}

void BackPaginatedSimpleForm::injectBackButton() {
    if (mBackCallback && !mIsAddedBackButton) {
        mIsAddedBackButton = true;

        mFactory->mButtons.insert(
            mFactory->mButtons.begin(),
            PaginatedSimpleFormFactory::ButtonData{
                .mText      = "Back",
                .mImageData = "textures/ui/icon_import",
                .mImageType = "path",
                .mCallback  = std::move(mBackCallback)
            }
        );
    }
}


} // namespace land