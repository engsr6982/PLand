#pragma once
#include "ll/api/form/SimpleForm.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include <functional>
#include <memory>
#include <type_traits>


namespace wrapper {

template <typename T, typename = void>
struct HasImplMethod : std::false_type {};
template <typename T>
struct HasImplMethod<T, std::void_t<decltype(T::impl)>> : std::true_type {};

enum class BackButtonPos : bool { Upper = true, Lower = false }; // 返回按钮位置

class SimpleFormEx {
public:
    using Callback       = ll::form::SimpleForm::Callback;
    using ButtonCallback = ll::form::SimpleForm::ButtonCallback;
    using BackCallback   = std::function<void(Player&)>;

    LDAPI SimpleFormEx();
    LDAPI SimpleFormEx(BackCallback backCallback, BackButtonPos pos = BackButtonPos::Lower);

    SimpleFormEx(SimpleFormEx&&) noexcept            = default;
    SimpleFormEx& operator=(SimpleFormEx&&) noexcept = default;
    SimpleFormEx(const SimpleFormEx&)                = delete;
    SimpleFormEx& operator=(const SimpleFormEx&)     = delete;


    LDAPI SimpleFormEx& setTitle(std::string const& title);
    SimpleFormEx&       setContent(std::string const& content);

    LDAPI SimpleFormEx& appendButton(std::string const& text, ButtonCallback callback = {});
    LDAPI SimpleFormEx&
    appendButton(std::string const& text, std::string const& imageData, ButtonCallback callback = {});
    LDAPI SimpleFormEx& appendButton(
        std::string const& text,
        std::string const& imageData,
        std::string const& imageType,
        ButtonCallback     callback = {}
    );
    LDAPI SimpleFormEx& sendTo(Player& player, Callback callback = Callback()); // Lower 在 sendTo 时添加


    //                  父表单                  返回按钮位置                      要回传给父表单的参数
    template <typename ParentForm = void, auto ButtonPos = BackButtonPos::Lower, typename... Args>
    LDNDAPI static SimpleFormEx create(Args&&... args) {
        if constexpr (std::is_same_v<ParentForm, void>) {
            return SimpleFormEx{}; // 没有父表单
        } else {
            static_assert(HasImplMethod<ParentForm>::value, "ParentForm must have impl method");
            return SimpleFormEx{
                [args = std::make_tuple(std::forward<Args>(args)...)](Player& p) {
                    std::apply(
                        [&p](auto&&... fargs) { ParentForm::impl(p, std::forward<decltype(fargs)>(fargs)...); },
                        args
                    );
                },
                ButtonPos
            };
        }
    }

private:
    std::unique_ptr<ll::form::SimpleForm> form;

    bool          mIsUpperAdded{false};                    // 是否已经添加了 Upper 返回按钮
    BackButtonPos mSimpleFormExBack{BackButtonPos::Lower}; // 返回按钮位置
    BackCallback  mBackCallback{nullptr};                  // 返回按钮回调
};


} // namespace wrapper