#pragma once

#include <Preamble.h>

#include <Config.h>

inline ui::Element hpad(ui::Element el) { return ui::hbox({ ui::separatorEmpty(), el | ui::flex, ui::separatorEmpty() }); }
inline ui::Element psep() { return ui::hbox({ ui::separatorEmpty(), ui::separatorStyled(UserSettings::border), ui::separatorEmpty() }); }
