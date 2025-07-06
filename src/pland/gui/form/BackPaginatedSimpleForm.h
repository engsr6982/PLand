#pragma once
#include "PaginatedSimpleForm.h"


namespace land {


class BackPaginatedSimpleForm {
public:
    using ButtonCallback = ll::form::SimpleForm::ButtonCallback;

private:
    std::unique_ptr<PaginatedSimpleFormFactory> mFactory;
    ButtonCallback                              mBackCallback{};
    bool                                        mIsAddedBackButton = false;

public:
    LDAPI BackPaginatedSimpleForm();

    LDAPI BackPaginatedSimpleForm& setTitle(std::string title);

    LDAPI BackPaginatedSimpleForm& setContent(std::string content);

    // concept: HasAppendButtonMethods
    LDAPI BackPaginatedSimpleForm& appendButton(std::string text, ButtonCallback callback = {});

    LDAPI BackPaginatedSimpleForm&
    appendButton(std::string text, std::string imageData, std::string imageType, ButtonCallback callback = {});

    // concept: HasSendToMethod
    LDAPI BackPaginatedSimpleForm& sendTo(Player& player);

private:
    void injectBackButton();
};


} // namespace land