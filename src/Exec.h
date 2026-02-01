#pragma once

#include <Preamble.h>

#include <cstddef>
#include <cstdlib>
#include <format>
#include <ftxui/dom/elements.hpp>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>

#include <Music.h>
#include <Screen.h>

struct InvocationEntry
{
    std::string cmd;
    std::string output;
};

class CommandInvocation
{
    static inline std::vector<InvocationEntry> history;
public:
    CommandInvocation() = delete;

    static void clearHistory() { CommandInvocation::history.clear(); }
    static void pushCommand(std::string cmd) { CommandInvocation::history.emplace_back(InvocationEntry { .cmd = std::move(cmd), .output = "" }); }
    template <typename... Args>
    static void println(std::format_string<Args...> fmt, Args&&... args)
    {
        _retif(, CommandInvocation::history.empty());
        CommandInvocation::history.back().output.append(std::format(fmt, std::forward<Args>(args)...)).push_back('\n');
    }
    static const std::vector<InvocationEntry>& rawHistory() { return CommandInvocation::history; }

    static void help(const std::vector<std::string>& cmd)
    {
        if (cmd.size() > 1) [[unlikely]]
        {
            CommandInvocation::println("[log.error] \"help\" takes no arguments!");
            return;
        }

        for (const auto& [query, _] : CommandInvocation::validCommands)
        {
            std::string cmdName;
            for (const auto& tokenSet : query.startsWith)
            {
                for (const auto& token : tokenSet)
                    cmdName.append(token).push_back('|');
                if (!cmdName.empty())
                    cmdName.back() = ' ';
            }
            if (!cmdName.empty())
                cmdName.pop_back();
            CommandInvocation::println("{}\n    {}\n    {}", cmdName, query.usage, query.desc);
        }
    }
    static void clear(const std::vector<std::string>& cmd)
    {
        if (cmd.size() > 1) [[unlikely]]
        {
            CommandInvocation::println("[log.error] \"clear\" takes no arguments!");
            return;
        }

        CommandInvocation::clearHistory();
    }
    static void quit(const std::vector<std::string>& cmd)
    {
        if (cmd.size() > 1) [[unlikely]]
        {
            CommandInvocation::println("[log.error] \"exit\" takes no arguments!");
            return;
        }

        Screen().Exit();
    }

