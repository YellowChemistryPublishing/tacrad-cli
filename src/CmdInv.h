#pragma once

#include <cstddef>
#include <cstdlib>
#include <format>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iterator>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <CmdInv.inl>
#include <Music.h>
#include <Screen.h>

inline void CmdInv::help(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>&)
{
    for (const auto& [query, _] : CmdInv::ValidCommands)
    {
        sys::cstr cmdName;
        for (const auto& tokenSet : query.startsWith)
        {
            for (const auto& token : tokenSet)
                cmdName.append(token).append('|');
            if (!cmdName.empty())
                cmdName.back(unsafe()) = ' ';
        }

        if (!cmdName.empty())
            cmdName.pop_back(unsafe());
        if (!query.exactCount)
            cmdName.append(" ...");

        CmdInv::println(history, "{}\n    {}\n    {}", cmdName, query.usage, query.desc);
    }
}
inline void CmdInv::clear(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>&) { CmdInv::clearHistory(history); }
inline void CmdInv::quit(std::vector<CmdInv::Entry>&, const std::vector<sys::cstr>&) { Screen().Exit(); }

inline void CmdInv::togglePlayingOrPlay(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    if (cmd.size() == 1uz && MusicPlayer::loaded())
    {
        if (MusicPlayer::playing())
        {
            if (auto pauseRes = MusicPlayer::pause(); !pauseRes)
                CmdInv::println(history, "[log.error] Failed to pause track: {}", _as(std::string_view, pauseRes.err()));
        }
        else if (auto resRes = MusicPlayer::resume(); !resRes)
            CmdInv::println(history, "[log.error] Failed to resume track: {}", _as(std::string_view, resRes.err()));
    }
    else
        CmdInv::play(history, cmd);
}
inline void CmdInv::resumeOrPlay(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    if (cmd.size() == 1 && !MusicPlayer::playing())
    {
        if (auto resRes = MusicPlayer::resume(); !resRes)
            CmdInv::println(history, "[log.error] Failed to resume track: {}", _as(std::string_view, resRes.err()));
    }
    else
        CmdInv::play(history, cmd);
}
inline void CmdInv::play(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] Track title argument must be given to "play"!")"), cmd.size() < 2uz);
    _retif(CmdInv::println(history, "[log.error] Failed to stop track. ({})", _as(std::string_view, stopRes.err())), auto stopRes = MusicPlayer::stopMusic(); !stopRes);

    const sys::cstr lookupName = sys::cstr::join(std::span(std::next(cmd.begin(), 1z), cmd.end()), ' ');
    _retif(CmdInv::println(history, "[log.error] Failed to start track. ({})", _as(std::string_view, startRes.err())),
           auto startRes = MusicPlayer::queryStartMusic(sys::str(_as(std::string_view, lookupName)));
           !startRes);
}
inline void CmdInv::resume(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>&)
{
    _retif(CmdInv::println(history, R"([log.error] Not currently playing music! Use "play" and "stop" to change media. ({}))", _as(std::string_view, resRes.err())),
           auto resRes = MusicPlayer::resume();
           !resRes);
}
inline void CmdInv::pause(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>&)
{
    _retif(CmdInv::println(history, R"([log.error] Not currently playing music! Use "play" and "stop" to change media. ({}))", _as(std::string_view, pauseRes.err())),
           auto pauseRes = MusicPlayer::pause();
           !pauseRes);
}
inline void CmdInv::seek(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] Not currently playing music! Use "play" and "stop" to change media.)"), !MusicPlayer::loaded());

    char* readEnd = nullptr; // NOLINT(misc-const-correctness)
    const float q = std::strtof(cmd[1uz].c_str(), &readEnd);
    _retif(CmdInv::println(history, R"([log.error] Invalid index argument given to "seek"!)"), readEnd - cmd[1uz].data() != _as(ptrdiff_t, cmd[1uz].size()));
    _retif(CmdInv::println(history, "[log.error] Failed to seek track. ({})", _as(std::string_view, seekRes.err())), auto seekRes = MusicPlayer::seek(q); !seekRes);
}
inline void CmdInv::volume(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    float v = 1.0f; // NOLINT(misc-const-correctness)
    std::istringstream(_as(std::string, cmd[1uz])) >> v;
    _retif(CmdInv::println(history, "[log.error] Failed to set volume. ({})", _as(std::string_view, volRes.err())), auto volRes = MusicPlayer::volume(v); !volRes);
}
inline void CmdInv::stop(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>&)
{
    if (MusicPlayer::loaded())
        _retif(CmdInv::println(history, "[log.error] Failed to stop track. ({})", _as(std::string_view, stopRes.err())), auto stopRes = MusicPlayer::stopMusic(); !stopRes);
    else
        CmdInv::println(history, R"([log.error] Not currently playing music! Use "play" to start media.)");
}
inline void CmdInv::next(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>&)
{
    _retif(CmdInv::println(history, "[log.error] Failed to play next track. ({})", _as(std::string_view, nextRes.err())), auto nextRes = MusicPlayer::next(); !nextRes);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline void CmdInv::playlist(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] Not enough arguments given to "playlist"!)"), cmd.size() < 2uz);

    if (cmd[1uz] == "set")
    {
        const sys::cstr lookupName = sys::cstr::join(std::span(std::next(cmd.begin(), 2z), cmd.end()), ' ');
        _retif(CmdInv::println(history, "[log.error] Failed to set playlist, tag `{}` doesn't exist.", _as(std::string_view, lookupName)),
               MusicPlayer::playlistWithTag(sys::str(_as(std::string_view, lookupName))).empty());

        MusicPlayer::currentTag = sys::str(_as(std::string_view, lookupName));
    }
    else if (cmd[1uz] == "--index" || cmd[1uz] == "-i")
    {
        _retif(CmdInv::println(history, R"([log.error] Wrong number of arguments given to "playlist"!)"), cmd.size() != 3uz);

        char* readEnd = nullptr;                                                  // NOLINT(misc-const-correctness)
        const sys::integer<long> i = std::strtol(cmd[2uz].c_str(), &readEnd, 10); // NOLINT(google-runtime-int, readability-magic-numbers)
        _retif(CmdInv::println(history, R"([log.error] Invalid index argument given to "playlist"!)"), ssz(readEnd - cmd[2uz].data()) != sz(cmd[2uz].size()));

        const std::vector<MusicPlayer::Track>& playlist = MusicPlayer::playlistWithTag(MusicPlayer::currentTag);
        _retif(CmdInv::println(history, R"([log.error] Invalid index argument given to "playlist"!)"), i < 0 || i >= playlist.size());

        MusicPlayer::currentTrack = i32(i);
    }
    else if (cmd[1uz] == "--autoplay" || cmd[1uz] == "-a")
    {
        _retif(CmdInv::println(history, R"([log.error] Too many arguments given to "playlist"!)"), cmd.size() > 3uz);

        if (cmd.size() == 2uz)
            MusicPlayer::autoplay(!MusicPlayer::autoplay());
        else
        {
            const sys::cstr s = sys::cstr(cmd[2uz]).trim().to_lower();
            bool val = false;
            if (s == "1" || s == "true" || s == "on")
                val = true;
            else if (s == "0" || s == "false" || s == "off")
                val = false;
            else
            {
                CmdInv::println(history, R"([log.error] Invalid argument given to "playlist"!)");
                return;
            }

            MusicPlayer::autoplay(val);
        }
    }
    else
    {
        _retif(CmdInv::println(history, R"([log.error] Wrong number of arguments given to "playlist"!)"), cmd.size() != 2uz);

        bool ok = false;
        if (cmd[1uz] == "--seq")
            ok = MusicPlayer::sortPlaylistLexicographically(MusicPlayer::currentTag);
        else if (cmd[1uz] == "--revseq")
            ok = MusicPlayer::sortPlaylistReverseLexicographically(MusicPlayer::currentTag);
        else if (cmd[1uz] == "--shuffle" || cmd[1uz] == "-sh")
            ok = MusicPlayer::shufflePlaylist(MusicPlayer::currentTag);
        else
        {
            CmdInv::println(history, R"([log.error] Invalid argument given to "playlist"!)");
            return;
        }

        if (!ok) [[unlikely]]
            CmdInv::println(history, "[log.error] Failed to reorder playlist, current playlist doesn't exist (somehow)!");
    }
}
