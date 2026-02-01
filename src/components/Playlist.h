#pragma once

#include <Preamble.h>

#include <string>
#include <utility>
#include <vector>

#include <module/sys>

#include <Music.h>

class PlaylistImpl : public ui::ComponentBase
{
    ui::Element postProcessEntry(const ui::EntryState& state)
    {
        ui::Element ret = ui::hbox({ ui::text(
                                         [&]
        {
            if (state.index == MusicPlayer::currentTrack)
                return "> ";
            if (state.active)
                return "* ";
            return "  ";
        }()),
                                     ui::paragraphAlignLeft(state.label) });
        if (state.index == MusicPlayer::currentTrack)
            ret |= ui::inverted;
        if (state.focused)
            ret = ret | ui::underlined;
        if (state.active)
            ret |= ui::bold;
        if (!state.focused && !state.active && state.index != MusicPlayer::currentTrack)
            ret |= ui::dim;
        return ret;
    }
    void onEntryEnter()
    {
        MusicPlayer::currentTrack = sz(this->highlighted);
        (void)MusicPlayer::play();
    }

    std::vector<std::string> trackNames;
    i32 highlighted = 0_i32;
    ui::Component menu = ui::Menu(
        &this->trackNames, &*this->highlighted,
        ui::MenuOption {
            .entries {},
            .underline = ui::UnderlineOption {},
            .entries_option =
                ui::MenuEntryOption { .label = "", .transform = [this](const ui::EntryState& state) -> ui::Element { return this->postProcessEntry(state); }, .animated_colors {} },
            .elements_prefix {},
            .elements_infix {},
            .elements_postfix {},
            .on_change {},
            .on_enter = [this]() -> void { this->onEntryEnter(); }
    });

    [[nodiscard]] ui::Element OnRender() final
    {
        const std::vector<MusicPlayer::FoundMusic>& playlist = MusicPlayer::currentPlaylist();
        while (this->trackNames.size() > playlist.size())
            this->trackNames.pop_back();
        for (sz i = 0_uz; i < this->trackNames.size(); i++)
            this->trackNames[i] = playlist[i].name;
        while (this->trackNames.size() < playlist.size())
        {
            std::string str = playlist[this->trackNames.size()].name;
            this->trackNames.emplace_back(std::move(str));
        }

        return menu->Render() | ui::vscroll_indicator | ui::yframe | ui::yflex;
    }
    [[nodiscard]] bool Focusable() const final { return true; }
public:
    PlaylistImpl()
    {
        if (MusicPlayer::currentPlaylist().empty())
            MusicPlayer::shufflePlaylist();

        this->Add(this->menu);
    }
};

/// @brief Create a playlist component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Playlist() { return ui::Make<PlaylistImpl>(); }
