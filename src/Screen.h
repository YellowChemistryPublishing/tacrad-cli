#pragma once

#include <Preamble.h>

/// @brief Obtain the global fullscreen interactive screen.
inline ui::ScreenInteractive& Screen /* NOLINT(readability-identifier-naming) */ ()
{
    static ui::ScreenInteractive screen = ui::ScreenInteractive::Fullscreen();
    return screen;
}
