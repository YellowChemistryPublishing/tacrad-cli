#pragma once

#include <Preamble.h>

#include <chrono>
#include <string_view>

/// @brief Global static configuration.
struct Config
{
    Config() = delete;

    static constexpr std::string_view ApplicationName = "tacrad-cli";
    static constexpr std::string_view VersionIdentifier = "v0.1.0-alpha";

    static constexpr char QuickActionKey = ':';
    static constexpr std::chrono::milliseconds QuickActionDelay = std::chrono::milliseconds(1000);
};

/// @brief User settings that can be modified at runtime.
struct UserSettings
{
    UserSettings() = delete;

    static inline ui::BorderStyle border = ui::DOUBLE;
};
