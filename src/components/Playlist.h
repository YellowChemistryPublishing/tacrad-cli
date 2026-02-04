#pragma once

#include <Preamble.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <module/sys>

#include <Music.h>

class PlaylistImpl : public ui::ComponentBase, public std::enable_shared_from_this<PlaylistImpl>
{
    std::vector<std::string> trackNames;
    std::vector<ui::Box> itemBounds;
    i32 hovered = 0_i32, highlighted = 0_i32;

    ui::Element postProcessEntry(const ui::EntryState& state)
    {
        if (this->itemBounds.size() < this->trackNames.size())
        {
            sz cap = 32_uz; // NOLINT(readability-magic-numbers)
            while ((cap = std::max(cap, sz(this->itemBounds.capacity()))) < this->trackNames.size())
                cap *= 2_uz;
            this->itemBounds.reserve(cap);
            this->itemBounds.resize(this->trackNames.size());
        }

        ui::Element ret = ui::paragraphAlignLeft(state.label);

        if (state.index == MusicPlayer::currentTrack)
            ret |= ui::inverted;
        else if (state.index == this->hovered)
            ret |= ui::underlined;

        if (state.active)
            ret |= ui::bold;
        else if (state.index != this->hovered && state.index != MusicPlayer::currentTrack)
            ret |= ui::dim;

        return ui::hbox({ ui::text(
                              [&]
        {
            if (state.index == MusicPlayer::currentTrack)
                return "> ";
            if (state.active)
                return "* ";
            return "  ";
        }()),
                          ret }) |
            ui::reflect(this->itemBounds[sz(state.index)]);
    }
    void onEntryEnter()
    {
        MusicPlayer::currentTrack = this->highlighted;
        (void)MusicPlayer::play();
    }

    ui::Component menuComp =
        ui::Menu(&this->trackNames, &*this->highlighted,
                 ui::MenuOption {
                     .entries {},
                     .underline = ui::UnderlineOption {},
                     .entries_option = ui::MenuEntryOption { .label = "",
                               .transform = [this](const ui::EntryState& state) -> ui::Element { return this->postProcessEntry(state); },
                               .animated_colors {} },
                     .elements_prefix {},
                     .elements_infix {},
                     .elements_postfix {},
                     .on_change {},
                     .on_enter = [this] { this->onEntryEnter(); }
    }) |
        ui::CatchEvent([this](ui::Event event) -> bool
    {
        if (this->itemBounds.size() > this->trackNames.size())
            this->itemBounds.erase(this->itemBounds.begin() + ssz(this->trackNames.size()));

        if (!event.is_mouse() || !this->bounds.Contain(event.mouse().x, event.mouse().y))
            return false;

        if (event.mouse().motion != ui::Mouse::Moved && (event.mouse().button != ui::Mouse::Left || event.mouse().motion != ui::Mouse::Pressed))
            return false;

        const ssz foundIndex =
            std::distance(this->itemBounds.begin(), std::ranges::find_if(this->itemBounds, [&](const ui::Box& box) { return box.Contain(event.mouse().x, event.mouse().y); }));
        if (foundIndex >= this->itemBounds.size())
            return false;

        if (event.mouse().motion == ui::Mouse::Moved)
            this->hovered = i32(foundIndex);
        else
        {
            this->highlighted = i32(foundIndex);
            this->menuComp->TakeFocus();
        }

        return true;
    });

    i32 currentTrackOld = MusicPlayer::currentTrack;
    ui::Box bounds;
    ui::Component displayComp = ui::Renderer(this->menuComp, [this]
    {
        if (this->currentTrackOld != MusicPlayer::currentTrack)
        {
            this->highlighted = i32(MusicPlayer::currentTrack);
            this->currentTrackOld = MusicPlayer::currentTrack;
        }

        const std::vector<MusicPlayer::FoundMusic>& playlist = MusicPlayer::currentPlaylist();
        if (this->trackNames.size() > playlist.size())
            this->trackNames.erase(this->trackNames.begin() + *ssz(playlist.size()));
        for (sz i = 0_uz; i < this->trackNames.size(); i++)
            this->trackNames[i] = playlist[i].name;
        while (this->trackNames.size() < playlist.size())
        {
            std::string str = playlist[this->trackNames.size()].name;
            this->trackNames.emplace_back(std::move(str));
        }

        return this->menuComp->Render() | ui::vscroll_indicator | ui::yframe | ui::yflex | ui::reflect(this->bounds);
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
