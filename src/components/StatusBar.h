#pragma once

#include <chrono>
#include <condition_variable>
#include <format>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/util/ref.hpp>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <Config.h>
#include <Exec.inl>
#include <Music.h>
#include <Screen.h>
#include <Style.h>
#include <components/ThinSlider.h>

namespace ui = ftxui;

/// @brief Status bar component that displays temporary messages and track progress.
/// @note
/// Displays the last command output for `Config::StatusBarMessageDelay`, then reverts to track info.
/// Pass `byptr`.
class StatusBarImpl : public ui::ComponentBase
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

    ui::Component playButtonComp = ui::Button("Play",
                                              []
    {
        if (MusicPlayer::playing())
        {
            if (!MusicPlayer::pause())
                CommandInvocation::println("[log.error] Failed to pause track.");
            return;
        }

        if (MusicPlayer::loaded())
        {
            if (!MusicPlayer::resume())
                CommandInvocation::println("[log.error] Failed to resume track.");
            return;
        }

        if (!MusicPlayer::play() || !MusicPlayer::resume())
            CommandInvocation::println("[log.error] Failed to play track.");
    },
                                              ui::ButtonOption { .transform = [](const ui::EntryState& state) -> ui::Element
    { return postProcessIconButton(MusicPlayer::playing() ? ui::text(UserSettings::PauseButtonLabel) : ui::text(UserSettings::PlayButtonLabel), state); },
                                                                 .animated_colors {} });
    ui::Component stopButtonComp =
        ui::Button("Stop", [] { CommandInvocation::stop({ "[invoked by button press]" }); },
                   ui::ButtonOption { .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(UserSettings::StopButtonLabel), state); },
                                      .animated_colors {} });
    ui::Component nextButtonComp =
        ui::Button("Next", [] { CommandInvocation::next({ "[invoked by button press]" }); },
                   ui::ButtonOption { .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(UserSettings::NextButtonLabel), state); },
                                      .animated_colors {} });
    ui::Component volumeButtonComp = ui::Button("VolumeToggle",
                                                []
    {
        static float volumeResetTo = 0.0f;
        if (MusicPlayer::volume() == 0.0f)
            (void)MusicPlayer::volume(volumeResetTo);
        else
        {
            volumeResetTo = MusicPlayer::volume();
            (void)MusicPlayer::volume(0.0f);
        }
    },
                                                ui::ButtonOption { .transform = [](const ui::EntryState& state) -> ui::Element
    {
        return postProcessIconButton(ui::text(std::format("{}{}", UserSettings::VolumeSliderLabel,
                                                          [vol = MusicPlayer::volume()]
        {
            if (vol == 0.0f)
                return UserSettings::VolumeMuteLabel;
            if (vol < Config::LowVolumeThreshold)
                return UserSettings::VolumeLowLabel;
            return UserSettings::VolumeHighLabel;
        }())),
                                     state) |
            ui::color(UserSettings::FlavorUnemphasizedColor);
    },
                                                                   .animated_colors {} });
    ui::Component volumeSliderComp = ThinSlider([](const float value) { (void)MusicPlayer::volume(value); }, [](float& val) { val = MusicPlayer::volume(); },
                                                ui::size(ui::WIDTH, ui::EQUAL, Config::VolumeSliderWidth));
    ui::Component progressSliderComp = ThinSlider(
                                           [](const float value)
    {
        if (MusicPlayer::loaded())
            if (!MusicPlayer::seek(value * MusicPlayer::totalTime()))
                CommandInvocation::println("[log.error] Failed to seek track.");
    },
                                           [](float& val)
    {
        if (MusicPlayer::loaded())
        {
            const float total = MusicPlayer::totalTime();
            val = (total > 0.0f) ? (MusicPlayer::currentTime() / total) : 0.0f;
        }
        else
            val = 0.0f;
    }, ui::xflex) |
        ui::Renderer([](ui::Element elem)
    {
        return ui::hbox({ ui::text(std::format("{} / {}", MusicPlayer::formatTime(MusicPlayer::currentTime()), MusicPlayer::formatTime(MusicPlayer::totalTime()))),
                          ui::separatorEmpty(), std::move(elem) }) |
            ui::xflex;
    });

    ui::Component containerComp = ui::Container::Horizontal({ this->playButtonComp, hspace(), this->stopButtonComp, hspace(), this->nextButtonComp, hspace(),
                                                              this->volumeButtonComp, hspace(), this->volumeSliderComp, hspace(), this->progressSliderComp });
    ui::Component displayComp = ui::Renderer(this->containerComp,
                                             [this]
    {
        {
            const std::unique_lock guard(this->messageLock);
            if (!this->message.empty())
                return ui::text(this->message);
        }

        return this->containerComp->Render();
    }) |
        ui::CatchEvent([this](const ui::Event&)
    {
        const std::unique_lock guard(this->messageLock);
        return !this->message.empty();
    });
public:
    StatusBarImpl() { this->Add(this->displayComp); }

    /// @brief Show a temporary message on the status bar.
    /// @param msg The message to display (which will auto-clear after `Config::StatusBarMessageDelay` seconds).
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
        if (output.empty())
        {
            this->clearMessage();
            return true;
        }

        std::vector<sys::cstr> v = sys::cstr(output).split('\n');
        while (!v.empty() && v.back().trim().empty())
            v.pop_back();

        if (!v.empty()) [[likely]]
            this->showMessage(std::string(v.back().trim()));

        return true;
    }
};

/// @brief Create a status bar component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ StatusBar() { return ui::Make<StatusBarImpl>(); }
