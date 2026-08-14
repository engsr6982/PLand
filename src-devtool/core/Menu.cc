#include "core/Menu.h"
#include "core/Window.h"
#include <imgui.h>
#include <ranges>
#include <utility>

namespace devtool {

IMenuElement::IMenuElement(std::string label) : label_(std::move(label)) {}

IMenuElement::IMenuElement(std::string label, std::string shortCut)
: label_(std::move(label)),
  shortCut_(std::move(shortCut)) {}

std::string const& IMenuElement::getLabel() const { return label_; }

std::string const& IMenuElement::getShortCut() const { return shortCut_; }

bool IMenuElement::isEnable() const { return enable_; }

bool IMenuElement::isSelected() const { return window_ ? window_->visible() : selected_; }

bool* IMenuElement::getEnableFlag() { return &enable_; }

bool* IMenuElement::getSelectFlag() { return window_ ? window_->getVisibleFlag() : &selected_; }

void IMenuElement::setWindow(IWindow* window) { window_ = window; }

void IMenuElement::render() { ImGui::MenuItem(label_.data(), shortCut_.data(), getSelectFlag()); }


IMenu::IMenu(std::string label) : label_(std::move(label)) {}

void IMenu::registerElementImpl(std::unique_ptr<IMenuElement> element) {
    this->elements_.emplace(element->getLabel(), std::move(element));
}

std::string const& IMenu::getLabel() const { return label_; }

bool IMenu::isEnable() const { return enable_; }

bool* IMenu::getEnableFlag() { return &enable_; }

void IMenu::render() {
    if (ImGui::BeginMenu(label_.data(), getEnableFlag())) {
        for (auto const& element : elements_ | std::views::values) {
            element->render();
        }
        ImGui::EndMenu();
    }
}

} // namespace devtool
