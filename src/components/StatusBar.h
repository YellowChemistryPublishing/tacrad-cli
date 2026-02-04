#pragma once

#include <Preamble.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <format>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <module/sys>

#include <Config.h>
#include <Exec.inl>
#include <Music.h>
#include <Screen.h>
#include <Utility.h>

/// @brief Status bar component that displays temporary messages and track progress.
/// @note
/// Displays the last command output for `Config::StatusBarMessageDelay`, then reverts to track info.
/// Pass `byptr`.
class StatusBarImpl : public ui::ComponentBase, public std::enable_shared_from_this<StatusBarImpl>
{
    ui::ScreenInteractive& screen = Screen(); // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

    std::string message;
    std::chrono::steady_clock::time_point messageExpiry = std::chrono::steady_clock::time_point::min();
    std::mutex messageLock;
    std::condition_variable_any messageCv;
    std::jthread messageThread { [this](std::stop_token token)
    {
        while (!token.stop_requested())
        {
            std::unique_lock guard(this->messageLock);
            if (this->messageExpiry == std::chrono::steady_clock::time_point::min())
            {
                this->messageCv.wait(guard, token, [this] { return this->messageExpiry != std::chrono::steady_clock::time_point::min(); });
                if (token.stop_requested())
                    break;
            }

            const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            if (now < this->messageExpiry)
            {
                this->messageCv.wait_until(guard, token, this->messageExpiry, [this] { return std::chrono::steady_clock::now() >= this->messageExpiry; });
                continue;
            }

            this->message.clear();
            this->messageExpiry = std::chrono::steady_clock::time_point::min();
            this->screen.PostEvent(ui::Event::Custom);
        }
    } };

    std::jthread progressThread { [](std::stop_token token)
    {
        while (!token.stop_requested())
        {
            if (MusicPlayer::playing() && MusicPlayer::loaded())
            {
                Screen().PostEvent(ui::Event::Custom);
                std::this_thread::sleep_for(Config::StatusBarDurationRefreshRate);
            }
            else
                std::this_thread::sleep_for(Config::StatusBarDurationRefreshRateInactive);
        }
    } };

    ui::Element /* NOLINT(readability-convert-member-functions-to-static) */ postProcessButton(ui::Element elem, const ui::EntryState& state)
    {
        if (state.active)
            elem = std::move(elem) | ui::bold | ui::focus;
        if (state.focused)
            elem = std::move(elem) | ui::color(UserSettings::FlavorEmphasizedColor) | ui::underlined;
        else
            elem |= ui::color(UserSettings::FlavorUnemphasizedColor);
        return elem;
    }

    ui::Component playPauseButton = ui::Button("Play",
                                               []
    {
        if (!MusicPlayer::loaded())
        {
            if (!MusicPlayer::play())
                CommandInvocation::println("[log.error] Failed to play track.");
            return;
        }

        if (!MusicPlayer::playing())
        {
            if (!MusicPlayer::resume())
                CommandInvocation::println("[log.error] Failed to resume track.");
            return;
        }

        if (!MusicPlayer::pause())
            CommandInvocation::println("[log.error] Failed to pause track.");
    },
                                               ui::ButtonOption { .transform = [this](const ui::EntryState& state) -> ui::Element
    {
        return this->postProcessButton(MusicPlayer::playing() && MusicPlayer::loaded() ? ui::text(UserSettings::PauseButtonLabel) : ui::text(UserSettings::PlayButtonLabel), state);
    },
                                                                  .animated_colors {} });
    ui::Component stopButton = ui::Button("Stop", [] { CommandInvocation::stop({ "[invoked by button press]" }); },
                                          ui::ButtonOption { .transform = [this](const ui::EntryState& state) -> ui::Element
    { return this->postProcessButton(ui::text(UserSettings::StopButtonLabel), state); },
                                                             .animated_colors {} });
    ui::Component nextButton = ui::Button("Next", [] { CommandInvocation::next({ "[invoked by button press]" }); },
                                          ui::ButtonOption { .transform = [this](const ui::EntryState& state) -> ui::Element
    { return this->postProcessButton(ui::text(UserSettings::NextButtonLabel), state); },
                                                             .animated_colors {} });

