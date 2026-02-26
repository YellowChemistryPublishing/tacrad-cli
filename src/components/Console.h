#pragma once

/// @file Console.h

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <CmdInv.inl>
#include <Config.h>
#include <Style.h>
#include <Utility.h>

namespace ui = ftxui;

/// @brief Displays command history and output.
/// @note Pass `byptr`.
class ConsoleImpl final : public ui::ComponentBase
{
public:
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::vector<CmdInv::Entry> history; ///< Command invocation (and output) history.
private:
    std::vector<sys::cstr> lastLines;
    sz linesSize = 0_uz;
    i32 lineWidth = i32::highest();
    i32 selected = 0_i32;

    sz lastHistorySize = 0_uz;
    ui::Box bounds;

    void renderLastLines(const i32 withLineWidth)
    {
        if (this->lastHistorySize > this->history.size() || this->lineWidth != withLineWidth)
        {
            this->containerComp->DetachAllChildren();
            this->lastLines.clear();
            this->lastHistorySize = 0_uz;
            this->lineWidth = withLineWidth;
        }

        if (withLineWidth <= 0)
            return;

        this->linesSize = this->lastLines.size();
        for (const auto& entry : std::span(this->history.begin() + *ssz(this->lastHistorySize), this->history.end()))
        {
            const auto process = [&](const sys::cstr& text)
            {
                if (text.empty())
                    return;

                stringSplitLengthConstrained(text, sz(withLineWidth), this->lastLines);
            };

            process(entry.cmd);
            process(entry.output);
        }
    }
    void sync()
    {
        const bool follow = (this->selected >= i32(this->containerComp->ChildCount()) - 1_i32) || (this->containerComp->ChildCount() == 0_uz);

        for (sz i = this->linesSize; i < this->lastLines.size(); i++)
            this->containerComp->Add(this->createEntry(this->lastLines[i]));
        this->lastHistorySize = this->history.size();

        if (follow && !this->lastLines.empty())
            this->selected = i32(this->lastLines.size()) - 1_i32;
    }
    void syncIfNeeded()
    {
        const i32 lnWidth = i32(this->bounds.x_max) - i32(this->bounds.x_min);
        if (this->lastHistorySize == this->history.size() && this->lineWidth == lnWidth)
            return;

        this->renderLastLines(lnWidth);
        this->sync();
    }

    [[nodiscard]] ui::Element postProcessEntry(const ui::EntryState& state)
    {
        ui::Element ret = ui::text(state.label);
        if (state.index == this->selected) // Circumvent native behaviour of unselecting when console not focused.
            ret = std::move(ret) | ui::bold | ui::focus;
        if (state.active)
            ret |= ui::underlined;
        if (!state.active && state.index != this->selected)
            ret |= ui::dim;
        return ret;
    }
    [[nodiscard]] ui::Component createEntry(sys::cstr str)
    {
        return ui::MenuEntry(_as(std::string_view, str),
                             ui::MenuEntryOption { .transform = [this](const ui::EntryState& state) -> ui::Element { return this->postProcessEntry(state); },
                                                   .animated_colors = ui::AnimatedColorsOption() });
    }

    ui::Component containerComp = ui::Container::Vertical({}, &*this->selected);
    ui::Component displayComp = ui::Renderer(this->containerComp, [this]() -> ui::Element
    {
        this->syncIfNeeded();
        return (this->lastLines.empty() ? ui::text(Config::BlankText) | ui::center : this->containerComp->Render()) | vscroll(this->bounds);
    });
public:
    explicit ConsoleImpl() { this->Add(this->displayComp); }
};

/// @brief Create a `ConsoleImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Console() { return ui::Make<ConsoleImpl>(); }
