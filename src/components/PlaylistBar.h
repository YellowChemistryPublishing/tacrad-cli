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
#include <memory>

#include <module/sys>

#include <Config.h>
#include <Exec.inl>
#include <Music.h>
#include <Style.h>
#include <Utility.h>
#include <components/Playlist.h>

namespace ui = ftxui;

class PlaylistBarImpl : public ui::ComponentBase
{
    std::shared_ptr<PlaylistImpl> playlistComp;

    void sortTracks(auto&& reorderTracks)
    {
        const std::vector<MusicPlayer::FoundMusic>& playlist = MusicPlayer::playlistWithTag(this->playlistComp->tagSelectorComp->lookingAtTag);
        const MusicPlayer::FoundMusic toFind =
            this->playlistComp->selected >= 0_i32 && this->playlistComp->selected < playlist.size() ? playlist[sz(this->playlistComp->selected)] : MusicPlayer::FoundMusic {};

        reorderTracks(std::u8string(this->playlistComp->tagSelectorComp->lookingAtTag));

        auto foundIt = std::ranges::find_if(playlist, [&toFind](const MusicPlayer::FoundMusic& v) { return v == toFind; });
        _retif(, foundIt == playlist.end());

        this->playlistComp->currentTrack = MusicPlayer::currentTrack; // Suppress auto-update to playing track, keep selected one highlighted instead.
        this->playlistComp->selected = std::distance(playlist.begin(), foundIt);
    }

    ui::Component containerComp =
        ui::Container::Horizontal(
            { ui::Renderer(ui::filler),
              ui::Button(ui::ButtonOption { .label = UserSettings::ShuffleLabel,
                                            .on_click = [this] { this->sortTracks(MusicPlayer::shufflePlaylist); },
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              hspace(),
              ui::Button(ui::ButtonOption { .label = UserSettings::LexicographicSortLabel,
                                            .on_click = [this] { this->sortTracks(MusicPlayer::sortPlaylistLexicographically); },
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              hspace(),
              ui::Button(ui::ButtonOption { .label = UserSettings::ReverseLexicographicSortLabel,
                                            .on_click = [this] { this->sortTracks(MusicPlayer::sortPlaylistReverseLexicographically); },
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }),
              hspace(),
              ui::Button(ui::ButtonOption { .label = UserSettings::ReloadLabel,
                                            .on_click = MusicPlayer::searchForTracks,
                                            .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(state.label), state); },
                                            .animated_colors {} }) }) |
        hpad;
public:
    explicit PlaylistBarImpl(std::shared_ptr<PlaylistImpl> playlistComp) : playlistComp(std::move(playlistComp)) { this->Add(this->containerComp); }
};

inline ui::Component /* NOLINT(readability-identifier-naming) */ PlaylistBar(std::shared_ptr<PlaylistImpl> playlistComp)
{
    return ui::Make<PlaylistBarImpl>(std::move(playlistComp));
}
