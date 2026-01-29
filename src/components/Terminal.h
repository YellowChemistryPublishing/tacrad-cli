#pragma once

#include "Debug.h"
#include <Preamble.h>

#include <atomic>
#include <cctype>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <module/sys>
#include <module/sys.Threading>

#include <Config.h>
#include <Music.h>
#include <Screen.h>

/// @brief Terminal input component with blinking cursor.
/// @note Pass `byref`.
class TerminalImpl : public ui::ComponentBase
{
    static std::vector<std::string> argvParse(std::string_view cmd)
    {
        std::vector<std::string> argv;
        bool ignoreSpaces = false;
        for (auto it = cmd.begin(); it != cmd.end();)
        {
            while (it != cmd.end() && std::isspace(*it))
                ++it;
            if (it == cmd.end())
                break;

            std::string arg;
            while (it != cmd.end() && (ignoreSpaces || !std::isspace(*it)))
            {
                if (*it == '\\')
                {
                    const auto next = std::next(it);
                    if (next == cmd.end() || (*next != ' ' && *next != '\\' && *next != '"'))
                        arg.push_back('\\');
                    else
                    {
                        arg.push_back(*next);
                        ++it;
                    }
                }
                else if (*it == '"')
                    ignoreSpaces = !ignoreSpaces;
                else
                    arg.push_back(*it);

                ++it;
            }
            argv.emplace_back(std::move(arg));

            if (it != cmd.end())
                ++it;
        }

        return argv;
    }

    ui::ScreenInteractive& screen = Screen();
    ui::Component inputComp;

    std::string cmd;
    std::atomic<int_least32_t> pendingClear = 0;

    /// @brief Process a command entered into the terminal.
    void processCommand()
    {
        std::vector<std::string> argv = TerminalImpl::argvParse(this->cmd);

        if (!argv.empty())
        {
            if (argv.front() == "next" || argv.front() == "n")
                MusicPlayer::next();
        }
    }

    /// @brief Let the currently typed quick action live until one second from
    /// now.
    void resetQuickActionTimeout()
    {
        ++this->pendingClear;
        [](decltype(this) _this) -> sys::async // NOLINT(readability-static-accessed-through-instance)
        {
            co_await sys::task<>::delay(_as(i32, Config::QuickActionDelay.count()));
            if (--_this->pendingClear > 0)
                co_return;

            _this->screen.Post([_this]()
            {
                if (!_this->cmd.starts_with(Config::QuickActionKey))
                    return;

                _this->cmd = Config::QuickActionKey;
                _this->screen.PostEvent(ui::Event::Custom);
            });
        }(this);
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
    explicit TerminalImpl() :
        inputComp(ui::Input(ui::InputOption { .content = &this->cmd,
                                              .placeholder = std::format("{} for quick action...", Config::QuickActionKey),
                                              .transform = [this](ui::InputState state) -> ui::Element
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
    } }))
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
inline ui::Component /* NOLINT(readability-identifier-naming) */ Terminal() { return ui::Make<TerminalImpl>(); }

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
