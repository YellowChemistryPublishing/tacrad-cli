#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/direction.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <utility>

#include <module/sys>

#include <Config.h>
#include <Style.h>
#include <components/Console.h>
#include <components/Details.h>
#include <components/Playlist.h>
#include <components/PlaylistBar.h>
#include <components/StatusBar.h>
#include <components/TagSelector.h>

namespace ui = ftxui;

/// @brief Displays the primary view for playing music.
/// @note Pass `byptr`.
class UIImpl final : public ui::ComponentBase
{
    i32 leftSize = UserSettings::TagSelectPanelInitWidth;
    i32 rightSize = UserSettings::DetailsPanelInitWidth;

    std::shared_ptr<TagSelectorImpl> tagListComp = std::static_pointer_cast<TagSelectorImpl>(TagSelector());
    std::shared_ptr<PlaylistImpl> playlistComp;
    ui::Component playlistPanelComp = ui::Container::Vertical({ playlistComp | ui::yflex, PlaylistBar(playlistComp) });
    ui::Component trackDetailsComp = Details(tagListComp, playlistComp);

    std::shared_ptr<StatusBarImpl> statBarComp;

    ui::Component containerComp = ui::Container::Vertical(
        { ui::ResizableSplit({ .main = trackDetailsComp,
                               .back = ui::ResizableSplit(
                                   { .main = tagListComp, .back = playlistPanelComp, .direction = ui::Direction::Left, .main_size = &*this->leftSize, .separator_func = psep }),
                               .direction = ui::Direction::Right,
                               .main_size = &*this->rightSize,
                               .separator_func = psep }) |
              ui::yflex,
          ui::Renderer([] { return ui::separatorStyled(UserSettings::border); }), this->statBarComp });
public:
    explicit UIImpl(std::shared_ptr<ConsoleImpl> consoleComp) :
        playlistComp(std::static_pointer_cast<PlaylistImpl>(Playlist(tagListComp, consoleComp))), statBarComp(std::static_pointer_cast<StatusBarImpl>(StatusBar(consoleComp)))
    {
        this->Add(this->containerComp);
    }

    std::shared_ptr<StatusBarImpl> statusBar() { return this->statBarComp; }
};

/// @brief Create a `UIImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ UI(std::shared_ptr<ConsoleImpl> consoleComp) { return ui::Make<UIImpl>(std::move(consoleComp)); }
