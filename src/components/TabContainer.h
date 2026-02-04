#pragma once

#include <Preamble.h>

#include <memory>

#include <module/sys>

class TabContainerImpl : public ui::ComponentBase, public std::enable_shared_from_this<TabContainerImpl>
{
public:
    i32 tabSelected = 0_i32; // NOLINT(misc-non-private-member-variables-in-classes)
private:
    const ui::Components components;
    const ui::Component containerComp = ui::Container::Tab(this->components, &*this->tabSelected);
public:
    explicit TabContainerImpl(ui::Components components) : components(std::move(components)) { this->Add(this->containerComp); }

    TabContainerImpl(const TabContainerImpl&) = delete;
    TabContainerImpl(TabContainerImpl&&) = delete;
    ~TabContainerImpl() override = default;

    TabContainerImpl& operator=(const TabContainerImpl&) = delete;
    TabContainerImpl& operator=(TabContainerImpl&&) = delete;
};

inline ui::Component /* NOLINT(readability-identifier-naming) */ TabContainer(ui::Components components) { return ui::Make<TabContainerImpl>(std::move(components)); }
