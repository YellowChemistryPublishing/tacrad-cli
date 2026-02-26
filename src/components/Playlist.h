#pragma once

/// @file Playlist.h

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <CmdInv.inl>
#include <Config.h>
#include <Music.h>
#include <Style.h>
#include <components/Console.h>
#include <components/TagSelector.h>

namespace ui = ftxui;

/// @brief Displays tracks within the currently selected playlist.
/// @note Pass `byptr`.
class PlaylistImpl final : public ui::ComponentBase
{
    std::shared_ptr<TagSelectorImpl> tagSelComp;
    std::shared_ptr<ConsoleImpl> consoleComp;

    std::vector<MusicPlayer::Track> renderedPlaylist;
    i32 selected = 0_i32, curTrack = MusicPlayer::currentTrack;
    ui::Box bounds;

    void onEntryEnter()
    {
        MusicPlayer::currentTag = this->tagSelComp->selectedTag();
        MusicPlayer::currentTrack = this->selected;

        _retif(CmdInv::println(this->consoleComp->history, "[log.warn] Failed to stop music. ({})", _as(std::string_view, stopRes.err())), auto stopRes = MusicPlayer::stopMusic();
               !stopRes);

        auto playRes = MusicPlayer::play();
        if (!playRes || !(playRes = MusicPlayer::resume())) // NOLINT(bugprone-assignment-in-if-condition)
            CmdInv::println(this->consoleComp->history, "[log.error] Failed to play track. ({})", _as(std::string_view, playRes.err()));
    }

    void syncIfNeeded()
    {
        const std::vector<MusicPlayer::Track>& playlist = MusicPlayer::playlistWithTag(this->tagSelComp->selectedTag());
        if (this->renderedPlaylist != playlist)
        {
            this->containerComp->DetachAllChildren();
            for (const auto& track : playlist)
            {
                this->containerComp->Add(ui::MenuEntry(_as(std::string_view, track.fullDisplayTitle()),
                                                       ui::MenuEntryOption { .transform = [this](const ui::EntryState& state) -> ui::Element
                {
                    return postProcessDisplayListEntry(state, this->selected, this->bounds,
                                                       [&] { return state.index == MusicPlayer::currentTrack && this->tagSelComp->selectedTag() == MusicPlayer::currentTag; });
                },
                                                                             .animated_colors {} }));
            }

            if (this->selected >= playlist.size())
                this->selected = 0_i32;

            this->renderedPlaylist = playlist;
        }

        if (this->curTrack != MusicPlayer::currentTrack && this->tagSelComp->selectedTag() == MusicPlayer::currentTag)
        {
            this->selected = i32(MusicPlayer::currentTrack);
            this->curTrack = MusicPlayer::currentTrack;
        }
    }

    ui::Component containerComp = ui::Container::Vertical({}, &*this->selected);
    ui::Component displayComp = ui::Renderer(this->containerComp,
                                             [this]() -> ui::Element
    {
        this->syncIfNeeded();
        return (!this->renderedPlaylist.empty() ? this->containerComp->Render() : (ui::text(Config::BlankText) | ui::center)) | vscroll(this->bounds);
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
    /// @internal
    /// @private
    explicit PlaylistImpl(std::shared_ptr<TagSelectorImpl> tagSelComp, std::shared_ptr<ConsoleImpl> consoleComp) :
        tagSelComp(std::move(tagSelComp)), consoleComp(std::move(consoleComp))
    {
        this->Add(this->displayComp);
    }

    /// @brief Retrieve the currently selected track.
    [[nodiscard]] i32 selectedTrack() const { return this->selected; }
    /// @brief Set the currently selected track.
    void selectedTrack(const i32 val) { this->selected = val; }
    /// @brief Retrieve the currently playing track.
    [[nodiscard]] i32 currentTrack() const { return this->curTrack; }
    /// @brief Set the currently playing track.
    void currentTrack(const i32 val) { this->curTrack = val; }

    /// @brief Retrieve the associated tag selector component.
    [[nodiscard]] std::shared_ptr<TagSelectorImpl> tagSelector() const { return this->tagSelComp; }
};

/// @brief Create a `PlaylistImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Playlist(std::shared_ptr<TagSelectorImpl> tagSelComp, std::shared_ptr<ConsoleImpl> consoleComp)
{
    return ui::Make<PlaylistImpl>(std::move(tagSelComp), std::move(consoleComp));
}
