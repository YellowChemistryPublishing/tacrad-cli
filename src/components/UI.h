#pragma once

#include <Preamble.h>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>

#include <module/sys>

#include <Config.h>
#include <Style.h>
#include <components/Playlist.h>
#include <components/StatusBar.h>
#include <components/Terminal.h>

class UIImpl : public ui::ComponentBase, public std::enable_shared_from_this<UIImpl>
{
    i32 leftSize = 20_i32;  // NOLINT(readability-magic-numbers)
    i32 rightSize = 32_i32; // NOLINT(readability-magic-numbers)

    ui::Component tagSelectComp = ui::Renderer([] { return ui::text("> all") | ui::bold; });
    ui::Component playlistComp = Playlist();
    ui::Component detailsComp = ui::Renderer([] { return ui::text("details here"); });

    std::shared_ptr<StatusBarImpl> statBarComp = std::static_pointer_cast<StatusBarImpl>(StatusBar());
    ui::Component containerComp = ui::Container::Vertical({ [this]
    {
        ui::Component ret =
            ui::ResizableSplit({ .main = tagSelectComp, .back = playlistComp, .direction = ui::Direction::Left, .main_size = &*this->leftSize, .separator_func = psep });
        ret = ui::ResizableSplit({ .main = detailsComp, .back = std::move(ret), .direction = ui::Direction::Right, .main_size = &*this->rightSize, .separator_func = psep });
        return ret;
    }(), this->statBarComp }) |
        ui::Renderer([](ui::Element elem) { return std::move(elem) | ui::yflex; });
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
