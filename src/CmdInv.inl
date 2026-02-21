#pragma once

#include <algorithm>
#include <format>
#include <ftxui/component/animation.hpp>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/receiver.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/direction.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/flexbox_config.hpp>
#include <ftxui/dom/linear_gradient.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/dom/requirement.hpp>
#include <ftxui/dom/selection.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/dom/take_any_args.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/color_info.hpp>
#include <ftxui/screen/deprecated.hpp>
#include <ftxui/screen/image.hpp>
#include <ftxui/screen/pixel.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <ftxui/screen/terminal.hpp>
#include <ftxui/util/autoreset.hpp>
#include <ftxui/util/ref.hpp>
#include <initializer_list>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>

#include <Screen.h>
#include <Utility.h>

/// @brief Command invocation functions.
/// @note Static class.
class CmdInv final // NOLINT(misc-use-internal-linkage): Spurious.
{
public:
    /// @brief Holds the invocation and output of a command.
    struct Entry
    {
        sys::cstr cmd;
        sys::cstr output;
    };

    CmdInv() = delete;

    static void clearHistory(std::vector<Entry>& history) { history.clear(); }
    static void pushCommand(std::vector<Entry>& history, sys::cstr cmd) { history.emplace_back(Entry { .cmd = sys::cstr("$ ").append(cmd), .output = "" }); }
    template <typename... Args>
    static void println(std::vector<Entry>& history, std::format_string<Args...> fmt, Args&&... args)
    {
        if (history.empty())
            history.emplace_back(Entry { .cmd = "", .output = "" });

        history.back().output.append(std::format(fmt, std::forward<Args>(args)...)).append('\n');
    }

    static void help(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void clear(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void quit(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);

    static void togglePlayingOrPlay(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void resumeOrPlay(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void play(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void resume(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void pause(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void seek(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void volume(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
    static void stop(std::vector<Entry>& history, const std::vector<sys::cstr>&);
    static void next(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);

    static void playlist(std::vector<Entry>& history, const std::vector<sys::cstr>& cmd);
private:
    struct Query
    {
        std::vector<std::set<std::string_view>> startsWith;
        sys::cstr usage;
        sys::cstr desc;
        bool exactCount = false;
    };
    static inline const std::vector<std::pair<Query, void (*)(std::vector<Entry>&, const std::vector<sys::cstr>&)>> ValidCommands {
        { Query { .startsWith = { { "p", ":p" } }, .usage = "1. `p`, 2. `p <track query>...`", .desc = "1. Toggle play/pause, 2. Alias for `play`.", .exactCount = false },
         &CmdInv::togglePlayingOrPlay                                                                                                                                                       },
        { Query { .startsWith = { { ">" } }, .usage = "1. `>`, 2. `> <track query>...`", .desc = "1. Alias for `resume`, 2. Alias for `play`.", .exactCount = false },
         &CmdInv::resumeOrPlay                                                                                                                                                              },
        { Query { .startsWith = { { "play", "p" } }, .usage = "`play <track query>...`", .desc = "Look for a track matching the query and play it.", .exactCount = false },
         &CmdInv::play                                                                                                                                                                      },
        { Query { .startsWith = { { "resume", "r", ":r" } }, .usage = "`resume`", .desc = "Resume current track.", .exactCount = true },                                    &CmdInv::resume },
        { Query { .startsWith = { { "pause", "p", "#", ":#" } }, .usage = "`pause`", .desc = "Pause current track.", .exactCount = true },                                  &CmdInv::pause  },
        { Query { .startsWith = { { "seek", "s", "=" } }, .usage = "`seek <seconds>`", .desc = "Seek to the given position in seconds.", .exactCount = true },              &CmdInv::seek   },
        { Query { .startsWith = { { "volume", "vol", "v" } }, .usage = "`vol <linear volume>`", .desc = "Set the volume to the given linear value.", .exactCount = true },
         &CmdInv::volume                                                                                                                                                                    },
        { Query { .startsWith = { { "stop", "s", ":x" } }, .usage = "`stop`", .desc = "Stop playing music.", .exactCount = true },                                          &CmdInv::stop   },
        { Query { .startsWith = { { "next", "n", ":n" } }, .usage = "`next`", .desc = "Play the next track.", .exactCount = true },                                         &CmdInv::next   },
        { Query { .startsWith = { { "playlist", "playl", "pl" } },
                  .usage = "`playlist [set <playlist name>]`\n    | `[--index|-i <index>]`\n    | `[--seq]`\n    | `[--revseq]`\n    | `[--shuffle|-sh]`\n    | `[--autoplay|-a]`",
                  .desc = "Playlist configuration.",
                  .exactCount = false },
         &CmdInv::playlist                                                                                                                                                                  },
        { Query { .startsWith = { { "clear", "c", ":c" } }, .usage = "`clear`", .desc = "Clear the console.", .exactCount = true },                                         &CmdInv::clear  },
        { Query { .startsWith = { { "exit", "q", ":q" } }, .usage = "`exit`", .desc = "Exit the program.", .exactCount = true },                                            &CmdInv::quit   },
        { Query { .startsWith = { { "help", "h" } }, .usage = "`help`", .desc = "Show this help message.", .exactCount = false },                                           &CmdInv::help   }
    };
public:
    /// @brief Execute a command with its argv.
    /// @return Whether a command to execute was found.
    [[nodiscard]] static bool matchExecuteCommand(std::vector<Entry>& history, const std::vector<sys::cstr>& argv)
    {
        if (argv.empty())
            return false;

        return std::ranges::any_of(CmdInv::ValidCommands, [&](const auto& v) -> bool
        {
            const auto& [query, func] = v;
            if (!setVecStartsWith(query.startsWith, argv))
                return false;

            if (query.exactCount && query.startsWith.size() != argv.size())
            {
                CmdInv::println(history, "[log.error] Wrong number of arguments given to `{}`! ({} instead of {})", _as(std::string_view, argv[0]), argv.size(),
                                query.startsWith.size());
                return false;
            }
            _retif(false, argv.size() < query.startsWith.size());

            func(history, argv);
            return true;
        });
    }
};
