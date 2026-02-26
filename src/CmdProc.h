#pragma once

/// @file CmdProc.h

#include <cctype>
#include <format>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <CmdInv.inl>
#include <Config.h>
#include <Debug.h>
#include <Music.h>
#include <components/StatusBar.h>

/// @brief Command processing functions.
/// @note Static class.
class CmdProc final
{
public:
    CmdProc() = delete;

    /// @brief Parse `cmd` into a vector of arguments.
    [[nodiscard]] static std::vector<sys::cstr> argvParse(std::string_view cmd)
    {
        std::vector<sys::cstr> argv;
        bool ignoreSpaces = false;
        for (auto it = cmd.begin(); it != cmd.end();) // NOLINT(readability-qualified-auto)
        {
            while (it != cmd.end() && std::isspace(*it))
                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (it == cmd.end())
                break;

            sys::cstr arg;
            while (it != cmd.end() && (ignoreSpaces || !std::isspace(*it)))
            {
                if (*it == '\\')
                {
                    const auto next = std::next(it); // NOLINT(readability-qualified-auto)
                    if (next == cmd.end() || (*next != ' ' && *next != '\\' && *next != '"'))
                        arg.append('\\');
                    else
                    {
                        arg.append(*next);
                        ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    }
                }
                else if (*it == '"')
                    ignoreSpaces = !ignoreSpaces;
                else
                    arg.append(*it);

                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            argv.emplace_back(std::move(arg));

            if (it != cmd.end())
                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        return argv;
    }

    /// @brief Parse and invoke `cmd`.
    static void invoke(std::vector<CmdInv::Entry>& history, sys::cstr cmd, std::weak_ptr<StatusBarImpl> statusBarPtr = {})
    {
        const std::vector<sys::cstr> argv = CmdProc::argvParse(cmd);
        if (argv.empty())
            return;

        CmdInv::pushCommand(history, std::move(cmd));
        if (!CmdInv::matchExecuteCommand(history, argv))
        {
            CmdInv::println(history, "[log.error] Unrecognized command `{}` with {}.", argv.front(),
                            argv.size() > 1uz ? sys::cstr(R"(args: ")").append(sys::cstr::join(std::span(argv).subspan(1), R"(", ")")).append('"') : sys::cstr("no args"));
        }

        if (const std::shared_ptr<StatusBarImpl> statusBar = statusBarPtr.lock())
            if (!statusBar->showLastCommandOutput())
                debugLog("Failed to show last command output on status bar.");
    }

    /// @brief Invoke a typed quick action `cmd`.
    [[nodiscard]] static bool invokeQuickAction(std::vector<CmdInv::Entry>& history, const std::string_view cmd)
    {
        const sz trimBeg = cmd.find_first_not_of(' ', !cmd.empty() && cmd[0] == Config::QuickActionKey ? 1 : 0);
        const sz trimEnd = cmd.find_last_not_of(' ') + 1uz;
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const sys::cstr actionId = sys::cstr(std::string_view(trimBeg != std::string_view::npos ? cmd.begin() + ssz(trimBeg) : cmd.begin(),
                                                              trimEnd != std::string_view::npos ? cmd.begin() + ssz(trimEnd) : cmd.end()))
                                       .to_lower();
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        if (actionId == "a")
        {
            MusicPlayer::autoplay(!MusicPlayer::autoplay());
            return true;
        }

        return CmdInv::matchExecuteCommand(history, { std::format(":{}", actionId) });
    }
};
