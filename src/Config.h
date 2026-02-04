#pragma once

#include <Preamble.h>

#include <chrono>
#include <string_view>

/// @brief Global static configuration.
struct Config
{
    Config() = delete;

    static constexpr std::string_view ApplicationName = "♪♫ tacrad-cli";
    static constexpr std::string_view VersionIdentifier = "v0.1.0-alpha";

    static constexpr char QuickActionKey = ':';
    static constexpr std::chrono::milliseconds QuickActionDelay = std::chrono::milliseconds(1000);
    static constexpr std::chrono::milliseconds StatusBarMessageDelay = std::chrono::milliseconds(3200);
    static constexpr std::chrono::milliseconds StatusBarDurationRefreshRate = std::chrono::milliseconds(250);
    static constexpr std::chrono::milliseconds StatusBarDurationRefreshRateInactive = std::chrono::milliseconds(1250);

    static constexpr std::chrono::milliseconds FlavorAnimationDuration = std::chrono::milliseconds(100);
};

/// @brief User settings that can be modified at runtime.
struct UserSettings
{
    UserSettings() = delete;

    static inline ui::BorderStyle border = ui::BorderStyle::LIGHT;

    static inline const ui::Color FlavorEmphasizedColor = ui::Color::White;
    static inline const ui::Color FlavorUnemphasizedColor = ui::Color::GrayDark;

    static constexpr std::string_view PlayButtonLabel = ">";
    static constexpr std::string_view PauseButtonLabel = "#";
    static constexpr std::string_view StopButtonLabel = "□";
    static constexpr std::string_view NextButtonLabel = "»";
    static constexpr std::string_view ProgressBarFill = "━";
};
