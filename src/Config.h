#pragma once

#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <string_view>

#include <module/sys>

namespace ui = ftxui;

/// @brief Global static configuration.
/// @note Static class.
class Config final
{
public:
    Config() = delete;

    static constexpr std::string_view ApplicationName = "♪♫ tacrad-cli";
    static constexpr std::string_view VersionIdentifier = "v1.0.0-beta";

    static constexpr std::string_view BlankText = "Nothing to see here!";

    static constexpr float LowVolumeThreshold = 0.5f;
    static constexpr i32 VolumeSliderWidth = 6_i32;

    static constexpr char QuickActionKey = ':';
    static constexpr std::chrono::milliseconds QuickActionDelay = std::chrono::milliseconds(1000);
    static constexpr std::chrono::milliseconds StatusBarMessageDelay = std::chrono::milliseconds(3200);
    static constexpr std::chrono::milliseconds StatusBarDurationRefreshRate = std::chrono::milliseconds(250);
    static constexpr std::chrono::milliseconds StatusBarDurationRefreshRateInactive = std::chrono::milliseconds(1250);

    static constexpr std::chrono::milliseconds FlavorAnimationDuration = std::chrono::milliseconds(100);
};

/// @brief User settings that can be modified at runtime.
/// @note Static class.
class UserSettings final
{
public:
    UserSettings() = delete;

    static inline ui::BorderStyle border = ui::BorderStyle::LIGHT;

    static inline const ui::Color FlavorEmphasizedColor = ui::Color::White;
    static inline const ui::Color FlavorDescriptionColor = ui::Color::Grey70;
    static inline const ui::Color FlavorUnemphasizedColor = ui::Color::GrayDark;

    static constexpr std::string_view PlayButtonLabel = ">";
    static constexpr std::string_view PauseButtonLabel = "#";
    static constexpr std::string_view StopButtonLabel = "▪";
    static constexpr std::string_view NextButtonLabel = "»";

    static constexpr std::string_view VolumeSliderLabel = "<";
    static constexpr std::string_view VolumeMuteLabel = "×";
    static constexpr std::string_view VolumeLowLabel = "~";
    static constexpr std::string_view VolumeHighLabel = "≈";

    static constexpr std::string_view ProgressBarFill = "━";

    static constexpr std::string_view ReloadLabel = "●";
    static constexpr std::string_view LexicographicSortLabel = "a↑";
    static constexpr std::string_view ReverseLexicographicSortLabel = "z↓";
    static constexpr std::string_view ShuffleLabel = "↔";

    static constexpr i32 TagSelectPanelInitWidth = 22_i32;
    static constexpr i32 DetailsPanelInitWidth = 34_i32;
};
