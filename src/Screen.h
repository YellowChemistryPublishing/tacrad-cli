#pragma once

#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace ui = ftxui;

/// @brief Obtain the global fullscreen interactive screen.
inline ui::ScreenInteractive& /* NOLINT(readability-identifier-naming) */ Screen()
{
    static ui::ScreenInteractive screen = ui::ScreenInteractive::Fullscreen();
    return screen;
}
