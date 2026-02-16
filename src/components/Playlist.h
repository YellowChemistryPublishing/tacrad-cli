#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <Music.h>
#include <Style.h>

namespace ui = ftxui;

class PlaylistImpl : public ui::ComponentBase
{
    std::vector<MusicPlayer::FoundMusic> renderedPlaylist;
    std::vector<sys::str> trackNames;
    i32 selected = 0_i32;

    ui::Element postProcessEntry(ui::EntryState state)
    {
        ui::Element ret = ui::text(state.label);

        if (state.index == MusicPlayer::currentTrack) // Circumvent native behaviour of unselecting when playlist not focused.
            ret |= ui::inverted;
        else if (state.index == this->selected)
            ret = std::move(ret) | ui::bold | ui::focus;

        if (state.active)
            ret |= ui::underlined;

        return ui::hbox({ ui::text(
                              [&]
        {
            if (state.index == MusicPlayer::currentTrack)
                return ">";
            if (state.index == this->selected)
                return "*";
            return " ";
        }()),
                          ui::separatorEmpty(), std::move(ret) });
    }
    void onEntryEnter()
    {
        MusicPlayer::currentTrack = this->selected;
        (void)MusicPlayer::play();
    }

    void syncIfNeeded()
    {
        const std::vector<MusicPlayer::FoundMusic>& playlist = MusicPlayer::currentPlaylist();
        if (this->renderedPlaylist != playlist)
        {
            this->containerComp->DetachAllChildren();
            this->trackNames.clear();
            for (const auto& track : playlist)
            {
                this->trackNames.emplace_back(track.name);
                this->containerComp->Add(ui::MenuEntry(
                    _as(std::string_view, sys::cstr(track.name)),
                    ui::MenuEntryOption { .transform = [this](const ui::EntryState& state) -> ui::Element { return this->postProcessEntry(state); }, .animated_colors {} }));
            }

            this->renderedPlaylist = playlist;
        }

        if (this->currentTrackOld != MusicPlayer::currentTrack)
        {
            this->selected = i32(MusicPlayer::currentTrack);
            this->currentTrackOld = MusicPlayer::currentTrack;
        }
    }

    i32 currentTrackOld = MusicPlayer::currentTrack;
    ui::Box bounds;
    ui::Component containerComp = ui::Container::Vertical({}, &*this->selected);
    ui::Component displayComp = ui::Renderer(this->containerComp,
                                             [this]() -> ui::Element
    {
        this->syncIfNeeded();
        return this->containerComp->Render() | vscroll(this->bounds);
    }) |
        ui::CatchEvent([this](const ui::Event& event) -> bool
    {
        if (event == ui::Event::Return)
        {
            this->onEntryEnter();
            return true;
        }

        return false;
    });
public:
    PlaylistImpl()
    {
        if (MusicPlayer::currentPlaylist().empty())
            MusicPlayer::generateShuffledPlaylist();

        this->Add(this->displayComp);
    }
};

/// @brief Create a playlist component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Playlist() { return ui::Make<PlaylistImpl>(); }
