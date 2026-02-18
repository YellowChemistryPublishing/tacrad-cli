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
#include <Utility.h>

namespace ui = ftxui;

class TagSelectorImpl : public ui::ComponentBase
{
    std::map<std::u8string, std::vector<MusicPlayer::FoundMusic>> playlists;
    std::vector<sys::str> tagNames;
    i32 selected = 0_i32;
    ui::Box bounds;
public:
    sys::str lookingAtTag = MusicPlayer::playlistTag; // NOLINT(misc-non-private-member-variables-in-classes)
private:
    void syncIfNeeded()
    {
        const std::map<std::u8string, std::vector<MusicPlayer::FoundMusic>>& freshPlaylists = MusicPlayer::allPlaylists();
        if (this->playlists != freshPlaylists)
        {
            this->containerComp->DetachAllChildren();

            const auto addTag = [this](std::u8string_view tag)
            {
                this->tagNames.emplace_back(tag);
                this->containerComp->Add(ui::MenuEntry(_as(std::string_view, sys::cstr(tag)),
                                                       ui::MenuEntryOption { .transform = [this](const ui::EntryState& state) -> ui::Element
                {
                    return postProcessDisplayListEntry(state, this->selected, this->bounds,
                                                       [&state] { return state.label == _as(std::string_view, sys::cstr(MusicPlayer::playlistTag)); });
                },
                                                                             .animated_colors {} }));
            };

            addTag(u8"all");
            addTag(u8"uncategorized");
            for (const auto& [tag, _] : freshPlaylists)
                if (tag != u8"all" && tag != u8"uncategorized")
                    addTag(tag);

            this->playlists = freshPlaylists;
        }

        if (this->selected >= 0_i32 && this->selected < this->tagNames.size()) [[likely]]
            this->lookingAtTag = this->tagNames[sz(this->selected)];
    }

    ui::Component containerComp = ui::Container::Vertical({}, &*this->selected);
    ui::Component displayComp = ui::Renderer(this->containerComp, [this]() -> ui::Element
    {
        this->syncIfNeeded();
        return (!this->playlists.empty() ? this->containerComp->Render() : (ui::text(Config::BlankText) | ui::center)) | vscroll(this->bounds);
    });
public:
    TagSelectorImpl()
    {
        MusicPlayer::searchForTracks();
        this->Add(this->displayComp);
    }
};

/// @brief Create a tag selector component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ TagSelector() { return ui::Make<TagSelectorImpl>(); }
