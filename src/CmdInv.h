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

inline void CmdInv::help(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] "help" takes no arguments!)"), cmd.size() > 1);

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
inline void CmdInv::clear(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] "clear" takes no arguments!)"), cmd.size() > 1);
    CmdInv::clearHistory(history);
}
inline void CmdInv::quit(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] "exit" takes no arguments!)"), cmd.size() > 1);
    Screen().Exit();
}

inline void CmdInv::togglePlayingOrPlay(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    if (cmd.size() == 1 && MusicPlayer::loaded())
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
    _retif(CmdInv::println(history, R"([log.error] Track title argument must be given to "play"!")"), cmd.size() < 2);
    _retif(CmdInv::println(history, "[log.error] Failed to stop track. ({})", _as(std::string_view, stopRes.err())), auto stopRes = MusicPlayer::stopMusic(); !stopRes);

    const sys::cstr lookupName = sys::cstr::join(std::span(std::next(cmd.begin(), 1), cmd.end()), ' ');
    _retif(CmdInv::println(history, "[log.error] Failed to start track. ({})", _as(std::string_view, startRes.err())),
           auto startRes = MusicPlayer::queryStartMusic(sys::str(_as(std::string_view, lookupName)));
           !startRes);
}
inline void CmdInv::resume(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] "resume" takes no arguments!)"), cmd.size() > 1);
    _retif(CmdInv::println(history, R"([log.error] Not currently playing music! Use "play" and "stop" to change media. ({}))", _as(std::string_view, resRes.err())),
           auto resRes = MusicPlayer::resume();
           !resRes);
}
inline void CmdInv::pause(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] "pause" takes no arguments!)"), cmd.size() > 1);
    _retif(CmdInv::println(history, R"([log.error] Not currently playing music! Use "play" and "stop" to change media. ({}))", _as(std::string_view, pauseRes.err())),
           auto pauseRes = MusicPlayer::pause();
           !pauseRes);
}
inline void CmdInv::seek(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] Seek position argument (in seconds) must be given to "seek"!)"), cmd.size() < 2);
    _retif(CmdInv::println(history, R"([log.error] Extra arguments given to "seek"!)"), cmd.size() > 2);
    _retif(CmdInv::println(history, R"([log.error] Not currently playing music! Use "play" and "stop" to change media.)"), !MusicPlayer::loaded());

    char* readEnd = nullptr; // NOLINT(misc-const-correctness)
    const float q = std::strtof(cmd[1].c_str(), &readEnd);
    _retif(CmdInv::println(history, R"([log.error] Invalid index argument given to "seek"!)"), readEnd - cmd[1].data() != _as(ptrdiff_t, cmd[1].size()));
    _retif(CmdInv::println(history, "[log.error] Failed to seek track. ({})", _as(std::string_view, seekRes.err())), auto seekRes = MusicPlayer::seek(q); !seekRes);
}
inline void CmdInv::volume(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] Volume argument (linear) must be given to "vol"!)"), cmd.size() < 2);
    _retif(CmdInv::println(history, R"([log.error] Extra arguments given to "vol"!)"), cmd.size() > 2);

    float v = 1.0f; // NOLINT(misc-const-correctness)
    std::istringstream(_as(std::string, cmd[1])) >> v;
    _retif(CmdInv::println(history, "[log.error] Failed to set volume. ({})", _as(std::string_view, volRes.err())), auto volRes = MusicPlayer::volume(v); !volRes);
}
inline void CmdInv::stop(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>&)
{
    if (MusicPlayer::loaded())
        _retif(CmdInv::println(history, "[log.error] Failed to stop track. ({})", _as(std::string_view, stopRes.err())), auto stopRes = MusicPlayer::stopMusic(); !stopRes);
    else
        CmdInv::println(history, R"([log.error] Not currently playing music! Use "play" to start media.)");
}
inline void CmdInv::next(std::vector<CmdInv::Entry>& history, const std::vector<sys::cstr>& cmd)
{
    _retif(CmdInv::println(history, R"([log.error] Extra arguments given to "next"!)"), cmd.size() > 1);
    _retif(CmdInv::println(history, "[log.error] Failed to play next track. ({})", _as(std::string_view, nextRes.err())), auto nextRes = MusicPlayer::next(); !nextRes);
}
