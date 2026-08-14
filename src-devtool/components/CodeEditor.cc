#include "CodeEditor.h"
#include "TextEditor.h"
#include "fmt/compile.h"
#include "imgui.h"
#include <format>

namespace devtool {


CodeEditor::CodeEditor(std::string const& code) : IWindow("") {
    static int NEXT_WINDOW_ID = 0;
    this->windowId_           = NEXT_WINDOW_ID++;
    this->title_              = fmt::format("[{}] CodeEditor", windowId_);

    editor_.SetText(code);
    editor_.SetLanguage(TextEditor::Language::Json());
}

void CodeEditor::renderMenuElement() {
    // clang-format off
    #define SHORTCUT "Ctrl-"
    if (ImGui::BeginMenu("编辑")) {
        if (ImGui::MenuItem("撤销", " " SHORTCUT "Z", nullptr, editor_.CanUndo())) { editor_.Undo(); }
        if (ImGui::MenuItem("重做", " " SHORTCUT "Y", nullptr, editor_.CanRedo())) { editor_.Redo(); }
        ImGui::Separator();
        if (ImGui::MenuItem("剪切", " " SHORTCUT "X", nullptr, editor_.AnyCursorHasSelection())) { editor_.Cut(); }
        if (ImGui::MenuItem("复制", " " SHORTCUT "C", nullptr, editor_.AnyCursorHasSelection())) { editor_.Copy(); }
        if (ImGui::MenuItem("粘贴", " " SHORTCUT "V", nullptr, ImGui::GetClipboardText() != nullptr)) { editor_.Paste(); }
        ImGui::Separator();
        if (ImGui::MenuItem("制表符转空格")) { editor_.TabsToSpaces(); }
        if (ImGui::MenuItem("空格转制表符")) { editor_.SpacesToTabs(); }
        if (ImGui::MenuItem("删除行尾空白字符")) { editor_.StripTrailingWhitespaces(); }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("选择")) {
        if (ImGui::MenuItem("全选", " " SHORTCUT "A", nullptr, !editor_.IsEmpty())) { editor_.SelectAll(); }
        ImGui::Separator();
        if (ImGui::MenuItem("增加缩进", " " SHORTCUT "]", nullptr, !editor_.IsEmpty())) { editor_.IndentLines(); }
        if (ImGui::MenuItem("减少缩进", " " SHORTCUT "[", nullptr, !editor_.IsEmpty())) { editor_.DeindentLines(); }
        if (ImGui::MenuItem("上移当前行", nullptr, nullptr, !editor_.IsEmpty())) { editor_.MoveUpLines(); }
        if (ImGui::MenuItem("下移当前行", nullptr, nullptr, !editor_.IsEmpty())) { editor_.MoveDownLines(); }
        if (ImGui::MenuItem("切换注释", " " SHORTCUT "/", nullptr, editor_.HasLanguage())) { editor_.ToggleComments(); }
        ImGui::Separator();
        if (ImGui::MenuItem("转为大写", nullptr, nullptr, editor_.AnyCursorHasSelection())) { editor_.SelectionToUpperCase(); }
        if (ImGui::MenuItem("转为小写", nullptr, nullptr, editor_.AnyCursorHasSelection())) { editor_.SelectionToLowerCase(); }
        ImGui::Separator();
        if (ImGui::MenuItem("添加下一个匹配项", " " SHORTCUT "D", nullptr, editor_.CurrentCursorHasSelection())) { editor_.AddNextOccurrence(); }
        if (ImGui::MenuItem("选择所有匹配项", "^" SHORTCUT "D", nullptr, editor_.CurrentCursorHasSelection())) { editor_.SelectAllOccurrences(); }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("视图")) {
        bool flag;
        flag = editor_.IsShowWhitespacesEnabled(); if (ImGui::MenuItem("显示空白字符", nullptr, &flag)) { editor_.SetShowWhitespacesEnabled(flag); };
        flag = editor_.IsShowLineNumbersEnabled(); if (ImGui::MenuItem("显示行号", nullptr, &flag)) { editor_.SetShowLineNumbersEnabled(flag); };
        flag = editor_.IsShowingMatchingBrackets(); if (ImGui::MenuItem("显示匹配括号", nullptr, &flag)) { editor_.SetShowMatchingBrackets(flag); };
        flag = editor_.IsCompletingPairedGlyphs(); if (ImGui::MenuItem("自动补全配对符号", nullptr, &flag)) { editor_.SetCompletePairedGlyphs(flag); };
        flag = editor_.IsShowPanScrollIndicatorEnabled(); if (ImGui::MenuItem("显示平移/滚动指示器", nullptr, &flag)) { editor_.SetShowPanScrollIndicatorEnabled(flag); };
        flag = editor_.IsMiddleMousePanMode(); if (ImGui::MenuItem("中键平移模式", nullptr, &flag)) { if (flag) editor_.SetMiddleMousePanMode(); else editor_.SetMiddleMouseScrollMode(); };
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("查找")) {
        if (ImGui::MenuItem("查找", " " SHORTCUT "F")) { editor_.OpenFindReplaceWindow(); }
        if (ImGui::MenuItem("查找下一个", " " SHORTCUT "G", nullptr, editor_.HasFindString())) { editor_.FindNext(); }
        if (ImGui::MenuItem("查找全部", "^" SHORTCUT "G", nullptr, editor_.HasFindString())) { editor_.FindAll(); }
        ImGui::EndMenu();
    }
    // clang-format on
}

void CodeEditor::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        renderMenuElement();
        ImGui::EndMenuBar();
    }
}

void CodeEditor::render() {
    WindowScope scope(*this, ImGuiWindowFlags_MenuBar); // 已移除 NoDocking, 支持停靠
    if (!scope.isOpen()) {
        return;
    }

    renderMenuBar();
    editor_.Render(fmt::format("CodeEditor-Impl##{}", windowId_).data());
}


} // namespace devtool
