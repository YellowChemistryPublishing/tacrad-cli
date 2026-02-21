#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <utility>

#include <module/sys>

namespace ui = ftxui;

/// @brief Container for different UI views.
/// @note Pass `byptr`.
class TabContainerImpl : public ui::ComponentBase
{
public:
    // Currently selected tab index.
    i32 selectedTab = 0_i32; // NOLINT(misc-non-private-member-variables-in-classes)

    explicit TabContainerImpl(ui::Components components) { this->Add(ui::Container::Tab(std::move(components), &*this->selectedTab)); }
};

/// @brief Create a `TabContainerImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ TabContainer(ui::Components components) { return ui::Make<TabContainerImpl>(std::move(components)); }
