#pragma once

#include "ftxui/component/mouse.hpp"
#include <Preamble.h>

#include <cctype>
#include <chrono>
#include <condition_variable>
#include <format>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>

#include <module/sys>

#include <CmdInv.h>
#include <Config.h>
#include <Screen.h>
#include <components/StatusBar.h>

/// @brief Terminal input component with blinking cursor.
/// @note Pass `byptr`.
class TerminalImpl : public ui::ComponentBase, public std::enable_shared_from_this<TerminalImpl>
{
    std::string cmd;

    std::chrono::steady_clock::time_point clearAfter = std::chrono::steady_clock::time_point::max();
    std::mutex clearAfterLock;
    std::condition_variable_any clearAfterCv;
    std::jthread clearAfterThread { [this](std::stop_token token)
    {
        while (!token.stop_requested())
        {
            {
                std::unique_lock guard(this->clearAfterLock);
                this->clearAfterCv.wait(guard, token, [this]() { return this->clearAfter != std::chrono::steady_clock::time_point::max(); });
                if (token.stop_requested())
                    break;

                this->clearAfterCv.wait_until(guard, token, this->clearAfter, [this]() { return std::chrono::steady_clock::now() >= this->clearAfter; });
                if (token.stop_requested())
                    break;

                this->clearAfter = std::chrono::steady_clock::time_point::max();
            }

            this->screen.Post([this]()
            {
                if (!this->cmd.starts_with(Config::QuickActionKey) || this->cmd.size() <= 1uz)
                    return;

                this->cmd = Config::QuickActionKey;
                this->screen.PostEvent(ui::Event::Custom);
            });
        }
    } };

    /// @brief Let the currently typed quick action live until one second from now.
    void resetQuickActionTimeout()
    {
        const std::unique_lock guard(this->clearAfterLock);
        this->clearAfter = std::chrono::steady_clock::now() + Config::QuickActionDelay;
        this->clearAfterCv.notify_one();
    }

    /// @brief Post-process an input element.
    static ui::Element postProcessInput(ui::InputState state)
    {
        if (state.focused)
        {
            state.element |= ui::focus;
            if (!state.is_placeholder)
                state.element |= ui::focusCursorBlockBlinking;
        }

        if (state.is_placeholder)
            state.element = ui::hbox({ std::move(state.element) | ui::flex, ui::text(std::format("{} {}", Config::ApplicationName, Config::VersionIdentifier)) }) |
                ui::color(UserSettings::FlavorUnemphasizedColor) | ui::flex | (state.hovered || state.focused ? ui::underlined : ui::nothing);
        else
            state.element = ui::hbox({ std::move(state.element), ui::filler() });

        return state.element;
    }
    /// @brief Handle character input.
    void onInputChange()
    {
        if (!this->cmd.starts_with(Config::QuickActionKey))
            return;

        this->resetQuickActionTimeout();
        if (std::isspace(this->cmd.back()) || (this->cmd.size() > 1uz && this->cmd.back() == Config::QuickActionKey))
        {
            this->cmd.pop_back();
            return;
        }

        if (CommandProcessor::quickAction(this->cmd))
            this->cmd.clear();
    }
    /// @brief Handle enter key press.
    void onInputEnter()
    {
        _retif(, this->cmd.empty());

        CommandProcessor::command(this->cmd, this->statusBar);
        this->cmd.clear();
    }

    ui::ScreenInteractive& screen = Screen();
    std::weak_ptr<StatusBarImpl> statusBar;

    ui::Box bounds;
    ui::Component inputComp = ui::Input(ui::InputOption { .content = &this->cmd,
                                                          .placeholder = std::format("{} for quick action...", Config::QuickActionKey),
                                                          .transform = TerminalImpl::postProcessInput,
                                                          .multiline = false,
                                                          .on_change = [this]() -> void { this->onInputChange(); },
                                                          .on_enter = [this]() -> void { this->onInputEnter(); } }) |
        ui::Renderer([this](ui::Element elem) { return std::move(elem) | ui::reflect(this->bounds); }) |
        ui::CatchEvent([this](ui::Event event)
    {
        if (event.is_mouse() && event.mouse().button == ui::Mouse::Left && this->bounds.Contain(event.mouse().x, event.mouse().y))
        {
            this->TakeFocus();
            return true;
        }

        return false;
    });
public:
    explicit TerminalImpl(std::weak_ptr<StatusBarImpl> statusBar) : statusBar(std::move(statusBar)) { this->Add(this->inputComp); }

    TerminalImpl(const TerminalImpl&) = delete;
    TerminalImpl(TerminalImpl&&) = delete;
    ~TerminalImpl() override = default;

    TerminalImpl& operator=(const TerminalImpl&) = delete;
    TerminalImpl& operator=(TerminalImpl&&) = delete;
};

/// @brief Create a terminal component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Terminal(std::weak_ptr<StatusBarImpl> statusBar) { return ui::Make<TerminalImpl>(statusBar); }

inline ui::ComponentDecorator /* NOLINT(readability-identifier-naming) */ TerminalSpaceToFocusHandler(const ui::Component& terminal)
{
    return ui::CatchEvent([&](const ui::Event& event)
    {
        if (!terminal->Focused() && event == ui::Event::Character(' '))
        {
            terminal->TakeFocus(); // Focus terminal, on spacebar pressed.
            return true;
        }

        return false;
    });
}

inline ui::ComponentDecorator /* NOLINT(readability-identifier-naming) */ TerminalQuickActionHandler(const ui::Component& terminal)
{
    return ui::CatchEvent([&](const ui::Event& event)
    {
        if (event == ui::Event::Character(Config::QuickActionKey))
            terminal->TakeFocus(); // Focus terminal, when user types quick action key.

        return false;
    });
}
