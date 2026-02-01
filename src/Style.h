#pragma once

#include <Preamble.h>

inline ui::Element hpad(ui::Element el) { return ui::hbox({ ui::separatorEmpty(), el | ui::flex, ui::separatorEmpty() }); }
