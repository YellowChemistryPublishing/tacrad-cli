#pragma once

/// @file

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <memory>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <Config.h>
#include <Music.h>
#include <components/Playlist.h>
#include <components/TagSelector.h>

namespace ui = ftxui;

/// @brief Displays information about the selected track.
/// @note Pass `byptr`.
class DetailsImpl final : public ui::ComponentBase
{
    std::shared_ptr<TagSelectorImpl> tagSelectorComp;
    std::shared_ptr<PlaylistImpl> playlistComp;
    sys::str selectedTag;
    i32 selectedTrack = 0_i32;
    ui::Element cachedElement = ui::text(Config::BlankText);

    ui::Component displayComp = ui::Renderer([this]() -> ui::Element
    {
        if (this->selectedTag == this->tagSelectorComp->selectedTag() && this->selectedTrack == this->playlistComp->selectedTrack()) [[likely]]
            return this->cachedElement;

        const std::vector<MusicPlayer::Track>& playlist = MusicPlayer::playlistWithTag(sys::str(this->tagSelectorComp->selectedTag()));
        _retif(this->cachedElement = ui::text(Config::BlankText), this->playlistComp->selectedTrack() < 0_i32 || this->playlistComp->selectedTrack() >= playlist.size());

        const MusicPlayer::Track& track = playlist[sz(this->playlistComp->selectedTrack())];

        this->selectedTag = this->tagSelectorComp->selectedTag();
        this->selectedTrack = this->playlistComp->selectedTrack();

        return this->cachedElement = ui::vbox(
                   { ui::paragraphAlignLeft(_as(std::string_view, track.titleDisplay)) | ui::bold | ui::underlined,
                     !track.subtitleDisplay.empty() ? (ui::paragraphAlignLeft(_as(std::string_view, track.subtitleDisplay)) | ui::bold) : ui::emptyElement(),
                     ui::paragraphAlignLeft(_as(std::string_view, track.artistsDisplay)) | ui::color(UserSettings::FlavorDescriptionColor), ui::separatorEmpty(),
                     ui::hbox({ ui::text("tags:"), ui::separatorEmpty(),
                                ui::paragraphAlignLeft(_as(std::string_view, track.tagsDisplay)) | ui::color(UserSettings::FlavorDescriptionColor) }),
                     !track.akaDisplay.empty() ? ui::hbox({ ui::text("aka."), ui::separatorEmpty(),
                                                            ui::paragraphAlignLeft(_as(std::string_view, track.akaDisplay)) | ui::color(UserSettings::FlavorDescriptionColor) })
                                               : ui::emptyElement(),
                     ui::filler() });
    });
public:
    /// @internal
    /// @private
    DetailsImpl(std::shared_ptr<TagSelectorImpl> tagSelectorComp, std::shared_ptr<PlaylistImpl> playlistComp) :
        tagSelectorComp(std::move(tagSelectorComp)), playlistComp(std::move(playlistComp))
    {
        this->Add(this->displayComp);
    }
};

/// @brief Create a `DetailsImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Details(std::shared_ptr<TagSelectorImpl> tagSelectorComp, std::shared_ptr<PlaylistImpl> playlistComp)
{
    return ui::Make<DetailsImpl>(std::move(tagSelectorComp), std::move(playlistComp));
}