    static void togglePlayingOrPlay(const std::vector<std::string>& cmd)
    {
        if (cmd.size() == 1 && MusicPlayer::loaded())
        {
            if (MusicPlayer::playing())
                MusicPlayer::pause();
            else
                MusicPlayer::resume();
        }
        else
            CommandInvocation::play(cmd);
    }
    static void resumeOrPlay(const std::vector<std::string>& cmd)
    {
        if (cmd.size() == 1 && !MusicPlayer::playing())
            MusicPlayer::resume();
        else
            CommandInvocation::play(cmd);
    }
    static void play(const std::vector<std::string>& cmd)
    {
        if (cmd.size() < 2) [[unlikely]]
        {
            CommandInvocation::println("[log.error] Track title argument must be given to \"play\"!");
            return;
        }

        MusicPlayer::stopMusic();

        std::string lookupName = cmd[1];
        for (const auto& word : std::span(std::next(cmd.begin(), 2), cmd.end()))
        {
            lookupName.push_back(' ');
            lookupName.append(word);
        }

        MusicPlayer::queryStartMusic(lookupName);
    }
    static void resume(const std::vector<std::string>& cmd)
    {
        if (cmd.size() > 1) [[unlikely]]
        {
            CommandInvocation::println("[log.error] \"resume\" takes no arguments!");
            return;
        }

        if (MusicPlayer::loaded())
            MusicPlayer::resume();
        else
            CommandInvocation::println(R"([log.error] Not currently playing music! Use "play" and "stop" to change media.)");
    }
    static void pause(const std::vector<std::string>& cmd)
    {
        if (cmd.size() > 1) [[unlikely]]
        {
            CommandInvocation::println("[log.error] \"pause\" takes no arguments!");
            return;
        }

        if (MusicPlayer::loaded())
            MusicPlayer::pause();
        else
            CommandInvocation::println(R"([log.error] Not currently playing music! Use "play" and "stop" to change media.)");
    }
    static void seek(const std::vector<std::string>& cmd)
    {
        if (cmd.size() < 2) [[unlikely]]
        {
            CommandInvocation::println("[log.error] Seek position argument (in seconds) must be given to \"seek\"!");
            return;
        }
        if (cmd.size() > 2) [[unlikely]]
        {
            CommandInvocation::println("[log.error] Extra arguments given to \"seek\"!");
            return;
        }

        if (!MusicPlayer::loaded())
        {
            CommandInvocation::println(R"([log.error] Not currently playing music! Use "play" and "stop" to change media.)");
            return;
        }

        char* readEnd = nullptr; // NOLINT(misc-const-correctness)
        const float q = std::strtof(cmd[1].c_str(), &readEnd);
        if ((readEnd - cmd[1].data()) != _as(ptrdiff_t, cmd[1].size()))
        {
            CommandInvocation::println(R"([log.error] Invalid index argument given to "seek"!)");
            return;
        }

        MusicPlayer::seek(q);
    }
    static void volume(const std::vector<std::string>& cmd)
    {
        if (cmd.size() < 2) [[unlikely]]
        {
            CommandInvocation::println(R"([log.error] Volume argument (linear) must be given to "vol"!)");
            return;
        }
        if (cmd.size() > 2) [[unlikely]]
        {
            CommandInvocation::println(R"([log.error] Extra arguments given to "vol"!)");
            return;
        }

        float v = 1.0f; // NOLINT(misc-const-correctness)
        std::istringstream(cmd[1]) >> v;
        MusicPlayer::volume(v);
    }
    static void stop(const std::vector<std::string>&)
    {
        if (MusicPlayer::loaded())
            MusicPlayer::stopMusic();
        else
            CommandInvocation::println(R"([log.error] Not currently playing music! Use "play" to start media.)");
    }
    static void next(const std::vector<std::string>& cmd)
    {
        if (cmd.size() > 2)
            CommandInvocation::println(R"([log.error] Extra arguments given to "playl"!)");

        MusicPlayer::next();
    }
private:
    struct Query
    {
        std::vector<std::set<std::string_view>> startsWith;
        std::string usage;
        std::string desc;
        bool exactCount = false;
    };
    static inline std::vector<std::pair<Query, void (*)(const std::vector<std::string>&)>> validCommands {
        { Query { .startsWith = { { "p", ":p" } }, .usage = "1. `p`, 2. `p <track query>...`", .desc = "1. Toggle play/pause, 2. Alias for `play`.", .exactCount = false },
         &CommandInvocation::togglePlayingOrPlay                                                                                                                                                       },
        { Query { .startsWith = { { ">" } }, .usage = "1. `>`, 2. `> <track query>...`", .desc = "1. Alias for `resume`, 2. Alias for `play`.", .exactCount = false },
         &CommandInvocation::resumeOrPlay                                                                                                                                                              },
        { Query { .startsWith = { { "play", "p" } }, .usage = "`play <track query>...`", .desc = "Look for a track matching the query and play it.", .exactCount = false },
         &CommandInvocation::play                                                                                                                                                                      },
        { Query { .startsWith = { { "resume", "r", ":r" } }, .usage = "`resume`", .desc = "Resume current track.", .exactCount = false },                                   &CommandInvocation::resume },
        { Query { .startsWith = { { "pause", "p", "#", ":#" } }, .usage = "`pause`", .desc = "Pause current track.", .exactCount = false },                                 &CommandInvocation::pause  },
        { Query { .startsWith = { { "seek", "s", "=" } }, .usage = "`seek <seconds>`", .desc = "Seek to the given position in seconds.", .exactCount = false },
         &CommandInvocation::seek                                                                                                                                                                      },
        { Query { .startsWith = { { "volume", "vol", "v" } }, .usage = "`vol <linear volume>`", .desc = "Set the volume to the given linear value.", .exactCount = false },
         &CommandInvocation::volume                                                                                                                                                                    },
        { Query { .startsWith = { { "stop", "s", ":x" } }, .usage = "`stop`", .desc = "Stop playing music.", .exactCount = false },                                         &CommandInvocation::stop   },
        { Query { .startsWith = { { "next", "n", ":n" } }, .usage = "`next`", .desc = "Play the next track.", .exactCount = false },                                        &CommandInvocation::next   },
        { Query { .startsWith = { { "clear", "c", ":c" } }, .usage = "`clear`", .desc = "Clear the console.", .exactCount = false },                                        &CommandInvocation::clear  },
        { Query { .startsWith = { { "exit", "q", ":q" } }, .usage = "`exit`", .desc = "Exit the program.", .exactCount = false },                                           &CommandInvocation::quit   },
        { Query { .startsWith = { { "help", "h" } }, .usage = "`help`", .desc = "Show this help message.", .exactCount = false },                                           &CommandInvocation::help   }
    };
public:
    static bool matchExecuteCommand(const std::vector<std::string>& cmd)
    {
        if (cmd.empty())
            return false;

        for (const auto& [query, func] : CommandInvocation::validCommands)
        {
            if (cmd.size() < query.startsWith.size() || (query.exactCount && query.startsWith.size() != cmd.size()))
                continue;

            bool doesStartWith = true;
            sz i = 0_uz;
            for (const auto& token : query.startsWith)
            {
                if (!token.contains(cmd[i]))
                {
                    doesStartWith = false;
                    break;
                }
                ++i;
            }
            if (!doesStartWith)
                continue;

            func(cmd);
            return true;
        }

        return false;
    }
};
