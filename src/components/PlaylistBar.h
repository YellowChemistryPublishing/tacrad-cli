#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/util/ref.hpp>

#include <module/sys>

#include <Config.h>
#include <Exec.inl>
#include <Music.h>
#include <Style.h>
#include <Utility.h>

namespace ui = ftxui;

class PlaylistBarImpl : public ui::ComponentBase
{
    ui::Component containerComp =
        ui::Container::Horizontal(
            { ui::Renderer(ui::filler),
              ui::Button(ui::ButtonOption { .label = UserSettings::ShuffleLabel,
                                            .on_click = MusicPlayer::shuffleCurrentPlaylist,
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              ui::Renderer(ui::separatorEmpty),
              ui::Button(ui::ButtonOption { .label = UserSettings::LexicographicSortLabel,
                                            .on_click = MusicPlayer::sortCurrentPlaylistLexicographically,
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              ui::Renderer(ui::separatorEmpty),
              ui::Button(ui::ButtonOption { .label = UserSettings::ReverseLexicographicSortLabel,
                                            .on_click = MusicPlayer::sortCurrentPlaylistReverseLexicographically,
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              ui::Renderer(ui::separatorEmpty),
              ui::Button(ui::ButtonOption { .label = UserSettings::ReloadLabel,
                                            .on_click = MusicPlayer::searchForTracks,
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }) }) |
        hpad;
public:
    explicit PlaylistBarImpl() { this->Add(this->containerComp); }
};

inline ui::Component /* NOLINT(readability-identifier-naming) */ PlaylistBar() { return ui::Make<PlaylistBarImpl>(); }
