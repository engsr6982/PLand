#pragma once
#include "ll/api/form/SimpleForm.h"
#include "pland/Global.h"
#include <memory>
#include <utility>
#include <vector>


namespace land {


using SimpleForm = ll::form::SimpleForm;

class PaginatedSimpleFormFactory {
    friend class PaginatedSimpleForm;
    friend class BackPaginatedSimpleForm;

    // 由于分页表单上下文停留时间较长
    // 栈上的字符串如果进行 const& 引用
    // 可能会悬空引用，所以拷贝存储
    struct ButtonData {
        std::string                mText;
        std::string                mImageData{};
        std::string                mImageType{};
        SimpleForm::ButtonCallback mCallback = {};
    };

    using Page  = std::pair<std::unique_ptr<SimpleForm>, std::vector<ButtonData>>; // 表单 + 表单按钮数据
    using Pages = std::vector<Page>;

    struct PaginatedData {
        std::string mTitle;
        std::string mContent;
        Pages       mPages;
    };

    std::string             mTitle;
    std::string             mContent;
    std::vector<ButtonData> mButtons;
    int                     mPageButtons{32}; // 每页按钮数量

    void beginBuildPage(std::shared_ptr<PaginatedSimpleForm> fm, Player& player, int pageNumber, int pageSize);
    void endBuildPage(std::shared_ptr<PaginatedSimpleForm> fm, Player& player, int pageNumber, int pageSize);

public:
    LDAPI PaginatedSimpleFormFactory(int pageButtons = 32);
    LDAPI explicit PaginatedSimpleFormFactory(std::string title, int pageButtons = 32);
    LDAPI explicit PaginatedSimpleFormFactory(std::string title, std::string content = {}, int pageButtons = 32);

    LDAPI PaginatedSimpleFormFactory& setTitle(std::string title);

    LDAPI PaginatedSimpleFormFactory& setContent(std::string content);

    LDAPI PaginatedSimpleFormFactory& appendButton(
        std::string                text,
        std::string                imageData,
        std::string                imageType,
        SimpleForm::ButtonCallback callback = {}
    );

    LDAPI PaginatedSimpleFormFactory& appendButton(std::string text, SimpleForm::ButtonCallback callback = {});

    LDAPI void buildAndSendTo(Player& player);
};


class PaginatedSimpleForm final : public std::enable_shared_from_this<PaginatedSimpleForm> {
    using UniquePaginatedData = std::unique_ptr<PaginatedSimpleFormFactory::PaginatedData>;
    friend PaginatedSimpleFormFactory;

    int                 mPageNumber{1};          // 当前页码
    UniquePaginatedData mPaginatedData{nullptr}; // 分页表单数据

    PaginatedSimpleFormFactory::Page& getPage(int pageNumber);

    SimpleForm* getPrevPage();    // 获取上一页
    SimpleForm* getNextPage();    // 获取下一页
    SimpleForm* getCurrentPage(); // 获取当前页
    SimpleForm* getFirstPage();   // 获取第一页
    SimpleForm* getLastPage();    // 获取最后一页


    void sendPrevPage(Player& player);  // 发送上一页
    void sendNextPage(Player& player);  // 发送下一页
    void sendFirstPage(Player& player); // 发送第一页
    void sendLastPage(Player& player);  // 发送最后一页

    void sendPage(Player& player, SimpleForm* page); // 发送指定页
    void sendTo(Player& player);

public:
    LDAPI PaginatedSimpleForm();
    LDAPI ~PaginatedSimpleForm();
};


} // namespace land