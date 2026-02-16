#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>

#include <Config.h>

namespace ui = ftxui;

inline ui::Element hpad(ui::Element el) { return ui::hbox({ ui::separatorEmpty(), el | ui::flex, ui::separatorEmpty() }); }
inline ui::Element psep() { return ui::hbox({ ui::separatorEmpty(), ui::separatorStyled(UserSettings::border), ui::separatorEmpty() }); }
