#pragma once

#include <Preamble.h>

#include <algorithm>
#include <format>
#include <initializer_list>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>

#include <Screen.h>
#include <Utility.h>

class CommandInvocation
{
public:
    struct Entry
    {
        std::string cmd;
        std::string output;
    };
private:
    static inline std::vector<Entry> history;
public:
    CommandInvocation() = delete;

    static void clearHistory() { CommandInvocation::history.clear(); }
    static void pushCommand(std::string cmd) { CommandInvocation::history.emplace_back(Entry { .cmd = std::move(cmd), .output = "" }); }
    template <typename... Args>
    static void println(std::format_string<Args...> fmt, Args&&... args)
    {
        if (CommandInvocation::history.empty())
            CommandInvocation::history.emplace_back(Entry { .cmd = "", .output = "" });

        CommandInvocation::history.back().output.append(std::format(fmt, std::forward<Args>(args)...)).push_back('\n');
    }
    static const std::vector<Entry>& rawHistory() { return CommandInvocation::history; }

    static void help(const std::vector<std::string>& cmd);
    static void clear(const std::vector<std::string>& cmd);
    static void quit(const std::vector<std::string>& cmd);

    static void togglePlayingOrPlay(const std::vector<std::string>& cmd);
    static void resumeOrPlay(const std::vector<std::string>& cmd);
    static void play(const std::vector<std::string>& cmd);
    static void resume(const std::vector<std::string>& cmd);
    static void pause(const std::vector<std::string>& cmd);
    static void seek(const std::vector<std::string>& cmd);
    static void volume(const std::vector<std::string>& cmd);
    static void stop(const std::vector<std::string>&);
    static void next(const std::vector<std::string>& cmd);
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
        { Query { .startsWith = { { "volume", "vo", "v" } }, .usage = "`vol <linear volume>`", .desc = "Set the volume to the given linear value.", .exactCount = false },
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

        return std::ranges::any_of(CommandInvocation::validCommands, [&](const auto& elem) -> bool
        {
            const auto& [query, func] = elem;
            if (cmd.size() < query.startsWith.size() || (query.exactCount && query.startsWith.size() != cmd.size()) || !setVecStartsWith(query.startsWith, cmd))
                return false;

            func(cmd);
            return true;
        });
    }
};
