#pragma once
#include "PaginatedSimpleForm.h"
#include "pland/gui/form/BackSimpleForm.h"


namespace land {


class BackPaginatedSimpleForm {

    std::shared_ptr<PaginatedSimpleForm> impl;

public:
    BackPaginatedSimpleForm() : impl(PaginatedSimpleForm::make()) {}

    BackPaginatedSimpleForm& setTitle(std::string title) {
        impl->setTitle(std::move(title));
        return *this;
    }

    BackPaginatedSimpleForm& setContent(std::string content) {
        impl->setContent(std::move(content));
        return *this;
    }

    // concept: HasAppendButtonMethods
    template <typename... Args>
    BackPaginatedSimpleForm& appendButton(Args&&... args) {
        impl->appendButton(std::forward<Args>(args)...);
        return *this;
    }

    // concept: HasSendToMethod
    template <typename... Args>
    void sendTo(Args&&... args) {
        impl->sendTo(std::forward<Args>(args)...);
    }

    template <typename... Args>
    BackPaginatedSimpleForm& onFormCanceled(Args&&... args) {
        impl->onFormCanceled(std::forward<Args>(args)...);
        return *this;
    }
};

static_assert(HasSendToMethod<BackPaginatedSimpleForm>, "BackPaginatedSimpleForm must satisfy HasSendToMethod");
static_assert(
    HasAppendButtonMethods<BackPaginatedSimpleForm>,
    "BackPaginatedSimpleForm must satisfy HasAppendButtonMethods"
);
static_assert(
    DisallowEnableSharedFromThis<BackPaginatedSimpleForm>,
    "BackPaginatedSimpleForm must not satisfy std::enable_shared_from_this<T>"
);


} // namespace land