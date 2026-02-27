#pragma once

/// @file TagSelector.h

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <inline/Integer.inl>
#include <map>
#include <string_view>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <Config.h>
#include <Music.h>
#include <Style.h>

namespace ui = ftxui;

/// @brief Displays a banner for playlist selection.
/// @note Pass `byptr`.
class TagSelectorImpl final : public ui::ComponentBase
{
    std::map<sys::str, std::vector<MusicPlayer::Track>> playlists;
    std::vector<sys::str> tagNames;
    sys::str selTag = MusicPlayer::currentTag; // NOLINT(misc-non-private-member-variables-in-classes)
    i32 selTrack = 0_i32;
    ui::Box bounds;

    void syncIfNeeded()
    {
        const std::map<sys::str, std::vector<MusicPlayer::Track>>& freshPlaylists = MusicPlayer::allPlaylists();
        if (this->playlists != freshPlaylists)
        {
            const auto addTag = [this](std::u8string_view tag)
            {
                this->tagNames.emplace_back(tag);
                this->containerComp->Add(ui::MenuEntry(_as(std::string_view, sys::cstr(tag)),
                                                       ui::MenuEntryOption { .transform = [this](const ui::EntryState& state) -> ui::Element
                {
                    return postProcessDisplayListEntry(state, this->selTrack, this->bounds,
                                                       [&] { return state.label == _as(std::string_view, sys::cstr(MusicPlayer::currentTag)); });
                },
                                                                             .animated_colors {} }));
            };

            this->containerComp->DetachAllChildren();
            addTag(u8"all");
            addTag(u8"uncategorized");
            for (const auto& [tag, _] : freshPlaylists)
                if (tag != u8"all" && tag != u8"uncategorized")
                    addTag(tag);

            this->playlists = freshPlaylists;
        }

        if (this->selTrack >= 0_i32 && this->selTrack < this->tagNames.size()) [[likely]]
            this->selTag = this->tagNames[sz(this->selTrack)];
    }

    ui::Component containerComp = ui::Container::Vertical({}, &*this->selTrack);
    ui::Component displayComp = ui::Renderer(this->containerComp, [this]() -> ui::Element
    {
        this->syncIfNeeded();
        return (!this->playlists.empty() ? this->containerComp->Render() : (ui::text(Config::BlankText) | ui::center)) | vscroll(this->bounds);
    });
public:
    TagSelectorImpl() { this->Add(this->displayComp); }

    /// @brief Retrieve the currently selected tag.
    [[nodiscard]] const sys::str& selectedTag() const { return this->selTag; }
};

/// @brief Create a `TagSelectorImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ TagSelector() { return ui::Make<TagSelectorImpl>(); }
