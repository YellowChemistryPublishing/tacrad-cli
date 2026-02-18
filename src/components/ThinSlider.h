#pragma once

#include <Debug.h>
#include <functional>
#pragma once

#include <algorithm>
#include <format>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/util/ref.hpp>

#include <module/sys>
#include <module/sys.Text>

#include <Config.h>
#include <Exec.inl>
#include <Screen.h>
#include <Style.h>

namespace ui = ftxui;

class ThinSliderImpl : public ui::ComponentBase
{
    std::function<void(float)> onChange;
    std::function<void(float&)> onDraw;
    ui::ElementDecorator decoratePreReflect = ui::nothing;

    float value = 0.0f; // b/w `[0.0f, 1.0f]`
    ui::Box bounds;
    ui::CapturedMouse capturedMouse;
    ui::Component displayComp = ui::Renderer(
                                    [this]
    {
        this->onDraw(this->value);

        const i32 totalWidth = i32(this->bounds.x_max) - i32(this->bounds.x_min) + 1_i32;
        const i32 filledWidth = i32(std::floorf(this->value * _as(float, totalWidth)));

        return ui::hbox({
                   ui::separatorCharacter(UserSettings::ProgressBarFill) | ui::color(UserSettings::FlavorEmphasizedColor) | ui::size(ui::WIDTH, ui::EQUAL, filledWidth),
                   ui::separatorCharacter(UserSettings::ProgressBarFill) | ui::color(UserSettings::FlavorUnemphasizedColor) | ui::xflex,
               }) |
            ui::selectionStyleReset | this->decoratePreReflect | ui::reflect(this->bounds);
    }) |
        ui::CatchEvent([this](ui::Event event)
    {
        if (!event.is_mouse())
            return false;

        if (!this->capturedMouse && event.mouse().button == ui::Mouse::Left && event.mouse().motion == ui::Mouse::Pressed && this->bounds.Contain(event.mouse().x, event.mouse().y))
        {
            this->capturedMouse = this->CaptureMouse(event);
            this->displayComp->TakeFocus();
        }

        if (!this->capturedMouse)
            return false;

        if (event.mouse().motion == ui::Mouse::Released)
        {
            this->capturedMouse.reset();
            return false;
        }

        if (this->bounds.x_min < this->bounds.x_max)
        {
            const float width = _as(float, i32(this->bounds.x_max) - i32(this->bounds.x_min) + 1_i32);
            const float value = std::clamp(std::floorf(_as(float, event.mouse().x - this->bounds.x_min)) / width, 0.0f, 1.0f);
            if (this->value != value)
            {
                this->onChange(value);
                this->value = value;

                return true;
            }
        }

        return false;
    });
public:
    explicit ThinSliderImpl(std::function<void(float)> onChange, std::function<void(float&)> onDraw, ui::ElementDecorator decoratePreReflect = ui::nothing) :
        onChange(std::move(onChange)), onDraw(std::move(onDraw)), decoratePreReflect(std::move(decoratePreReflect))
    {
        this->Add(this->displayComp);
    }
};

/// @brief Create a status bar component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ ThinSlider(std::function<void(float)> onChange, std::function<void(float&)> onDraw,
                                                                            ui::ElementDecorator decoratePreReflect = ui::nothing)
{
    return ui::Make<ThinSliderImpl>(std::move(onChange), std::move(onDraw), std::move(decoratePreReflect));
}
