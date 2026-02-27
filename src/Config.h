#pragma once

/// @file Config.h

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

    /// @brief @anchor Config_ApplicationName
    static constexpr std::string_view ApplicationName = "♪♫ tacrad-cli";
    /// @brief @anchor Config_VersionIdentifier
    static constexpr std::string_view VersionIdentifier = "v1.0.0-beta";

    /// @brief @anchor Config_BlankText
    static constexpr std::string_view BlankText = "Nothing to see here!";

    /// @brief @anchor Config_LowVolumeThreshold
    static constexpr float LowVolumeThreshold = 0.5f;
    /// @brief @anchor Config_VolumeSliderWidth
    static constexpr i32 VolumeSliderWidth = 6_i32;

    /// @brief @anchor Config_QuickActionKey
    static constexpr char QuickActionKey = ':';
    /// @brief @anchor Config_QuickActionDelay
    static constexpr std::chrono::milliseconds QuickActionDelay = std::chrono::milliseconds(1000);
    /// @brief @anchor Config_StatusBarMessageDelay
    static constexpr std::chrono::milliseconds StatusBarMessageDelay = std::chrono::milliseconds(3200);
    /// @brief @anchor Config_StatusBarDurationRefreshRate
    static constexpr std::chrono::milliseconds StatusBarDurationRefreshRate = std::chrono::milliseconds(250);
    /// @brief @anchor Config_StatusBarDurationRefreshRateInactive
    static constexpr std::chrono::milliseconds StatusBarDurationRefreshRateInactive = std::chrono::milliseconds(1250);
    /// @brief @anchor Config_FlavorAnimationDuration
    static constexpr std::chrono::milliseconds FlavorAnimationDuration = std::chrono::milliseconds(100);
};

/// @brief User settings that can be modified at runtime.
/// @note Static class.
class UserSettings final
{
public:
    UserSettings() = delete;

    /// @brief @anchor UserSettings_Border
    static inline ui::BorderStyle border = ui::BorderStyle::LIGHT;

    /// @brief @anchor UserSettings_FlavorEmphasizedColor
    static inline const ui::Color FlavorEmphasizedColor = ui::Color::White;
    /// @brief @anchor UserSettings_FlavorDescriptionColor
    static inline const ui::Color FlavorDescriptionColor = ui::Color::Grey70;
    /// @brief @anchor UserSettings_FlavorUnemphasizedColor
    static inline const ui::Color FlavorUnemphasizedColor = ui::Color::GrayDark;

    /// @brief @anchor UserSettings_PlayButtonLabel
    static constexpr std::string_view PlayButtonLabel = ">";
    /// @brief @anchor UserSettings_PauseButtonLabel
    static constexpr std::string_view PauseButtonLabel = "#";
    /// @brief @anchor UserSettings_StopButtonLabel
    static constexpr std::string_view StopButtonLabel = "▪";
    /// @brief @anchor UserSettings_NextButtonLabel
    static constexpr std::string_view NextButtonLabel = "»";

    /// @brief @anchor UserSettings_VolumeSliderLabel
    static constexpr std::string_view VolumeSliderLabel = "<";
    /// @brief @anchor UserSettings_VolumeMuteLabel
    static constexpr std::string_view VolumeMuteLabel = "×";
    /// @brief @anchor UserSettings_VolumeLowLabel
    static constexpr std::string_view VolumeLowLabel = "~";
    /// @brief @anchor UserSettings_VolumeHighLabel
    static constexpr std::string_view VolumeHighLabel = "≈";

    /// @brief @anchor UserSettings_ProgressBarFill
    static constexpr std::string_view ProgressBarFill = "━";

    /// @brief @anchor UserSettings_ReloadLabel
    static constexpr std::string_view ReloadLabel = "●";
    /// @brief @anchor UserSettings_LexicographicSortLabel
    static constexpr std::string_view LexicographicSortLabel = "a↑";
    /// @brief @anchor UserSettings_ReverseLexicographicSortLabel
    static constexpr std::string_view ReverseLexicographicSortLabel = "z↓";
    /// @brief @anchor UserSettings_ShuffleLabel
    static constexpr std::string_view ShuffleLabel = "↔";

    /// @brief @anchor UserSettings_TagSelectPanelInitWidth
    static constexpr i32 TagSelectPanelInitWidth = 22_i32;
    /// @brief @anchor UserSettings_DetailsPanelInitWidth
    static constexpr i32 DetailsPanelInitWidth = 34_i32;
};
