#pragma once

#include "Exec.h"
#include <Preamble.h>

#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <stop_token>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Threading>

#include <Config.h>
#include <Exec.h>
#include <Music.h>
#include <Screen.h>
#include <components/StatusBar.h>

/// @brief Terminal input component with blinking cursor.
/// @note Pass `byref`.
class TerminalImpl : public ui::ComponentBase, public std::enable_shared_from_this<TerminalImpl>
{
    static std::vector<std::string> argvParse(std::string_view cmd)
    {
        std::vector<std::string> argv;
        bool ignoreSpaces = false;
        for (auto it = cmd.begin(); it != cmd.end();) // NOLINT(readability-qualified-auto)
        {
            while (it != cmd.end() && std::isspace(*it))
                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (it == cmd.end())
                break;

            std::string arg;
            while (it != cmd.end() && (ignoreSpaces || !std::isspace(*it)))
            {
                if (*it == '\\')
                {
                    const auto next = std::next(it); // NOLINT(readability-qualified-auto)
                    if (next == cmd.end() || (*next != ' ' && *next != '\\' && *next != '"'))
                        arg.push_back('\\');
                    else
                    {
                        arg.push_back(*next);
                        ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    }
                }
                else if (*it == '"')
                    ignoreSpaces = !ignoreSpaces;
                else
                    arg.push_back(*it);

                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            argv.emplace_back(std::move(arg));

            if (it != cmd.end())
                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        return argv;
    }

    ui::ScreenInteractive& screen = Screen();
    std::weak_ptr<StatusBarImpl> statusBar;
    ui::Component inputComp;

    std::string cmd;

    std::chrono::steady_clock::time_point pendingClear = std::chrono::steady_clock::time_point::max();
    std::mutex pendingClearLock;
    std::condition_variable_any pendingClearCv;
    std::jthread pendingClearThread;

    /// @brief Process a command entered into the terminal.
    void processCommand()
    {
        std::vector<std::string> argv = TerminalImpl::argvParse(this->cmd);

        if (!argv.empty())
        {
            CommandInvocation::pushCommand(this->cmd);

            if (argv.front() == "next" || argv.front() == "n")
                MusicPlayer::next();
            else
                CommandInvocation::println("Unrecognized command `{}` with args {}.", argv.front(), std::span(argv).subspan(1));

            if (const std::shared_ptr<StatusBarImpl> statusBar = this->statusBar.lock())
                statusBar->showLastCommandOutput();
        }
    }

    /// @brief Let the currently typed quick action live until one second from
    /// now.
    void resetQuickActionTimeout()
    {
        std::unique_lock guard(this->pendingClearLock);
        this->pendingClear = std::chrono::steady_clock::now() + Config::QuickActionDelay;
        this->pendingClearCv.notify_one();
    }

    /// @brief Process a quick action entered into the terminal.
    /* NOLINT(readability-convert-member-functions-to-static) */ bool processQuickAction(std::string_view action, ui::ScreenInteractive& screen)
    {
        const sz trimBeg = action.find_first_not_of(' ', !action.empty() && action[0] == Config::QuickActionKey ? 1 : 0);
        const sz trimEnd = action.find_last_not_of(' ') + 1uz;
        std::string actionId(trimBeg != std::string_view::npos ? action.begin() + ssz(trimBeg) : action.begin() /* NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) */,
                             trimEnd != std::string_view::npos ? action.begin() + ssz(trimEnd) : action.end() /* NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) */);
        for (char& c : actionId)
            c = _as(char, std::tolower(c));

        if (actionId == "q")
        {
            screen.Exit();
            return true;
        }
        if (actionId == "n")
        {
            MusicPlayer::next();
            return true;
        }
        if (actionId == "p")
        {
            if (MusicPlayer::playing())
                MusicPlayer::pause();
            else
                MusicPlayer::resume();
            return true;
        }
        if (actionId == "a")
        {
            MusicPlayer::autoplay(!MusicPlayer::autoplay());
            return true;
        }

        return false;
    }
public:
    explicit TerminalImpl(std::weak_ptr<StatusBarImpl> statusBar) :
        statusBar(std::move(statusBar)), inputComp(ui::Input(ui::InputOption { .content = &this->cmd,
                                                                               .placeholder = std::format("{} for quick action...", Config::QuickActionKey),
                                                                               .transform = [](ui::InputState state) -> ui::Element
    {
        if (state.focused && !state.is_placeholder)
            state.element |= ui::focusCursorBlockBlinking;

        if (state.is_placeholder)
            state.element = ui::hbox({ ui::hbox({ state.element | ui::flex, ui::text(std::format("{} {}", Config::ApplicationName, Config::VersionIdentifier)) }) |
                                       ui::color(ui::Color::GrayDark) /* TODO: Replace back with `ui::dim` when it works on Windows cmd. */ | ui::flex |
                                       (state.hovered ? ui::underlined : ui::nothing) });
        else
            state.element = ui::hbox({ state.element, ui::filler() });

        return state.element;
    },
                                                                               .multiline = false,
                                                                               .on_change = [this]() -> void
    {
        if (!this->cmd.starts_with(Config::QuickActionKey))
            return;

        this->resetQuickActionTimeout();
        if (std::isspace(this->cmd.back()) || (this->cmd.size() > 1uz && this->cmd.back() == Config::QuickActionKey))
        {
            this->cmd.pop_back();
            return;
        }

        if (this->processQuickAction(this->cmd, this->screen))
            this->cmd.clear();
    },
                                                                               .on_enter = [this]() -> void
    {
        _retif(, this->cmd.empty());

        this->processCommand();
        this->cmd.clear();
    } })),
        pendingClearThread([this](std::stop_token token)
    {
        while (!token.stop_requested())
        {
            {
                std::unique_lock guard(this->pendingClearLock);
                this->pendingClearCv.wait(guard, token, [this]() { return this->pendingClear != std::chrono::steady_clock::time_point::max(); });
                if (token.stop_requested())
                    break;

                this->pendingClearCv.wait_until(guard, token, this->pendingClear, [this]() { return std::chrono::steady_clock::now() >= this->pendingClear; });
                if (token.stop_requested())
                    break;

                this->pendingClear = std::chrono::steady_clock::time_point::max();
            }

            this->screen.Post([this]()
            {
                if (!this->cmd.starts_with(Config::QuickActionKey) || this->cmd.size() <= 1uz)
                    return;

                this->cmd = Config::QuickActionKey;
                this->screen.PostEvent(ui::Event::Custom);
            });
        }
    })
    {
        this->Add(this->inputComp);
    }
    TerminalImpl(const TerminalImpl&) = delete;
    TerminalImpl(TerminalImpl&&) = delete;
    ~TerminalImpl() override = default;

    TerminalImpl& operator=(const TerminalImpl&) = delete;
    TerminalImpl& operator=(TerminalImpl&&) = delete;
};

/// @brief Create a terminal component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ Terminal(std::weak_ptr<StatusBarImpl> statusBar) { return ui::Make<TerminalImpl>(statusBar); }

inline ui::ComponentDecorator /* NOLINT(readability-identifier-naming) */ TerminalQuickActionHandler(const ui::Component& terminal)
{
    return ui::CatchEvent([&](ui::Event event)
    {
        if (event == ui::Event::Character(Config::QuickActionKey))
            terminal->TakeFocus(); // Focus terminal, but let user type the quick
                                   // action key.

        return false;
    });
}
