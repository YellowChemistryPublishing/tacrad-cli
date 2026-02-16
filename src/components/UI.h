#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/direction.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>

#include <module/sys>

#include <Config.h>
#include <Style.h>
#include <components/Playlist.h>
#include <components/StatusBar.h>
#include <components/TagSelector.h>

namespace ui = ftxui;

class UIImpl : public ui::ComponentBase
{
    i32 leftSize = 20_i32;  // NOLINT(readability-magic-numbers)
    i32 rightSize = 32_i32; // NOLINT(readability-magic-numbers)

    std::shared_ptr<TagSelectorImpl> tagListComp = std::static_pointer_cast<TagSelectorImpl>(TagSelector());
    ui::Component playlistComp = Playlist(tagListComp);
    ui::Component trackInfoComp = ui::Renderer([] { return ui::text("details here"); });

    std::shared_ptr<StatusBarImpl> statBarComp = std::static_pointer_cast<StatusBarImpl>(StatusBar());

    ui::Component containerComp = ui::Container::Vertical(
        { ui::ResizableSplit(
              { .main = trackInfoComp,
                .back = ui::ResizableSplit({ .main = tagListComp, .back = playlistComp, .direction = ui::Direction::Left, .main_size = &*this->leftSize, .separator_func = psep }),
                .direction = ui::Direction::Right,
                .main_size = &*this->rightSize,
                .separator_func = psep }) |
              ui::yflex,
          ui::Renderer([] { return ui::separatorStyled(UserSettings::border); }), this->statBarComp });
public:
    explicit UIImpl() { this->Add(this->containerComp); }

    UIImpl(const UIImpl&) = delete;
    UIImpl(UIImpl&&) = delete;
    ~UIImpl() override = default;

    UIImpl& operator=(const UIImpl&) = delete;
    UIImpl& operator=(UIImpl&&) = delete;

    std::shared_ptr<StatusBarImpl> statusBar() { return this->statBarComp; }
};

inline ui::Component /* NOLINT(readability-identifier-naming) */ UI() { return ui::Make<UIImpl>(); }
