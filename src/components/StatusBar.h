#pragma once

#include "Style.h"
#include <Preamble.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include <Config.h>
#include <Exec.h>
#include <Screen.h>

/// @brief Status bar component that displays temporary messages and track progress.
/// @note Displays the last command output for a configured duration, then reverts to track info.
class StatusBarImpl : public ui::ComponentBase, public std::enable_shared_from_this<StatusBarImpl>
{
    ui::ScreenInteractive& screen = Screen();

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

    // TODO(halloimdragon): Track progress.
public:
    StatusBarImpl() = default;

    /// @brief Show a temporary message on the status bar.
    /// @param msg The message to display (will auto-clear after `Config::StatusBarMessageDelay` seconds).
    void showMessage(std::string msg)
    {
        std::unique_lock guard(this->messageLock);
        this->message = std::move(msg);
        this->messageExpiry = std::chrono::steady_clock::now() + Config::StatusBarMessageDelay;
        this->messageCv.notify_one();
    }
    /// @brief Clear the current message immediately.
    void clearMessage()
    {
        std::unique_lock guard(this->messageLock);
        this->message.clear();
        this->messageExpiry = std::chrono::steady_clock::time_point::min();
        this->messageCv.notify_one();
    }

    /// @brief Show the last line of the most recent command's output in the status bar.
    /// @return Whether any output existed to show.
    bool showLastCommandOutput()
    {
        std::vector<InvocationEntry>& history = CommandInvocation::rawHistory();
        _retif(false, history.empty());

        const std::string& output = history.back().output;
        _retif(false, output.empty());

        // Find the very last line.
        sz end = output.find_last_not_of('\n');
        _retif(false, end == std::string::npos);
        sz beg = output.rfind('\n', end);
        beg = (beg == std::string::npos) ? 0_uz : beg + 1_uz;

        this->showMessage(output.substr(beg, end - beg + 1_uz));
        return true;
    }

    ui::Element OnRender() override
    {
        std::unique_lock guard(this->messageLock);

        if (!this->message.empty())
            return ui::hbox({
                ui::text(" "),
                ui::text(this->message) | ui::flex,
                ui::text(" "),
            });

        // TODO(halloimdragon): Show track progress when music is playing
        // return ui::hbox({
        //     ui::text(" "),
        //     ui::text(trackName) | ui::flex,
        //     ui::gauge(trackProgress) | ui::flex,
        //     ui::text(" "),
        // });

        return hpad(ui::filler());
    }
};

/// @brief Create a status bar component.
inline ui::Component /* NOLINT(readability-identifier-naming) */ StatusBar() { return ui::Make<StatusBarImpl>(); }
