#include <Preamble.h>

#include <memory>

#include <CmdInv.h>
#include <Config.h>

class ConsoleImpl : public ui::ComponentBase, public std::enable_shared_from_this<ConsoleImpl>
{
    i32 selected = 0;
    ui::Component containerComp = ui::Container::Vertical({}, &*this->selected);
    ui::Box bounds;

    std::vector<std::string> lastLines;

    ui::Element postProcessRow(ui::EntryState state)
    {
        ui::Element ret = ui::text(state.label);
        if (state.index == this->selected) // Circumvent native behaviour of unselecting when console not focused.
            ret = ret | ui::bold | ui::focus;
        if (state.active)
            ret = ret | ui::underlined;
        return ret;
    }
    ui::Component createRow(std::string str)
    {
        return ui::MenuEntry(std::move(str),
                             ui::MenuEntryOption { .transform = [this](ui::EntryState state) -> ui::Element { return this->postProcessRow(state); },
                                                   .animated_colors = ui::AnimatedColorsOption() });
    }

    bool OnEvent(ui::Event event) final
    {
        this->ComponentBase::OnEvent(event);

        i32 selectedOld = this->selected;
        if (event == ui::Event::ArrowUp || (event.is_mouse() && event.mouse().button == ui::Mouse::WheelUp))
            --this->selected;
        if (event == ui::Event::ArrowDown || (event.is_mouse() && event.mouse().button == ui::Mouse::WheelDown))
            ++this->selected;
        this->selected = std::max(0_i32, std::min(this->selected, i32(this->containerComp->ChildCount()) - 1_i32));

        if (this->selected != selectedOld)
            this->TakeFocus();
        return this->selected != selectedOld;
    }

    ui::Element OnRender() final
    {
        i32 maxLineWidth = std::max(i32(this->bounds.x_max) - i32(this->bounds.x_min), 1_i32);

        std::vector<std::string> lines;
        for (const auto& entry : CommandInvocation::rawHistory())
        {
            const auto process = [&](const std::string& text)
            {
                if (text.empty())
                    return;

                for (sz i = 0_uz; i < text.size();)
                {
                    sz iEnd = i;
                    while (iEnd < text.size() && text[iEnd] != '\n' && sz(iEnd - i) < sz(maxLineWidth))
                        iEnd++;
                    lines.emplace_back(text.substr(sz(i), sz(iEnd - i)));
                    i = (iEnd == i || (iEnd < text.size() && text[iEnd] == '\n')) ? iEnd + 1_uz : iEnd;
                }
            };

            process(entry.cmd);
            process(entry.output);
        }

        if (lines != this->lastLines)
        {
            const bool follow = (this->selected >= i32(this->containerComp->ChildCount()) - 1_i32) || (this->containerComp->ChildCount() == 0_uz);

            this->containerComp->DetachAllChildren();
            for (const auto& line : lines)
                this->containerComp->Add(this->createRow(line));

            if (follow && !lines.empty())
                this->selected = i32(lines.size()) - 1_i32;

            this->lastLines = std::move(lines);
        }

        return (this->lastLines.empty() ? ui::filler() : this->ComponentBase::Render()) | ui::vscroll_indicator | ui::yframe | ui::reflect(this->bounds);
    }
    [[nodiscard]] bool Focusable() const final { return true; }
public:
    explicit ConsoleImpl() { this->Add(this->containerComp); }
};

inline ui::Component /* NOLINT(readability-identifier-naming) */ Console() { return ui::Make<ConsoleImpl>(); }
