#include <Preamble.h>

#include <algorithm>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <module/sys>

#include <Exec.inl>
#include <Utility.h>

class ConsoleImpl : public ui::ComponentBase, public std::enable_shared_from_this<ConsoleImpl>
{
    i32 selected = 0;
    ui::Component containerComp = ui::Container::Vertical({}, &*this->selected);
    ui::Box bounds;

    sz lastCmdIndex = 0_uz;
    i32 lastLineWidth = 1_i32;
    std::vector<std::string> lastLines;

    [[nodiscard]] bool Focusable() const final { return true; }

    [[nodiscard]] bool OnEvent(ui::Event event) final
    {
        const i32 selectedOld = this->selected;
        this->ComponentBase::OnEvent(event);
        if (this->selected != selectedOld)
            this->TakeFocus();
        return this->selected != selectedOld;
    }

    [[nodiscard]] ui::Element postProcessRow(ui::EntryState state)
    {
        ui::Element ret = ui::text(state.label);
        if (state.index == this->selected) // Circumvent native behaviour of unselecting when console not focused.
            ret = ret | ui::bold | ui::focus;
        if (state.active)
            ret |= ui::underlined;
        if (!state.active && state.index != this->selected)
            ret |= ui::dim;
        return ret;
    }
    [[nodiscard]] ui::Component createRow(std::string str)
    {
        return ui::MenuEntry(std::move(str),
                             ui::MenuEntryOption { .transform = [this](ui::EntryState state) -> ui::Element { return this->postProcessRow(state); },
                                                   .animated_colors = ui::AnimatedColorsOption() });
    }

    [[nodiscard]] ui::Element InternalRender() // NOLINT(readability-identifier-naming)
    {
        return (this->lastLines.empty() ? ui::text("<empty>") | ui::center : this->ComponentBase::Render()) | ui::vscroll_indicator | ui::yframe | ui::reflect(this->bounds);
    }
    [[nodiscard]] ui::Element OnRender() final
    {
        const std::vector<CommandInvocation::Entry>& history = CommandInvocation::rawHistory();
        i32 maxLineWidth = std::max(i32(this->bounds.x_max) - i32(this->bounds.x_min), i32::highest());
        if (this->lastCmdIndex > history.size() || this->lastLineWidth != maxLineWidth)
        {
            this->containerComp->DetachAllChildren();
            this->lastLines.clear();
            this->lastCmdIndex = 0_uz;
            this->lastLineWidth = maxLineWidth;
        }

        _retif(this->InternalRender(), this->lastCmdIndex == history.size());

        const sz lastLinesSizeOld = this->lastLines.size();
        for (const auto& entry : std::span(history.begin() + *ssz(this->lastCmdIndex), history.end()))
        {
            const auto process = [&](const std::string& text)
            {
                if (text.empty())
                    return;

                wstringSplitLengthConstrained(text, sz(maxLineWidth), this->lastLines);
            };

            process(entry.cmd);
            process(entry.output);
        }

        const bool follow = (this->selected >= i32(this->containerComp->ChildCount()) - 1_i32) || (this->containerComp->ChildCount() == 0_uz);

        for (sz i = lastLinesSizeOld; i < this->lastLines.size(); i++)
            this->containerComp->Add(this->createRow(this->lastLines[i]));

        this->lastCmdIndex = history.size();

        if (follow && !this->lastLines.empty())
            this->selected = i32(this->lastLines.size()) - 1_i32;

        return this->InternalRender();
    }
public:
    explicit ConsoleImpl() { this->Add(this->containerComp); }
};

inline ui::Component /* NOLINT(readability-identifier-naming) */ Console() { return ui::Make<ConsoleImpl>(); }
