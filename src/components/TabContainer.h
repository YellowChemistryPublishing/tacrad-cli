#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <utility>

#include <module/sys>

namespace ui = ftxui;

class TabContainerImpl : public ui::ComponentBase
{
public:
    i32 tabSelected = 0_i32; // NOLINT(misc-non-private-member-variables-in-classes)
private:
    const ui::Component tabComp;
public:
    explicit TabContainerImpl(ui::Components components) : tabComp(ui::Container::Tab(std::move(components), &*this->tabSelected)) { this->Add(this->tabComp); }

    TabContainerImpl(const TabContainerImpl&) = delete;
    TabContainerImpl(TabContainerImpl&&) = delete;
    ~TabContainerImpl() override = default;

    TabContainerImpl& operator=(const TabContainerImpl&) = delete;
    TabContainerImpl& operator=(TabContainerImpl&&) = delete;
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline ui::Component TabContainer(ui::Components components) { return ui::Make<TabContainerImpl>(std::move(components)); }
