#pragma once
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/SimpleForm.h"
#include "pland/Global.h"
#include <map>
#include <memory>
#include <utility>
#include <vector>

#ifdef DEBUG
#include "pland/debug/FuckSharedResourceMemoryLeak.h"
#endif


namespace land {


using SimpleForm = ll::form::SimpleForm;


class PaginatedSimpleForm : public std::enable_shared_from_this<PaginatedSimpleForm> {
public:
    struct Options {
        int  pageButtons           = 32;   // 每页按钮数量
        bool enableJumpFirstOrLast = true; // 是否启用跳转到第一页和最后一页按钮
        bool enableJumpSpecial     = true; // 是否启用跳转到指定页按钮
    };

    using FormCanceledCallback = std::function<void(Player& player)>;

public:
    template <typename... Args>
    static std::shared_ptr<PaginatedSimpleForm> make(Args&&... args) {
// return std::make_shared<PaginatedSimpleForm>(std::forward<Args>(args)...);
#ifdef DEBUG
        auto p = std::shared_ptr<PaginatedSimpleForm>(new PaginatedSimpleForm(std::forward<Args>(args)...));
        GlobalFuckSharedResourceMemoryLeak<PaginatedSimpleForm>.add(p);
        return p;
#else
        return std::shared_ptr<PaginatedSimpleForm>(new PaginatedSimpleForm(std::forward<Args>(args)...));
#endif
    }

    LDAPI virtual ~PaginatedSimpleForm();

    LDAPI PaginatedSimpleForm& setTitle(std::string title);

    LDAPI PaginatedSimpleForm& setContent(std::string content);

    LDAPI PaginatedSimpleForm& appendButton(
        std::string                text,
        std::string                imageData,
        std::string                imageType,
        SimpleForm::ButtonCallback callback = {}
    );

    LDAPI PaginatedSimpleForm& appendButton(std::string text, SimpleForm::ButtonCallback callback = {});

    LDAPI void sendTo(Player& player);

    /**
     * @brief 表单被取消时的回调函数
     */
    LDAPI PaginatedSimpleForm& onFormCanceled(FormCanceledCallback cb = {});

private:
    LDAPI explicit PaginatedSimpleForm();
    LDAPI explicit PaginatedSimpleForm(Options options);
    LDAPI explicit PaginatedSimpleForm(std::string title, Options options);
    LDAPI explicit PaginatedSimpleForm(std::string title, std::string content, Options options);


    struct ButtonData {
        std::string                mText;
        std::string                mImageData;
        std::string                mImageType;
        SimpleForm::ButtonCallback mCallback;

        LD_DISALLOW_COPY(ButtonData);
        ButtonData(ButtonData&&) noexcept            = default;
        ButtonData& operator=(ButtonData&&) noexcept = default;
        LDAPI explicit ButtonData(std::string text, SimpleForm::ButtonCallback callback = {});
        LDAPI explicit ButtonData(
            std::string                text,
            std::string                imageData,
            std::string                imageType,
            SimpleForm::ButtonCallback callback = {}
        );
    };

    enum class SpecialButton {
        PrevPage,        // 上一页
        NextPage,        // 下一页
        Special,         // 跳转到指定页
        JumpToFirstPage, // 跳转到第一页
        JumpToLastPage,  // 跳转到最后一页
    };

    class Page final {
        std::unique_ptr<SimpleForm>      mForm;     // 表单数据
        std::map<int, ButtonData const&> mIndexMap; // 按钮索引映射

        friend PaginatedSimpleForm;

    public:
        LD_DISALLOW_COPY(Page);
        Page(Page&&) noexcept            = default;
        Page& operator=(Page&&) noexcept = default;
        Page(std::unique_ptr<SimpleForm> form, std::map<int, ButtonData const&> indexMap);
        void sendTo(Player& player, SimpleForm::Callback cb) const;
        void inovkeCallback(Player& player, int index) const;
    };


    void              buildSpecialButtons(Player& player);
    ButtonData const& getSpecialButton(SpecialButton specialButton);

    Page& getPage(int pageNumber);

    void buildPages(); // 生成分页表单数据
    void _countPages();
    void _beginBuild(Page& page, int pageNumber, int& buttonIndex);
    void _endBuild(Page& page, int pageNumber, int& buttonIndex);

    SimpleForm::Callback makeCallback(); // 创建回调函数

    void sendPrevPage(Player& player);                    // 发送上一页
    void sendNextPage(Player& player);                    // 发送下一页
    void sendFirstPage(Player& player);                   // 发送第一页
    void sendLastPage(Player& player);                    // 发送最后一页
    void sendSpecialPage(Player& player, int pageNumber); // 发送跳转到指定页

    void sendChoosePageForm(Player& player); // 发送跳转到指定页表单

    void invokeCancelCallback(Player& player);


    // 表单数据(原始)
    Options                             mOptions{};        // 表单选项
    std::string                         mTitle;            // 表单标题
    std::string                         mContent;          // 表单内容
    std::vector<ButtonData>             mButtons;          // 按钮数据
    std::map<SpecialButton, ButtonData> mSpecialButtons;   // 特殊按钮数据
    bool                                mIsDirty{true};    // 是否需要重新生成表单
    FormCanceledCallback                mFormCanceledCb{}; // 表单取消回调函数

    // 表单数据(分页)
    std::vector<Page> mPages;                // 分页表单数据
    int               mTotalPages{0};        // 总页数
    int               mCurrentPageNumber{1}; // 当前页码(从1开始)
};


} // namespace land