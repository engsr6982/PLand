#include "pland/wrapper/FormEx.h"
#include "ll/api/form/SimpleForm.h"
#include <memory>

namespace wrapper {

using namespace ll::form;

SimpleFormEx::SimpleFormEx() { form = std::make_unique<SimpleForm>(); }
SimpleFormEx::SimpleFormEx(BackCallback backCallback, BackButtonPos pos) {
    form              = std::make_unique<SimpleForm>();
    mBackCallback     = backCallback;
    mSimpleFormExBack = pos;
}

SimpleFormEx& SimpleFormEx::setTitle(const std::string& title) {
    form->setTitle(title);
    return *this;
}
SimpleFormEx& SimpleFormEx::setContent(const std::string& content) {
    form->setContent(content);
    return *this;
}


void AppendBackButton(SimpleForm& fm, SimpleFormEx::BackCallback backCallback) {
    if (backCallback) {
        fm.appendButton("Back", "textures/ui/icon_import", "path", backCallback);
    }
}

SimpleFormEx& SimpleFormEx::appendButton(std::string const& text, ButtonCallback callback) {
    if (!mIsUpperAdded && mBackCallback && (bool)mSimpleFormExBack) {
        AppendBackButton(*form, mBackCallback); // Upper
        mIsUpperAdded = true;
    }
    form->appendButton(text, callback);
    return *this;
}
SimpleFormEx& SimpleFormEx::appendButton(
    std::string const& text,
    std::string const& imageData,
    std::string const& imageType,
    ButtonCallback     callback
) {
    if (!mIsUpperAdded && mBackCallback && (bool)mSimpleFormExBack) {
        AppendBackButton(*form, mBackCallback); // Upper
        mIsUpperAdded = true;
    }
    form->appendButton(text, imageData, imageType, callback);
    return *this;
}
SimpleFormEx&
SimpleFormEx::appendButton(std::string const& text, std::string const& imageData, ButtonCallback callback) {
    if (!mIsUpperAdded && mBackCallback && (bool)mSimpleFormExBack) {
        AppendBackButton(*form, mBackCallback); // Upper
        mIsUpperAdded = true;
    }
    form->appendButton(text, imageData, "path", callback);
    return *this;
}

SimpleFormEx& SimpleFormEx::sendTo(Player& player, Callback callback) {
    if (mBackCallback && !((bool)mSimpleFormExBack)) {
        AppendBackButton(*form, mBackCallback); // Lower
    }
    form->sendTo(player, callback);
    return *this;
}


} // namespace wrapper