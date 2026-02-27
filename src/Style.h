#pragma once

/// @file Style.h

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <utility>

#include <module/sys>

#include <Config.h>
#include <Music.h>
#include <Utility.h>

namespace ui = ftxui;

/// @brief Horizontally pad both sides of an element with a space.
inline ui::Element hpad(ui::Element elem) { return ui::hbox({ ui::separatorEmpty(), std::move(elem) | ui::xflex, ui::separatorEmpty() }); }

/// @brief Component displaying only a horizontal space.
inline ui::Component hspace()
{
    return ui::Renderer([] { return ui::separatorEmpty(); });
}

/// @brief A single-space horizontally padded horizontal separator using the user-set style.
inline ui::Element psep() { return ui::hbox({ ui::separatorEmpty(), ui::separatorStyled(UserSettings::border), ui::separatorEmpty() }); }

/// @brief An element bordered with the user-set style.
inline ui::Element bordered(ui::Element elem) { return std::move(elem) | ui::borderStyled(UserSettings::border); }

/// @brief Vertical scrollable region.
/// @param bounds Bounds to reflect region dimensions to.
///
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

/// @brief Post-process an icon button to display interaction state.
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

/// @brief Post-process a list entry to display interaction state.
inline ui::Element postProcessDisplayListEntry(const ui::EntryState& state, i32 selected, const ui::Box& bounds, auto&& extra_cond = [] { return true; })
{
    ui::Element ret = ui::text(truncateStrForDisplay(state.label, bounds));

    // Circumvent native behaviour of unselecting when container not focused.
    if (MusicPlayer::loaded() && MusicPlayer::playing() && extra_cond())
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
