#pragma once

#include <cctype>
#include <chrono>
#include <condition_variable>
#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/color.hpp>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>

#include <module/sys>

#include <CmdProc.h>
#include <Config.h>
#include <Screen.h>
#include <components/Console.h>
#include <components/StatusBar.h>

namespace ui = ftxui;

/// @brief Displays command input bar with blinking cursor.
/// @note Pass `byptr`.
class TerminalImpl final : public ui::ComponentBase
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

            Screen().Post([this]()
            {
                if (!this->cmd.starts_with(Config::QuickActionKey) || this->cmd.size() <= 1uz)
                    return;

                this->cmd.clear();
                this->cmd.push_back(Config::QuickActionKey);
                Screen().PostEvent(ui::Event::Custom);
            });
        }
    } };

    void resetQuickActionTimeout()
    {
        const std::unique_lock guard(this->clearAfterLock);
        this->clearAfter = std::chrono::steady_clock::now() + Config::QuickActionDelay;
        this->clearAfterCv.notify_one();
    }

    static ui::Element postProcessInput(ui::InputState state)
    {
        if (state.focused)
        {
            state.element |= ui::focus;
            if (!state.is_placeholder)
                state.element |= ui::focusCursorBlockBlinking;
        }

        if (state.is_placeholder)
            state.element = ui::hbox({ std::move(state.element) | ui::xflex, ui::text(std::format("{} {}", Config::ApplicationName, Config::VersionIdentifier)) }) |
                ui::color(UserSettings::FlavorUnemphasizedColor) | ui::xflex | (state.hovered || state.focused ? ui::underlined : ui::nothing);
        else
            state.element = ui::hbox({ std::move(state.element), ui::filler() });

        return state.element;
    }
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

        if (CmdProc::invokeQuickAction(this->consoleComp->history, this->cmd))
            this->cmd.clear();
    }
    void onInputEnter()
    {
        _retif(, this->cmd.empty());

        CmdProc::invoke(this->consoleComp->history, this->cmd, this->statusBar);
        this->cmd.clear();
    }

    std::shared_ptr<ConsoleImpl> consoleComp;
    std::weak_ptr<StatusBarImpl> statusBar;

    ui::Box bounds;
    ui::Component inputComp = ui::Input(ui::InputOption { .content = &this->cmd,
                                                          .placeholder = std::format("{} for quick action...", Config::QuickActionKey),
                                                          .transform = TerminalImpl::postProcessInput,
                                                          .multiline = false,
                                                          .on_change = [this]() -> void { this->onInputChange(); },
                                                          .on_enter = [this]() -> void { this->onInputEnter(); } }) |
        ui::reflect(this->bounds) |
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
    explicit TerminalImpl(std::shared_ptr<ConsoleImpl> consoleComp, std::weak_ptr<StatusBarImpl> statusBar) : consoleComp(std::move(consoleComp)), statusBar(std::move(statusBar))
    {
        this->Add(this->inputComp);
    }
};

/// @brief Create a `TerminalImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Terminal(std::shared_ptr<ConsoleImpl> consoleComp, std::weak_ptr<StatusBarImpl> statusBar)
{
    return ui::Make<TerminalImpl>(std::move(consoleComp), std::move(statusBar));
}

/// @brief Event handler for quick-focus on space pressed.
inline ui::ComponentDecorator /* NOLINT(readability-identifier-naming) */ TerminalSpaceToFocusHandler(ui::Component terminal)
{
    return ui::CatchEvent([terminal = std::move(terminal)](const ui::Event& event)
    {
        if (!terminal->Focused() && event == ui::Event::Character(' '))
        {
            terminal->TakeFocus(); // Focus terminal, on spacebar pressed.
            return true;
        }

        return false;
    });
}

/// @brief Event handler for quick-focus on ':' key press.
inline ui::ComponentDecorator /* NOLINT(readability-identifier-naming) */ TerminalQuickActionHandler(ui::Component terminal)
{
    return ui::CatchEvent([terminal = std::move(terminal)](const ui::Event& event)
    {
        if (event == ui::Event::Character(Config::QuickActionKey))
            terminal->TakeFocus(); // Focus terminal, when user types quick action key.

        return false;
    });
}
