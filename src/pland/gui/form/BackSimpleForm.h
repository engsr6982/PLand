#pragma once
#include "ll/api/form/SimpleForm.h"
#include "pland/Global.h"
#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>


namespace land {

using ll::form::SimpleForm;


template <typename T>
concept HasAppendButtonMethods = requires(
    T                          t,
    std::string                text,
    std::string                imageData,
    std::string                imageType,
    SimpleForm::ButtonCallback callback,
    const std::string&         const_text,
    const std::string&         const_imageData,
    const std::string&         const_imageType
) {
    requires(
        requires { // const&
            { t.appendButton(const_text, const_imageData, const_imageType, callback) } -> std::same_as<T&>;
            { t.appendButton(const_text, callback) } -> std::same_as<T&>;
        }
        || requires { // 值传递
            { t.appendButton(text, imageData, imageType, callback) } -> std::same_as<T&>;
            { t.appendButton(text, callback) } -> std::same_as<T&>;
        }
    );
};

template <typename T>
concept HasSendTo1Arg = requires(T t, Player& p) {
    requires (
        requires { { t.sendTo(p) } -> std::same_as<T&>; } ||
        requires { { t.sendTo(p) } -> std::same_as<void>; } 
    );
};

template <typename T>
concept HasSendTo2Arg = requires(T t, Player& p, SimpleForm::Callback cb) {
    { t.sendTo(p, cb) } -> std::same_as<T&>;
};

template <typename T>
concept HasSendToMethod = HasSendTo1Arg<T> || HasSendTo2Arg<T>;

template <typename T>
concept DisallowEnableSharedFromThis = !std::is_base_of_v<std::enable_shared_from_this<T>, T>;

template <typename T = SimpleForm>
    requires HasAppendButtonMethods<T> && HasSendToMethod<T> && DisallowEnableSharedFromThis<T>
class BackSimpleForm : public T {
public:
    static_assert(HasAppendButtonMethods<T>, "T must satisfy HasAppendButtonMethods");
    static_assert(HasSendToMethod<T>, "T must satisfy HasSendToMethod");
    static_assert(DisallowEnableSharedFromThis<T>, "T must not satisfy std::enable_shared_from_this<T>");

    enum class ButtonPos { Upper, Lower };

    using Callback       = SimpleForm::Callback;
    using ButtonCallback = SimpleForm::ButtonCallback;

    // using T::appendButton;
    // using T::sendTo;

    explicit BackSimpleForm() : T{} {}

    explicit BackSimpleForm(ButtonCallback backCallback, ButtonPos buttonPos = ButtonPos::Upper)
    : T{},
      mButtonPos(buttonPos),
      mBackCallback(std::move(backCallback)) {
        if (mBackCallback && !mIsAddedBackButton) {
            if (mButtonPos == ButtonPos::Upper) {
                mIsAddedBackButton = true;
                T::appendButton("Back", "textures/ui/icon_import", "path", mBackCallback);
            }
        }
    }

    BackSimpleForm(const BackSimpleForm&)            = delete;
    BackSimpleForm& operator=(const BackSimpleForm&) = delete;
    BackSimpleForm(BackSimpleForm&&)                 = default;
    BackSimpleForm& operator=(BackSimpleForm&&)      = default;

    BackSimpleForm& sendTo(Player& player) {
        if (mBackCallback && !mIsAddedBackButton) {
            if (mButtonPos == ButtonPos::Lower) {
                mIsAddedBackButton = true;
                T::appendButton("Back", "textures/ui/icon_import", "path", mBackCallback);
            }
        }

        if constexpr (HasSendTo2Arg<T>) {
            T::sendTo(player, SimpleForm::Callback{});
        } else if constexpr (HasSendTo1Arg<T>) {
            T::sendTo(player);
        } else {
            static_assert(HasSendToMethod<T>, "T must satisfy HasSendToMethod");
        }

        return *this;
    }

    // factory method
    template <auto Fn, typename... Args>
        requires std::invocable<decltype(Fn), Player&, Args...>
    static ButtonCallback makeCallback(Args&&... args) {
        static_assert(
            std::is_invocable_v<decltype(Fn), Player&, Args...>,
            "Fn must be callable with (Player&, Args...)"
        );
        return [args = std::make_tuple(std::forward<Args>(args)...)](Player& p) mutable {
            std::apply(
                [&p](auto&&... unpacked) {
                    Fn(p, std::forward<decltype(unpacked)>(unpacked)...); // 直接调用
                },
                std::move(args)
            );
        };
    }


    template <auto ParentFn = nullptr, auto BP = ButtonPos::Upper, typename... Args>
    static BackSimpleForm make(Args&&... args) {
        if constexpr (ParentFn == nullptr) {
            return BackSimpleForm{}; // 没有父表单，不需要返回按钮
        } else {
            return BackSimpleForm{makeCallback<ParentFn>(std::forward<Args>(args)...), BP};
        }
    }


private:
    bool           mIsAddedBackButton{false};
    ButtonPos      mButtonPos{ButtonPos::Upper};
    ButtonCallback mBackCallback{nullptr};
};

} // namespace land
