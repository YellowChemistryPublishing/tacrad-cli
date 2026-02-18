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

#include <Config.h>
#include <Music.h>
#include <Style.h>
#include <Utility.h>
#include <components/TagSelector.h>

namespace ui = ftxui;

class PlaylistImpl : public ui::ComponentBase
{
public:
    std::shared_ptr<TagSelectorImpl> tagSelectorComp;
private:
    std::vector<MusicPlayer::FoundMusic> renderedPlaylist;
    std::vector<sys::str> trackNames;
public:
    i32 selected = 0_i32, currentTrack = MusicPlayer::currentTrack; // NOLINT(misc-non-private-member-variables-in-classes)
private:
    ui::Box bounds;

    void onEntryEnter()
    {
        MusicPlayer::playlistTag = this->tagSelectorComp->lookingAtTag;
        MusicPlayer::currentTrack = this->selected;
        if (!MusicPlayer::stopMusic())
            CommandInvocation::println("[log.warn] Failed to stop music.");
        if (!MusicPlayer::play() || !MusicPlayer::resume())
            CommandInvocation::println("[log.error] Failed to play track.");
    }

    void syncIfNeeded()
    {
        const std::vector<MusicPlayer::FoundMusic>& playlist = MusicPlayer::playlistWithTag(this->tagSelectorComp->lookingAtTag);
        if (this->renderedPlaylist != playlist)
        {
            this->containerComp->DetachAllChildren();
            this->trackNames.clear();
            for (const auto& track : playlist)
            {
                this->trackNames.emplace_back(track.name);
                this->containerComp->Add(ui::MenuEntry(_as(std::string_view, sys::cstr(track.name)),
                                                       ui::MenuEntryOption { .transform = [this](const ui::EntryState& state) -> ui::Element
                {
                    return postProcessDisplayListEntry(state, this->selected, this->bounds,
                                                       [&] { return state.index == MusicPlayer::currentTrack && this->tagSelectorComp->lookingAtTag == MusicPlayer::playlistTag; });
                },
                                                                             .animated_colors {} }));
            }

            if (this->selected >= playlist.size())
                this->selected = 0_i32;

            this->renderedPlaylist = playlist;
        }

        if (this->currentTrack != MusicPlayer::currentTrack)
        {
            this->selected = i32(MusicPlayer::currentTrack);
            this->currentTrack = MusicPlayer::currentTrack;
        }
    }

    ui::Component containerComp = ui::Container::Vertical({}, &*this->selected);
    ui::Component displayComp = ui::Renderer(this->containerComp,
                                             [this]() -> ui::Element
    {
        this->syncIfNeeded();
        return (!this->trackNames.empty() ? this->containerComp->Render() : (ui::text(Config::BlankText) | ui::center)) | vscroll(this->bounds);
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
    explicit PlaylistImpl(std::shared_ptr<TagSelectorImpl> tagSelectorComp) : tagSelectorComp(std::move(tagSelectorComp)) { this->Add(this->displayComp); }
};

/// @brief Create a playlist component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Playlist(std::shared_ptr<TagSelectorImpl> tagSelectorComp)
{
    return ui::Make<PlaylistImpl>(std::move(tagSelectorComp));
}
