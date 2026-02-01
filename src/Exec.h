#pragma once

#include <Preamble.h>

#include <cstddef>
#include <cstdlib>
#include <format>
#include <iterator>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <module/sys>

#include <Exec.inl>
#include <Music.h>
#include <Screen.h>

inline void CommandInvocation::help(const std::vector<std::string>& cmd)
{
    if (cmd.size() > 1) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] "help" takes no arguments!)");
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
        if (!query.exactCount)
            cmdName.append(" ...");
        CommandInvocation::println("{}\n    {}\n    {}", cmdName, query.usage, query.desc);
    }
}
inline void CommandInvocation::clear(const std::vector<std::string>& cmd)
{
    if (cmd.size() > 1) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] "clear" takes no arguments!)");
        return;
    }

    CommandInvocation::clearHistory();
}
inline void CommandInvocation::quit(const std::vector<std::string>& cmd)
{
    if (cmd.size() > 1) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] "exit" takes no arguments!)");
        return;
    }

    Screen().Exit();
}

inline void CommandInvocation::togglePlayingOrPlay(const std::vector<std::string>& cmd)
{
    if (cmd.size() == 1 && MusicPlayer::loaded())
    {
        if (MusicPlayer::playing())
        {
            if (!MusicPlayer::pause())
                CommandInvocation::println("[log.error] Failed to pause track.");
        }
        else
        {
            if (!MusicPlayer::resume())
                CommandInvocation::println("[log.error] Failed to resume track.");
        }
    }
    else
        CommandInvocation::play(cmd);
}
inline void CommandInvocation::resumeOrPlay(const std::vector<std::string>& cmd)
{
    if (cmd.size() == 1 && !MusicPlayer::playing())
    {
        if (!MusicPlayer::resume())
            CommandInvocation::println("[log.error] Failed to resume track.");
    }
    else
        CommandInvocation::play(cmd);
}
inline void CommandInvocation::play(const std::vector<std::string>& cmd)
{
    if (cmd.size() < 2) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] Track title argument must be given to "play"!")");
        return;
    }

    if (!MusicPlayer::stopMusic())
        CommandInvocation::println("[log.error] Failed to stop track.");

    std::string lookupName = cmd[1];
    for (const auto& word : std::span(std::next(cmd.begin(), 2), cmd.end()))
    {
        lookupName.push_back(' ');
        lookupName.append(word);
    }

    if (!MusicPlayer::queryStartMusic(lookupName))
        CommandInvocation::println("[log.error] Failed to start track.");
}
inline void CommandInvocation::resume(const std::vector<std::string>& cmd)
{
    if (cmd.size() > 1) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] "resume" takes no arguments!)");
        return;
    }

    if (!MusicPlayer::resume())
        CommandInvocation::println(R"([log.error] Not currently playing music! Use "play" and "stop" to change media.)");
}
inline void CommandInvocation::pause(const std::vector<std::string>& cmd)
{
    if (cmd.size() > 1) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] "pause" takes no arguments!)");
        return;
    }

    if (!MusicPlayer::pause())
        CommandInvocation::println(R"([log.error] Not currently playing music! Use "play" and "stop" to change media.)");
}
inline void CommandInvocation::seek(const std::vector<std::string>& cmd)
{
    if (cmd.size() < 2) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] Seek position argument (in seconds) must be given to "seek"!)");
        return;
    }
    if (cmd.size() > 2) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] Extra arguments given to "seek"!)");
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

    if (!MusicPlayer::seek(q))
        CommandInvocation::println("[log.error] Failed to seek track.");
}
inline void CommandInvocation::volume(const std::vector<std::string>& cmd)
{
    if (cmd.size() < 2) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] Volume argument (linear) must be given to "vo"!)");
        return;
    }
    if (cmd.size() > 2) [[unlikely]]
    {
        CommandInvocation::println(R"([log.error] Extra arguments given to "vo"!)");
        return;
    }

    float v = 1.0f; // NOLINT(misc-const-correctness)
    std::istringstream(cmd[1]) >> v;
    if (!MusicPlayer::volume(v))
        CommandInvocation::println("[log.error] Failed to set volume.");
}
inline void CommandInvocation::stop(const std::vector<std::string>&)
{
    if (MusicPlayer::loaded())
    {
        if (!MusicPlayer::stopMusic())
            CommandInvocation::println("[log.error] Failed to stop track.");
    }
    else
        CommandInvocation::println(R"([log.error] Not currently playing music! Use "play" to start media.)");
}
inline void CommandInvocation::next(const std::vector<std::string>& cmd)
{
    if (cmd.size() > 1)
        CommandInvocation::println(R"([log.error] Extra arguments given to "next"!)");

    if (!MusicPlayer::next())
        CommandInvocation::println("[log.error] Failed to play next track.");
}
