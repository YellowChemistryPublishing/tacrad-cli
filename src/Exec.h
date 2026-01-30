#pragma once

#include <cstddef>
#include <cstdlib>
#include <format>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include <module/sys>

#include <Debug.h>
#include <Music.h>

struct InvocationEntry
{
    std::string command;
    std::string output;
};

class CommandInvocation
{
    static inline std::vector<InvocationEntry> history;
public:
    CommandInvocation() = delete;

    static void pushCommand(std::string cmd) { CommandInvocation::history.push_back(InvocationEntry { .command = std::move(cmd), .output = "" }); }
    template <typename... Args>
    static void println(std::format_string<Args...> fmt, Args&&... args)
    {
        _retif(, CommandInvocation::history.empty());
        CommandInvocation::history.back().output.append(std::format(fmt, std::forward<Args>(args)...)).push_back('\n');
    }
    static std::vector<InvocationEntry>& rawHistory() { return CommandInvocation::history; }

    static void playOrTogglePlaying(const std::vector<std::string>& cmd)
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

        if (!MusicPlayer::loaded())
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

        char* readEnd = nullptr;
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

        float v = 1.0f;
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
};
