#pragma once

#include <Preamble.h>

inline ui::Element hpad(ui::Element el) { return ui::hbox({ ui::text(" "), el | ui::flex, ui::text(" ") }); }
