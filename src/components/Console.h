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
    std::vector<std::string> lastLines;
    sz lastLinesSizeOld = 0_uz;
    i32 lastLineWidth = 1_i32;
    i32 selected = 0_i32, selectedOld = 0_i32;

    sz lastHistorySize = 0_uz;
    ui::Box bounds;

    void renderLastLines(const std::vector<CommandInvocation::Entry>& history)
    {
        i32 maxLineWidth = std::max(i32(this->bounds.x_max) - i32(this->bounds.x_min), i32::highest());
        if (this->lastHistorySize > history.size() || this->lastLineWidth != maxLineWidth)
        {
            this->containerComp->DetachAllChildren();
            this->lastLines.clear();
            this->lastHistorySize = 0_uz;
            this->lastLineWidth = maxLineWidth;
        }

        this->lastLinesSizeOld = this->lastLines.size();
        for (const auto& entry : std::span(history.begin() + *ssz(this->lastHistorySize), history.end()))
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
    }
    void syncLineComponents(const std::vector<CommandInvocation::Entry>& history)
    {
        const bool follow = (this->selected >= i32(this->containerComp->ChildCount()) - 1_i32) || (this->containerComp->ChildCount() == 0_uz);

        for (sz i = this->lastLinesSizeOld; i < this->lastLines.size(); i++)
            this->containerComp->Add(this->createRow(this->lastLines[i]));

        this->lastHistorySize = history.size();

        if (follow && !this->lastLines.empty())
            this->selected = i32(this->lastLines.size()) - 1_i32;
    }

    [[nodiscard]] ui::Element postProcessRow(const ui::EntryState& state)
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
                             ui::MenuEntryOption { .transform = [this](const ui::EntryState& state) -> ui::Element { return this->postProcessRow(state); },
                                                   .animated_colors = ui::AnimatedColorsOption() });
    }

    ui::Component containerComp = ui::Container::Vertical({}, &*this->selected);
    ui::Component displayComp = ui::Renderer(this->containerComp, [this]() -> ui::Element
    {
        const auto internalRender = [&]() -> ui::Element
        { return (this->lastLines.empty() ? ui::text("<empty>") | ui::center : this->containerComp->Render()) | ui::vscroll_indicator | ui::yframe | ui::reflect(this->bounds); };

        const std::vector<CommandInvocation::Entry>& history = CommandInvocation::rawHistory();
        _retif(internalRender(), this->lastHistorySize == history.size());

        this->renderLastLines(history);
        this->syncLineComponents(history);

        return internalRender();
    });
public:
    explicit ConsoleImpl() { this->Add(this->displayComp); }
};

inline ui::Component /* NOLINT(readability-identifier-naming) */ Console() { return ui::Make<ConsoleImpl>(); }
