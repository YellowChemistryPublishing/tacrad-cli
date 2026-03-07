#pragma once

/// @file

#include <chrono>
#include <condition_variable>
#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/util/ref.hpp>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <CmdInv.inl>
#include <Config.h>
#include <Music.h>
#include <Screen.h>
#include <Style.h>
#include <components/Console.h>
#include <components/ThinSlider.h>

namespace ui = ftxui;

/// @brief Displays temporary messages, track controls, and track progress.
/// @note
/// Displays the last command output for `Config::StatusBarMessageDelay`, then reverts to track info.
/// Pass `byptr`.
class StatusBarImpl final : public ui::ComponentBase
{
    sys::cstr message;
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
            Screen().PostEvent(ui::Event::Custom);
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

    std::shared_ptr<ConsoleImpl> consoleComp;

    ui::Component playButtonComp = ui::Button("Play",
                                              [this]
    {
        if (MusicPlayer::loaded() && MusicPlayer::playing())
        {
            _retif(CmdInv::println(this->consoleComp->history, "[log.error] Failed to pause track. ({})", _as(std::string_view, pauseRes.err())),
                   auto pauseRes = MusicPlayer::pause();
                   !pauseRes);
            return;
        }

        if (MusicPlayer::loaded())
        {
            _retif(CmdInv::println(this->consoleComp->history, "[log.error] Failed to resume track. ({})", _as(std::string_view, resRes.err())),
                   auto resRes = MusicPlayer::resume();
                   !resRes);
            return;
        }

        auto playRes = MusicPlayer::play();
        if (!playRes || !(playRes = MusicPlayer::resume())) // NOLINT(bugprone-assignment-in-if-condition)
            CmdInv::println(this->consoleComp->history, "[log.error] Failed to play track. ({})", _as(std::string_view, playRes.err()));
    },
                                              ui::ButtonOption { .transform = [](const ui::EntryState& state) -> ui::Element
    { return postProcessIconButton(MusicPlayer::loaded() && MusicPlayer::playing() ? ui::text(UserSettings::PauseButtonLabel) : ui::text(UserSettings::PlayButtonLabel), state); },
                                                                 .animated_colors {} });
    ui::Component stopButtonComp =
        ui::Button("Stop", [this] { CmdInv::stop(this->consoleComp->history, { "[invoked by button press]" }); },
                   ui::ButtonOption { .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(UserSettings::StopButtonLabel), state); },
                                      .animated_colors {} });
    ui::Component nextButtonComp =
        ui::Button("Next", [this] { CmdInv::next(this->consoleComp->history, { "[invoked by button press]" }); },
                   ui::ButtonOption { .transform = [](const ui::EntryState& state) -> ui::Element { return postProcessIconButton(ui::text(UserSettings::NextButtonLabel), state); },
                                      .animated_colors {} });
    ui::Component volumeButtonComp = ui::Button("VolumeToggle",
                                                [this]
    {
        static float volumeResetTo = 0.0f;
        const float curVol = MusicPlayer::volume();
        if (curVol != 0.0f)
            volumeResetTo = curVol;

        if (auto volRes = MusicPlayer::volume(curVol == 0.0f ? volumeResetTo : 0.0f); !volRes)
            CmdInv::println(this->consoleComp->history, "[log.error] Failed to set volume. ({})", _as(std::string_view, volRes.err()));
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
    ui::Component volumeSliderComp = ThinSlider([this](const float value)
    {
        if (auto volRes = MusicPlayer::volume(value); !volRes)
            CmdInv::println(this->consoleComp->history, "[log.error] Failed to set volume. ({})", _as(std::string_view, volRes.err()));
    }, [](float& val) { val = MusicPlayer::volume(); }, ui::size(ui::WIDTH, ui::EQUAL, Config::VolumeSliderWidth));
    ui::Component progressSliderComp = ThinSlider(
                                           [this](const float value)
    {
        if (MusicPlayer::loaded())
            _retif(CmdInv::println(this->consoleComp->history, "[log.error] Failed to seek track. ({})", _as(std::string_view, seekRes.err())),
                   auto seekRes = MusicPlayer::seek(value * MusicPlayer::totalTime().move_or(0.0f));
                   !seekRes);
    },
                                           [](float& val)
    {
        if (MusicPlayer::loaded())
        {
            const float total = MusicPlayer::totalTime().move_or(0.0f);
            val = (total > 0.0f) ? (MusicPlayer::currentTime().move_or(0.0f) / total) : 0.0f;
        }
        else
            val = 0.0f;
    }, ui::xflex) |
        ui::Renderer([](ui::Element elem)
    {
        return ui::hbox({ ui::text(std::format("{} / {}", MusicPlayer::formatTime(MusicPlayer::currentTime().move_or(0.0f)),
                                               MusicPlayer::formatTime(MusicPlayer::totalTime().move_or(0.0f)))),
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
    /// @internal
    /// @private
    explicit StatusBarImpl(std::shared_ptr<ConsoleImpl> consoleComp) : consoleComp(std::move(consoleComp)) { this->Add(this->displayComp); }

    /// @brief Show a temporary message on the status bar.
    /// @param msg The message to display (which will auto-clear after `Config::StatusBarMessageDelay` seconds).
    void showMessage(sys::cstr msg)
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
    /// @return Whether last command existed and was able to be shown.
    [[nodiscard]] bool showLastCommandOutput()
    {
        const std::vector<CmdInv::Entry>& history = this->consoleComp->history;
        _retif(false, history.empty());

        const sys::cstr& output = history.back().output;
        if (output.empty())
        {
            this->clearMessage();
            return true;
        }

        std::vector<sys::cstr> v = sys::cstr(output).split('\n');
        while (!v.empty() && v.back().trim().empty())
            v.pop_back();

        if (!v.empty()) [[likely]]
            this->showMessage(sys::cstr(v.back().trim()));

        return true;
    }
};

/// @brief Create a `StatusBarImpl` component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ StatusBar(std::shared_ptr<ConsoleImpl> consoleComp) { return ui::Make<StatusBarImpl>(std::move(consoleComp)); }
