#pragma once

#include <Preamble.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <format>
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

    float trackProgress = 0.0f;
    ui::Box sliderBounds;
    ui::CapturedMouse capturedMouse;

    ui::Element postProcessButton(ui::Element elem, ui::EntryState state)
    {
        if (state.active)
            elem |= ui::bold;
        if (state.focused)
            elem = elem | ui::color(UserSettings::FlavorEmphasizedColor) | ui::underlined;
        else
            elem |= ui::color(UserSettings::FlavorUnemphasizedColor);
        return elem;
    }

    ui::Component playPauseButton = ui::Button("Play",
                                               []
    {
        if (MusicPlayer::loaded())
        {
            if (MusicPlayer::playing() && !MusicPlayer::pause())
                CommandInvocation::println("[log.error] Failed to pause track.");
            else if (!MusicPlayer::resume())
                CommandInvocation::println("[log.error] Failed to resume track.");
        }
        else if (!MusicPlayer::play())
            CommandInvocation::println("[log.error] Failed to play track.");
    },
                                               ui::ButtonOption { .transform = [this](ui::EntryState state) -> ui::Element
    {
        return this->postProcessButton(MusicPlayer::playing() && MusicPlayer::loaded() ? ui::text(UserSettings::PauseButtonLabel) : ui::text(UserSettings::PlayButtonLabel), state);
    },
                                                                  .animated_colors {} });
    ui::Component stopButton =
        ui::Button("Stop", [] { CommandInvocation::stop({ "[invoked by button press]" }); },
                   ui::ButtonOption { .transform = [this](ui::EntryState state) -> ui::Element { return this->postProcessButton(ui::text(UserSettings::StopButtonLabel), state); },
                                      .animated_colors {} });
    ui::Component nextButton =
        ui::Button("Next", [] { CommandInvocation::next({ "[invoked by button press]" }); },
                   ui::ButtonOption { .transform = [this](ui::EntryState state) -> ui::Element { return this->postProcessButton(ui::text(UserSettings::NextButtonLabel), state); },
                                      .animated_colors {} });

    ui::Component progressSlider = ui::Renderer([this]
    {
        const i32 totalWidth = std::max(0_i32, i32(this->sliderBounds.x_max) - i32(this->sliderBounds.x_min) + 1_i32);
        const i32 filledWidth = i32(this->trackProgress * _as(float, totalWidth));

        return ui::hbox({ ui::text(std::format("{} / {}", MusicPlayer::formatTime(MusicPlayer::currentTime()), MusicPlayer::formatTime(MusicPlayer::totalTime()))),
                          ui::separatorEmpty(),
                          ui::hbox({
                              ui::separatorCharacter(UserSettings::ProgressBarFill) | ui::color(UserSettings::FlavorEmphasizedColor) | ui::size(ui::WIDTH, ui::EQUAL, filledWidth),
                              ui::separatorCharacter(UserSettings::ProgressBarFill) | ui::color(UserSettings::FlavorUnemphasizedColor) | ui::xflex,
                          }) | ui::reflect(this->sliderBounds) |
                              ui::xflex | ui::selectionStyleReset });
    });

    [[nodiscard]] bool Focusable() const final { return true; }

    [[nodiscard]] bool OnEvent(ui::Event event) final
    {
        if (!event.is_mouse() || !MusicPlayer::loaded())
            return false;

        if (event.mouse().button == ui::Mouse::Left && event.mouse().motion == ui::Mouse::Pressed && this->sliderBounds.Contain(event.mouse().x, event.mouse().y) &&
            !this->capturedMouse)
        {
            this->capturedMouse = this->CaptureMouse(event);
            this->TakeFocus();
        }

        if (this->capturedMouse && event.mouse().motion == ui::Mouse::Released)
            this->capturedMouse.reset();

        if (this->sliderBounds.x_max != this->sliderBounds.x_min)
        {
            const float lastProgress = this->trackProgress;
            this->trackProgress = _as(float, event.mouse().x - this->sliderBounds.x_min) / _as(float, this->sliderBounds.x_max - this->sliderBounds.x_min);
            this->trackProgress = std::clamp(this->trackProgress, 0.0f, 1.0f);

            if (this->trackProgress != lastProgress && MusicPlayer::loaded())
            {
                if (!MusicPlayer::seek(this->trackProgress * MusicPlayer::totalTime()))
                    CommandInvocation::println("[log.error] Failed to seek track.");
            }
        }

        return this->ComponentBase::OnEvent(event);
    }

    [[nodiscard]] ui::Element OnRender() final
    {
        {
            const std::unique_lock guard(this->messageLock);
            if (!this->message.empty())
                return ui::text(this->message);
        }

        if (!this->capturedMouse)
        {
            if (MusicPlayer::loaded())
            {
                const float total = MusicPlayer::totalTime();
                this->trackProgress = (total > 0.0f) ? (MusicPlayer::currentTime() / total) : 0.0f;
            }
            else
                this->trackProgress = 0.0f;
        }

        return ui::hbox({ this->playPauseButton->Render(), ui::separatorEmpty(), this->stopButton->Render(), ui::separatorEmpty(), this->nextButton->Render(), ui::separatorEmpty(),
                          this->progressSlider->Render() });
    }
public:
    StatusBarImpl()
    {
        this->Add(this->playPauseButton);
        this->Add(this->stopButton);
        this->Add(this->nextButton);
        this->Add(this->progressSlider);
    }

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
