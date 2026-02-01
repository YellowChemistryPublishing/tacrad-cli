#pragma once

#include <cctype>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>

#include <Config.h>
#include <Exec.h>
#include <Music.h>
#include <Screen.h>
#include <components/StatusBar.h>

struct CommandProcessor
{
    CommandProcessor() = delete;

    static std::vector<std::string> argvParse(std::string_view cmd)
    {
        std::vector<std::string> argv;
        bool ignoreSpaces = false;
        for (auto it = cmd.begin(); it != cmd.end();) // NOLINT(readability-qualified-auto)
        {
            while (it != cmd.end() && std::isspace(*it))
                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (it == cmd.end())
                break;

            std::string arg;
            while (it != cmd.end() && (ignoreSpaces || !std::isspace(*it)))
            {
                if (*it == '\\')
                {
                    const auto next = std::next(it); // NOLINT(readability-qualified-auto)
                    if (next == cmd.end() || (*next != ' ' && *next != '\\' && *next != '"'))
                        arg.push_back('\\');
                    else
                    {
                        arg.push_back(*next);
                        ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    }
                }
                else if (*it == '"')
                    ignoreSpaces = !ignoreSpaces;
                else
                    arg.push_back(*it);

                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            argv.emplace_back(std::move(arg));

            if (it != cmd.end())
                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        return argv;
    }

    /// @brief Process a command entered into the terminal.
    static bool command(std::string_view cmd, std::weak_ptr<StatusBarImpl> statusBarPtr = {})
    {
        const std::vector<std::string> argv = CommandProcessor::argvParse(cmd);

        if (argv.empty())
            return false;

        CommandInvocation::pushCommand(std::string(cmd));
        if (!CommandInvocation::matchExecuteCommand(argv))
            CommandInvocation::println("[log.error] Unrecognized command `{}` with args {}.", cmd.front(), std::span(cmd).subspan(1));

        if (const std::shared_ptr<StatusBarImpl> statusBar = statusBarPtr.lock())
            statusBar->showLastCommandOutput();

        return true;
    }

    /// @brief Process a quick action entered into the terminal.
    static bool quickAction(std::string_view cmd)
    {
        const sz trimBeg = cmd.find_first_not_of(' ', !cmd.empty() && cmd[0] == Config::QuickActionKey ? 1 : 0);
        const sz trimEnd = cmd.find_last_not_of(' ') + 1uz;
        std::string actionId(trimBeg != std::string_view::npos ? cmd.begin() + ssz(trimBeg) : cmd.begin() /* NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) */,
                             trimEnd != std::string_view::npos ? cmd.begin() + ssz(trimEnd) : cmd.end() /* NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) */);
        for (char& c : actionId)
            c = _as(char, std::tolower(c));

        if (actionId == "a")
        {
            MusicPlayer::autoplay(!MusicPlayer::autoplay());
            return true;
        }

        return CommandInvocation::matchExecuteCommand({ std::format(":{}", actionId) });
    }
};
