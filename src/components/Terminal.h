#pragma once

#include <Preamble.h>

#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <module/sys>
#include <module/sys.Threading>

#include <Config.h>
#include <Screen.h>

/// @brief Terminal input component with blinking cursor.
/// @note Pass `byref`.
class TerminalImpl : public ui::ComponentBase
{
    ui::ScreenInteractive& screen = Screen();
    ui::Component input_comp;

    std::string cmd;
    std::atomic<int_least32_t> pendingClear = 0;

    /// @brief Process a command entered into the terminal.
    void processCommand()
    {
        std::vector<std::string_view> argv;
        for (auto it = this->cmd.begin(); it != this->cmd.end();)
        {
            while (it != this->cmd.end() && std::isspace(*it))
                ++it;
            if (it == this->cmd.end())
                break;

            auto beg = it;
            while (it != this->cmd.end() && !std::isspace(*it))
                ++it;
            argv.emplace_back(std::to_address(beg), std::to_address(it));

            if (it != this->cmd.end())
                ++it;
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
    /* NOLINT(readability-convert-member-functions-to-static) */ void processQuickAction(std::string_view action, ui::ScreenInteractive& screen)
    {
        const sz trimBeg = action.find_first_not_of(' ', !action.empty() && action[0] == Config::QuickActionKey ? 1 : 0);
        const sz trimEnd = action.find_last_not_of(' ') + 1uz;
        std::string actionId(trimBeg != std::string_view::npos ? action.begin() + ssz(trimBeg) : action.begin() /* NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) */,
                             trimEnd != std::string_view::npos ? action.begin() + ssz(trimEnd) : action.end() /* NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) */);
        for (char& c : actionId)
            c = _as(char, std::tolower(c));

        if (actionId == "q")
            screen.Exit();
    }
public:
    explicit TerminalImpl() :
        input_comp(ui::Input(ui::InputOption { .content = &this->cmd,
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

        this->processQuickAction(this->cmd, this->screen);
    },
                                               .on_enter = [this]() -> void
    {
        this->processCommand();
        this->cmd.clear();
    } }))
    {
        this->Add(this->input_comp);
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
