#pragma once

#include <Preamble.h>

#include <Debug.h>
#include <Music.h>

class CommandInvocation
{
public:
    CommandInvocation() = delete;

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
            Debug::log("[log.error] Track title argument must be given to \"play\"!");
            return;
        }

        MusicPlayer::stopMusic();

        std::string lookupName = cmd[1];
        for (auto& word : std::span(std::next(cmd.begin(), 2), cmd.end()))
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
            Debug::log("[log.error] \"resume\" takes no arguments!");
            return;
        }

        if (!MusicPlayer::loaded())
            MusicPlayer::resume();
        else
            Debug::log("[log.error] Not currently playing music! Use \"play\" and \"stop\" to change media.");
    }
    static void pause(const std::vector<std::string>& cmd)
    {
        if (cmd.size() > 1) [[unlikely]]
        {
            Debug::log("[log.error] \"pause\" takes no arguments!");
            return;
        }

        if (MusicPlayer::loaded())
            MusicPlayer::pause();
        else
            Debug::log("[log.error] Not currently playing music! Use \"play\" and \"stop\" to change media.");
    }
    static void seek(const std::vector<std::string>& cmd)
    {
        if (cmd.size() < 2) [[unlikely]]
        {
            Debug::log("[log.error] Seek position argument (in seconds) must be given to \"seek\"!");
            return;
        }
        if (cmd.size() > 2) [[unlikely]]
        {
            Debug::log("[log.error] Extra arguments given to \"seek\"!");
            return;
        }

        if (!MusicPlayer::loaded())
        {
            Debug::log("[log.error] Not currently playing music! Use \"play\" and \"stop\" to change media.");
            return;
        }

        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
        char* readEnd = nullptr;
        std::string convBytes = conv.to_bytes(cmd[1]);
        float q = std::strtof(convBytes.c_str(), &readEnd);

        if ((readEnd - convBytes.data()) != convBytes.size())
        {
            Debug::log("[log.error] Invalid index argument given to \"seek\"!");
            return;
        }

        float seekQuery = (float)ma_engine_get_sample_rate(&MusicPlayer::engine) * q;
        if (seekQuery >= 0.0f && seekQuery <= MusicPlayer::frameLen)
            ma_sound_seek_to_pcm_frame(&MusicPlayer::music, (ma_uint64)seekQuery);
        else
            Debug::log("[log.error] Seek query out of duration of media!");
    }
    static void volume(const std::vector<std::string>& cmd)
    {
        if (cmd.size() < 2) [[unlikely]]
        {
            Debug::log("[log.error] Volume argument (linear) must be given to \"vol\"!");
            return;
        }
        if (cmd.size() > 2) [[unlikely]]
        {
            Debug::log("[log.error] Extra arguments given to \"vol\"!");
            return;
        }

        std::string vStr = cmd[1];
        std::wstring wvStr;
        wvStr.reserve(vStr.size());
        for (auto c : vStr)
            wvStr.push_back(c);
        std::wistringstream ss(std::move(wvStr));
        float v;
        ss >> v;
        ma_engine_set_volume(&MusicPlayer::engine, v);
    }
    static void stop(const std::vector<std::string>& cmd)
    {
        if (MusicPlayer::loaded())
            MusicPlayer::stopMusic();
        else
            Debug::log("[log.error] Not currently playing music! Use \"play\" to start media.");
    }
    static void next(const std::vector<std::string>& cmd)
    {
        if (cmd.size() > 2)
            Debug::log("[log.error] Extra arguments given to \"playl\"!");

        MusicPlayer::next();
    }
};
