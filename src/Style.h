#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <utility>

#include <Config.h>
#include <Music.h>

namespace ui = ftxui;

inline ui::Element hpad(ui::Element elem) { return ui::hbox({ ui::separatorEmpty(), std::move(elem) | ui::xflex, ui::separatorEmpty() }); }
inline ui::Component hspace()
{
    return ui::Renderer([] { return ui::separatorEmpty(); });
}
inline ui::Element psep() { return ui::hbox({ ui::separatorEmpty(), ui::separatorStyled(UserSettings::border), ui::separatorEmpty() }); }

inline ui::Element bordered(ui::Element elem) { return std::move(elem) | ui::borderStyled(UserSettings::border); }
inline ui::Component hborder()
{
    return ui::Renderer([] { return ui::separatorStyled(UserSettings::border); });
}

/// @brief Vertical scrollable region.
/// @param bounds Bounds to reflect region dimensions to.
/// @attention Lifetime assumptions!
/// ```cpp
/// ... obj;
/// ui::Box bounds;
///
/// ... decorated = obj | vscroll(bounds);
///
/// decorated.~...();
/// obj.~...();
/// bounds.~Box();
/// ```
inline ui::ElementDecorator vscroll(ui::Box& bounds)
{
    return [&bounds](ui::Element elem) { return std::move(elem) | ui::vscroll_indicator | ui::yframe | ui::reflect(bounds); };
}

inline ui::Element postProcessIconButton(ui::Element elem, const ui::EntryState& state)
{
    if (state.active)
        elem = std::move(elem) | ui::bold | ui::focus;

    if (state.focused)
        elem = std::move(elem) | ui::color(UserSettings::FlavorEmphasizedColor) | ui::underlined;
    else
        elem |= ui::color(UserSettings::FlavorUnemphasizedColor);

    return elem;
}
inline ui::Element postProcessDisplayListEntry(const ui::EntryState& state, i32 selected, const ui::Box& bounds, auto&& extra_cond = [] { return true; })
{
    ui::Element ret = ui::text(truncateStrForDisplay(state.label, bounds));

    // Circumvent native behaviour of unselecting when container not focused.
    if (MusicPlayer::loaded() && extra_cond())
        ret |= ui::inverted;
    if (state.index == selected)
        ret = std::move(ret) | ui::bold | ui::focus;
    if (state.active)
        ret |= ui::underlined;

    return ui::hbox({ ui::text(
                          [&]
    {
        if (extra_cond())
            return ">";
        if (state.index == selected)
            return "*";
        return " ";
    }()),
                      ui::separatorEmpty(), std::move(ret) });
}
