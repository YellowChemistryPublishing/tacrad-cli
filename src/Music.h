#pragma once

#include <Preamble.h>

#include <codecvt>
#include <filesystem>
#include <miniaudio.h>
#include <optional>
#include <random>

#include <module/sys>

#include <Screen.h>

class MusicPlayer
{
    static std::string toLower(std::string_view str)
    {
        std::string ret;
        ret.reserve(str.size());
        for (char c : str)
            ret.push_back(_as(char, std::tolower(c)));
        return ret;
    }

    static inline std::random_device seeder;
    static inline std::mt19937 randEngine { seeder() };
    static inline std::uniform_real_distribution<float> dist { 0.0f, 1.0f };

    static ma_engine& audioEngine()
    {
        static ma_engine cctor = []
        {
            ma_engine ret;
            ma_engine_init(nullptr, &ret);
            return ret;
        }();
        static sys::destructor ddtor = [] noexcept
        {
            MusicPlayer::stopMusic();
            ma_engine_uninit(&MusicPlayer::audioEngine());
        };
        return cctor;
    };

    static inline bool isPlaying = false;
    static inline bool shouldAutoplay = false;

    struct Audio
    {
        ma_sound sound;
        std::string name;

        u64 prevFrame = 0_u64;
        u64 frameLen = 0_u64;
        float audioLen = 0.0f;
    };
    static inline std::optional<Audio> audio;
public:
    MusicPlayer() = delete;

    static bool loaded() { return MusicPlayer::audio.has_value(); }
    static bool playing() { return MusicPlayer::isPlaying; }
    static bool autoplay() { return MusicPlayer::shouldAutoplay; }
    static void autoplay(bool value) { MusicPlayer::shouldAutoplay = value; }

    struct FoundMusic
    {
        std::string name;
        std::filesystem::path file;
    };
    static sys::result<FoundMusic> musicLookup(std::string_view name)
    {
        namespace fs = std::filesystem;

        std::error_code ec; // TODO: Use this everywhere.
        std::string compare = MusicPlayer::toLower(name);

        if (fs::exists("music/"))
        {
            for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
            {
                if (dir.is_regular_file(ec))
                {
                    if (MusicPlayer::toLower(dir.path().stem().string()) == compare)
                        return FoundMusic { .name = dir.path().stem().string(), .file = dir.path() };
                }
            }
            for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
            {
                if (dir.is_regular_file(ec))
                {
                    if (MusicPlayer::toLower(dir.path().stem().string()).starts_with(compare))
                        return FoundMusic { .name = dir.path().stem().string(), .file = dir.path() };
                }
            }
            for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
            {
                if (dir.is_regular_file(ec))
                {
                    if (MusicPlayer::toLower(dir.path().stem().string()).contains(compare))
                        return FoundMusic { .name = dir.path().stem().string(), .file = dir.path() };
                }
            }
        }

        return nullptr;
    }

    static void resume()
    {
        _retif(, !MusicPlayer::audio);

        Audio& audio = *MusicPlayer::audio;
        ma_sound_start(&audio.sound);
        MusicPlayer::isPlaying = true;
    }
    static void pause()
    {
        _retif(, !MusicPlayer::audio);

        Audio& audio = *MusicPlayer::audio;
        ma_sound_stop(&audio.sound);
        MusicPlayer::isPlaying = false;
    }

    static void startMusic(std::string foundMusicName, const std::wstring& foundMusicFile)
    {
        if (!MusicPlayer::audio)
            MusicPlayer::audio = Audio();

        Audio& audio = *MusicPlayer::audio;
        if (ma_result res = ma_sound_init_from_file_w(&MusicPlayer::audioEngine(), foundMusicFile.c_str(), 0, nullptr, nullptr, &audio.sound); res != MA_SUCCESS)
        {
            Debug::log("Failed to load track `{}`, with error code {}.", std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(foundMusicFile), _as(int, res));
            return;
        }

        ma_sound_get_length_in_pcm_frames(&audio.sound, &*audio.frameLen);
        ma_sound_get_length_in_seconds(&audio.sound, &audio.audioLen);
        audio.name = std::move(foundMusicName);

        ma_sound_set_end_callback(&audio.sound, [](void*, ma_sound*)
        {
            Screen().Post([]
            {
                if (MusicPlayer::autoplay())
                    MusicPlayer::tryPlayNextShuffle();
            });
        }, nullptr);

        if (MusicPlayer::isPlaying)
            MusicPlayer::resume();
    }
    static void queryStartMusic(std::string_view query)
    {
        sys::result<FoundMusic> foundRes = MusicPlayer::musicLookup(query);
        _retif(, !foundRes); // TODO: Error handling.

        const auto [foundMusicName, foundMusicFile] = foundRes.move();
        startMusic(foundMusicName, foundMusicFile.wstring());
    }
    static void stopMusic()
    {
        _retif(, !MusicPlayer::audio);

        Audio& audio = *MusicPlayer::audio;
        ma_sound_stop(&audio.sound);
        ma_sound_uninit(&audio.sound);
        MusicPlayer::audio = std::nullopt;
    }

    static void next()
    {
        MusicPlayer::stopMusic();
        MusicPlayer::tryPlayNextShuffle();
    }
    static void tryPlayNextShuffle()
    {
        namespace fs = std::filesystem;

        if (!MusicPlayer::audio)
            MusicPlayer::audio = Audio();

        Audio& audio = *MusicPlayer::audio;
        std::error_code ec; // TODO: Check these everywhere.

        size_t ct = 0;
        if (fs::exists("music/", ec))
            for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
                if (dir.is_regular_file() && dir.path().stem().string() != audio.name)
                    ++ct;

        if (ct == 0)
            return; // TODO: Error handling.

        while (true)
        {
            for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
            {
                if (dir.is_regular_file() && dir.path().stem().string() != audio.name && MusicPlayer::dist(randEngine) < 1.0f / _as(float, ct))
                {
                    MusicPlayer::startMusic(dir.path().stem().string(), dir.path().wstring());
                    return;
                }
            }
        }
    }
};
