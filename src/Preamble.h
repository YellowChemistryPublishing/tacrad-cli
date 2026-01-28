#pragma once

/// @file Preamble.h
/// @brief Include-before-all-else header.

#include <ftxui/component/animation.hpp>          // IWYU pragma: export
#include <ftxui/component/captured_mouse.hpp>     // IWYU pragma: export
#include <ftxui/component/component.hpp>          // IWYU pragma: export
#include <ftxui/component/component_base.hpp>     // IWYU pragma: export
#include <ftxui/component/component_options.hpp>  // IWYU pragma: export
#include <ftxui/component/event.hpp>              // IWYU pragma: export
#include <ftxui/component/loop.hpp>               // IWYU pragma: export
#include <ftxui/component/mouse.hpp>              // IWYU pragma: export
#include <ftxui/component/receiver.hpp>           // IWYU pragma: export
#include <ftxui/component/screen_interactive.hpp> // IWYU pragma: export
#include <ftxui/component/task.hpp>               // IWYU pragma: export
#include <ftxui/dom/canvas.hpp>                   // IWYU pragma: export
#include <ftxui/dom/deprecated.hpp>               // IWYU pragma: export
#include <ftxui/dom/direction.hpp>                // IWYU pragma: export
#include <ftxui/dom/elements.hpp>                 // IWYU pragma: export
#include <ftxui/dom/flexbox_config.hpp>           // IWYU pragma: export
#include <ftxui/dom/linear_gradient.hpp>          // IWYU pragma: export
#include <ftxui/dom/node.hpp>                     // IWYU pragma: export
#include <ftxui/dom/requirement.hpp>              // IWYU pragma: export
#include <ftxui/dom/selection.hpp>                // IWYU pragma: export
#include <ftxui/dom/table.hpp>                    // IWYU pragma: export
#include <ftxui/dom/take_any_args.hpp>            // IWYU pragma: export
#include <ftxui/screen/box.hpp>                   // IWYU pragma: export
#include <ftxui/screen/color.hpp>                 // IWYU pragma: export
#include <ftxui/screen/color_info.hpp>            // IWYU pragma: export
#include <ftxui/screen/deprecated.hpp>            // IWYU pragma: export
#include <ftxui/screen/image.hpp>                 // IWYU pragma: export
#include <ftxui/screen/pixel.hpp>                 // IWYU pragma: export
#include <ftxui/screen/screen.hpp>                // IWYU pragma: export
#include <ftxui/screen/string.hpp>                // IWYU pragma: export
#include <ftxui/screen/terminal.hpp>              // IWYU pragma: export
#include <ftxui/util/autoreset.hpp>               // IWYU pragma: export
#include <ftxui/util/ref.hpp>                     // IWYU pragma: export

namespace ui = ftxui; // NOLINT(misc-unused-alias-decls)
