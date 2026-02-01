#pragma once

#include <Preamble.h>

#include <cctype>
#include <cmath>
#include <codecvt>
#include <filesystem>
#include <locale>
#include <miniaudio.h>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <module/sys>

#include <Debug.h>
#include <Screen.h>

class MusicPlayer
{
    static std::string toLower(std::string_view str)
    {
        std::string ret;
        ret.reserve(str.size());
        for (const char c : str)
            ret.push_back(_as(char, std::tolower(c)));
        return ret;
    }

    static inline std::random_device seeder;
    static inline std::mt19937 randEngine { _as(std::mt19937, seeder()) };
    static inline std::uniform_real_distribution<float> dist { 0.0f, 1.0f };

    static ma_engine& audioEngine()
    {
        static ma_engine cctor = []
        {
            ma_engine ret;
            ma_engine_init(nullptr, &ret);
            return ret;
        }();
        static const sys::destructor ddtor = [] noexcept
        {
            MusicPlayer::stopMusic();
            ma_engine_uninit(&MusicPlayer::audioEngine());
        };
        return cctor;
    };

    static inline std::atomic<bool> isPlaying = true;
    static inline std::atomic<bool> shouldAutoplay = false;

    struct Audio
    {
        ma_sound sound {};
        std::string name;

        sys::integer<ma_uint64> prevFrame { 0 };
        sys::integer<ma_uint64> frameLen { 0 };
        float audioLen = 0.0f;
    };
    static inline std::optional<Audio> audio;
public:
    MusicPlayer() = delete;

    static float currentTime()
    {
        _retif(0.0f, !MusicPlayer::audio);
        float ret = 0.0f;
        ma_sound_get_cursor_in_seconds(&MusicPlayer::audio->sound, &ret);
        return ret;
    }
    static float totalTime()
    {
        _retif(0.0f, !MusicPlayer::audio);
        return MusicPlayer::audio->audioLen;
    }
    static std::string formatTime(float seconds) { return std::format("{}:{:02}", *i32(seconds / 60.0f), *i32(std::fmod(seconds, 60.0f))); }

    /// @brief Checks if there is music loaded.
    static bool loaded() { return MusicPlayer::audio.has_value(); }

    /// @brief Checks if music is currently playing.
    /// @note Thread-safe.
    static bool playing() { return MusicPlayer::isPlaying.load(); }
    /// @brief Checks if music should autoplay.
    /// @note Thread-safe.
    static bool autoplay() { return MusicPlayer::shouldAutoplay.load(); }
    /// @brief Sets whether music should autoplay.
    /// @note Thread-safe.
    static void autoplay(bool value) { MusicPlayer::shouldAutoplay.store(value); }

    struct FoundMusic
    {
        std::string name;
        std::filesystem::path file;
    };
    static sys::result<FoundMusic> musicLookup(std::string_view name)
    {
        namespace fs = std::filesystem;

        std::error_code ec; // NOLINT(misc-const-correctness) TODO(halloimdragon): Use this everywhere.
        const std::string compare = MusicPlayer::toLower(name);

        if (!fs::exists("music/", ec))
            return nullptr;

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
    static void seek(float querySeconds)
    {
        _retif(, !MusicPlayer::audio);
        Audio& audio = *MusicPlayer::audio;

        const sys::integer<ma_uint64> seekQuery(_as(float, ma_engine_get_sample_rate(&MusicPlayer::audioEngine())) * querySeconds);
        if (seekQuery >= 0 && seekQuery <= audio.frameLen)
            ma_sound_seek_to_pcm_frame(&audio.sound, _as(ma_uint64, seekQuery));
        else
            Debug::log(R"([log.error] Seek query out of duration of media!)");
    }
    static void volume(float linear) { ma_engine_set_volume(&MusicPlayer::audioEngine(), linear); }

    static void startMusic(std::string foundMusicName, const std::wstring& foundMusicFile)
    {
        if (!MusicPlayer::audio)
            MusicPlayer::audio = Audio();

        Audio& audio = *MusicPlayer::audio;
        if (const ma_result res = ma_sound_init_from_file_w(&MusicPlayer::audioEngine(), foundMusicFile.c_str(), 0, nullptr, nullptr, &audio.sound); res != MA_SUCCESS)
        {
            _push_nowarn_deprecated();
            Debug::log("Failed to load track `{}`, with error code {}.", std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(foundMusicFile), _as(int, res));
            _pop_nowarn_deprecated();
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
                else
                    MusicPlayer::isPlaying.store(false);
            });
        }, nullptr);

        if (MusicPlayer::isPlaying)
            MusicPlayer::resume();
    }
    static void queryStartMusic(std::string_view query)
    {
        sys::result<FoundMusic> foundRes = MusicPlayer::musicLookup(query);
        _retif(, !foundRes); // TODO(halloimdragon): Error handling.

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

        MusicPlayer::stopMusic();
        if (!MusicPlayer::audio)
            MusicPlayer::audio = Audio();

        const Audio& audio = *MusicPlayer::audio;
        std::error_code ec; // TODO(halloimdragon): Check these everywhere.

        sz ct = 0_uz;
        if (fs::exists("music/", ec))
            for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
                if (dir.is_regular_file() && dir.path().stem().string() != audio.name)
                    ++ct;

        if (ct == 0)
            return; // TODO(halloimdragon): Error handling.

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