    float trackProgress = 0.0f;
    ui::Box sliderBounds;
    ui::Component progressSliderComp = ui::Renderer([this]
    {
        {
            const std::unique_lock guard(this->messageLock);
            if (!this->message.empty())
                return ui::text(this->message);
        }

        if (MusicPlayer::loaded())
        {
            const float total = MusicPlayer::totalTime();
            this->trackProgress = (total > 0.0f) ? (MusicPlayer::currentTime() / total) : 0.0f;
        }
        else
            this->trackProgress = 0.0f;

        const i32 totalWidth = std::max(0_i32, i32(this->sliderBounds.x_max) - i32(this->sliderBounds.x_min) + 1_i32);
        const i32 filledWidth = i32(this->trackProgress * _as(float, totalWidth));

        return ui::hbox({ ui::text(std::format("{} / {}", MusicPlayer::formatTime(MusicPlayer::currentTime()), MusicPlayer::formatTime(MusicPlayer::totalTime()))),
                          ui::separatorEmpty(),
                          ui::hbox({
                              ui::separatorCharacter(UserSettings::ProgressBarFill) | ui::color(UserSettings::FlavorEmphasizedColor) | ui::size(ui::WIDTH, ui::EQUAL, filledWidth),
                              ui::separatorCharacter(UserSettings::ProgressBarFill) | ui::color(UserSettings::FlavorUnemphasizedColor) | ui::xflex,
                          }) | ui::selectionStyleReset |
                              ui::xflex | ui::reflect(this->sliderBounds) }) |
            ui::xflex;
    });

    ui::CapturedMouse capturedMouse;
    ui::Component containerComp =
        ui::Container::Horizontal({ this->playPauseButton, ui::Renderer([] { return ui::separatorEmpty(); }), this->stopButton, ui::Renderer([] { return ui::separatorEmpty(); }),
                                    this->nextButton, ui::Renderer([] { return ui::separatorEmpty(); }), this->progressSliderComp }) |
        ui::CatchEvent([this](ui::Event event)
    {
        if (!event.is_mouse() || !MusicPlayer::loaded())
            return false;

        if (!this->capturedMouse && event.mouse().button == ui::Mouse::Left && event.mouse().motion == ui::Mouse::Pressed &&
            this->sliderBounds.Contain(event.mouse().x, event.mouse().y))
        {
            this->capturedMouse = this->CaptureMouse(event);
            this->progressSliderComp->TakeFocus();
        }

        if (!this->capturedMouse)
            return false;

        if (event.mouse().motion == ui::Mouse::Released)
        {
            this->capturedMouse.reset();
            return true;
        }

        if (this->sliderBounds.x_min < this->sliderBounds.x_max)
        {
            const float width = _as(float, this->sliderBounds.x_max - this->sliderBounds.x_min);
            const float progress = std::clamp(_as(float, event.mouse().x - this->sliderBounds.x_min) / width, 0.0f, 1.0f);

            if (this->trackProgress != progress)
            {
                this->trackProgress = progress;
                if (!MusicPlayer::seek(this->trackProgress * MusicPlayer::totalTime()))
                    CommandInvocation::println("[log.error] Failed to seek track.");
            }
        }

        return true;
    });
public:
    StatusBarImpl() { this->Add(this->containerComp); }

    /// @brief Show a temporary message on the status bar.
    /// @param msg The message to display (will auto-clear after `Config::StatusBarMessageDelay` seconds).
    void showMessage(std::string msg)
    {
        const std::unique_lock guard(this->messageLock);
        this->message = std::move(msg);
        this->messageExpiry = std::chrono::steady_clock::now() + Config::StatusBarMessageDelay;
        this->messageCv.notify_one();
    }
    /// @brief Clear the current message immediately.
    void clearMessage()
    {
        const std::unique_lock guard(this->messageLock);
        this->message.clear();
        this->messageExpiry = std::chrono::steady_clock::time_point::min();
        this->messageCv.notify_one();
    }

    /// @brief Show the last line of the most recent command's output in the status bar.
    /// @return Whether any output existed to show.
    bool showLastCommandOutput()
    {
        const std::vector<CommandInvocation::Entry>& history = CommandInvocation::rawHistory();
        _retif(false, history.empty());

        const std::string& output = history.back().output;
        _retif(false, output.empty());

        this->showMessage(wstringLastLineTrimmed(output));
        return true;
    }
};

/// @brief Create a status bar component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ StatusBar() { return ui::Make<StatusBarImpl>(); }
