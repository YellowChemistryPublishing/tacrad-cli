#pragma once

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/util/ref.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <module/sys>

#include <Config.h>
#include <components/TabContainer.h>

namespace ui = ftxui;

/// @brief Tab selector banner for different UI views.
/// @note Pass `byptr`.
class TabSelectImpl : public ui::ComponentBase
{
    static inline const std::vector<std::string> TabValues = {
        "UI",
        "Console",
    };

    const std::shared_ptr<TabContainerImpl> containerComp;
    ui::Component selectorComp = ui::Menu(&TabSelectImpl::TabValues, &*this->containerComp->selectedTab, []() -> ui::MenuOption
    {
        ui::MenuOption ret = ui::MenuOption::HorizontalAnimated();
        ret.underline.color_active = UserSettings::FlavorEmphasizedColor;
        ret.underline.color_inactive = UserSettings::FlavorUnemphasizedColor;
        ret.underline.leader_duration = Config::FlavorAnimationDuration;
        ret.underline.leader_function = ui::animation::easing::Linear;
        ret.underline.follower_duration = Config::FlavorAnimationDuration;
        ret.underline.follower_function = ui::animation::easing::Linear;
        return ret; // NOLINT(clang-analyzer-core.StackAddressEscape): Spurious.
    }());
public:
    explicit TabSelectImpl(std::shared_ptr<TabContainerImpl> containerComp) : containerComp(std::move(containerComp)) { this->Add(this->selectorComp); }
};

/// @brief Create a `TabSelectImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ TabSelect(std::shared_ptr<TabContainerImpl> containerComp)
{
    return ui::Make<TabSelectImpl>(std::move(containerComp));
}
