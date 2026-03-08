#pragma once

/// @file

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <memory>
#include <ranges> // NOLINT(misc-include-cleaner)
#include <system_error>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <CmdInv.inl>
#include <Config.h>
#include <Music.h>
#include <Style.h>
#include <components/Console.h>
#include <components/Playlist.h>

namespace ui = ftxui;

/// @brief Displays playlist interation functions.
/// @note Pass `byptr`.
class PlaylistBarImpl final : public ui::ComponentBase
{
    std::shared_ptr<PlaylistImpl> playlistComp;
    std::shared_ptr<ConsoleImpl> consoleComp;

    void arrangeTracks(auto&& reorderTracks)
    {
        const std::vector<MusicPlayer::Track>& playlist = MusicPlayer::playlistWithTag(this->playlistComp->tagSelector()->selectedTag());
        const MusicPlayer::Track toFind = this->playlistComp->selectedTrack() >= 0_i32 && this->playlistComp->selectedTrack() < playlist.size()
            ? playlist[sz(this->playlistComp->selectedTrack())]
            : MusicPlayer::Track {};

        if (!reorderTracks(sys::str(this->playlistComp->tagSelector()->selectedTag()), MusicPlayer::loaded() || this->playlistComp->selectedTrack() != 0_i32))
        {
            CmdInv::println(this->consoleComp->history, "[log.warn] Failed to reorder playlist.");
            return;
        }

        if (MusicPlayer::loaded() || this->playlistComp->selectedTrack() != 0_i32)
        {
            const i32 i = MusicPlayer::indexOf(playlist, toFind);
            this->playlistComp->selectedTrack(i != i32::sentinel() ? i : 0_i32);
        }
        else
            MusicPlayer::currentTrack = 0_i32;
    }
    void searchForTracks()
    {
        const std::vector<std::error_code> errs = MusicPlayer::searchForTracks(MusicPlayer::loaded() || this->playlistComp->selectedTrack() != 0_i32);
        if (!errs.empty())
        {
            sys::cstr log = "[log.warn] Searching for playable music encountered some errors, playlists may be incomplete!";
            for (const std::error_code& ec : errs)
                log.append("\n    ").append(ec.message());
            CmdInv::println(this->consoleComp->history, "{}", log);
        }
    }

    ui::Component containerComp =
        ui::Container::Horizontal(
            { ui::Renderer(ui::filler),
              ui::Button(ui::ButtonOption { .label = UserSettings::ShuffleLabel,
                                            .on_click = [this] { this->arrangeTracks(MusicPlayer::shufflePlaylist); },
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              hspace(),
              ui::Button(ui::ButtonOption { .label = UserSettings::LexicographicSortLabel,
                                            .on_click = [this] { this->arrangeTracks(MusicPlayer::sortPlaylistLexicographically); },
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              hspace(),
              ui::Button(ui::ButtonOption { .label = UserSettings::ReverseLexicographicSortLabel,
                                            .on_click = [this] { this->arrangeTracks(MusicPlayer::sortPlaylistReverseLexicographically); },
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              hspace(),
              ui::Button(ui::ButtonOption { .label = UserSettings::ReloadLabel,
                                            .on_click = [this] { this->searchForTracks(); },
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }) }) |
        hpad;
public:
    /// @internal
    /// @private
    explicit PlaylistBarImpl(std::shared_ptr<PlaylistImpl> playlistComp, std::shared_ptr<ConsoleImpl> console) :
        playlistComp(std::move(playlistComp)), consoleComp(std::move(console))
    {
        this->searchForTracks();
        this->Add(this->containerComp);
    }
};

/// @brief Create a `PlaylistBarImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ PlaylistBar(std::shared_ptr<PlaylistImpl> playlistComp, std::shared_ptr<ConsoleImpl> console)
{
    return ui::Make<PlaylistBarImpl>(std::move(playlistComp), std::move(console));
}
