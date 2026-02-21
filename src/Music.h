#pragma once

#include <CompilerWarnings.h>
#include <Metadata.h>
#include <Music.h>
#include <StringEx.h>
_push_nowarn_c_cast();
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>
#include <iterator>
#include <miniaudio.h>
#include <new>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
_pop_nowarn_c_cast();

#include <module/sys>
#include <module/sys.Text>

#include <CmdInv.inl>
#include <Debug.h>
#include <NativeString.h>
#include <Screen.h>

namespace ui = ftxui;

class MusicPlayer
{
    static inline std::random_device seeder;
    static inline std::mt19937 randEngine { _as(std::mt19937, MusicPlayer::seeder()) };
    static inline std::uniform_real_distribution<float> dist { 0.0f, 1.0f };

    static ma_engine& audioEngine()
    {
        static ma_engine cctor = [] noexcept
        {
            ma_engine ret;
            if (ma_result res = ma_engine_init(nullptr, &ret); res != MA_SUCCESS)
            {
                try
                {
                    debugLog("[log.error] Failed to initialize audio engine, with error code {}.", _as(int, res));
                }
                catch (...)
                {
                    std::exit(0xBADC0DE); // NOLINT(concurrency-mt-unsafe, readability-magic-numbers)
                }
                std::memset(&ret, 0, sizeof(ma_engine));
            }
            return ret;
        }();
        static const sys::destructor ddtor = [] noexcept
        {
            try
            {
                (void)MusicPlayer::stopMusic();
            }
            catch (const std::length_error& ex)
            {
                debugLog("[log.error] Failed to stop music, `std::length_error` raised with message {}.", ex.what());
            }
            catch (const std::bad_alloc& ex)
            {
                debugLog("[log.error] Failed to stop music, `std::bad_alloc` raised with message {}.", ex.what());
            }
            catch (...)
            {
                debugLog("[log.error] Failed to stop music, unknown exception raised.");
            }
            ma_engine_uninit(&MusicPlayer::audioEngine());
        };
        return cctor;
    };

    static inline std::atomic<bool> isPlaying = false;
    static inline std::atomic<bool> shouldAutoplay = true;

    struct Audio
    {
        ma_sound sound {}; // _MUST_ be valid.

        sys::integer<ma_uint64> prevFrame { 0 };
        sys::integer<ma_uint64> frameLen { 0 };
        float audioLen = -1.0f;
    };
    static inline std::optional<Audio> audio;
    static inline std::atomic<bool> hasAudio = false;
public:
    struct FoundMusic
    {
        sys::str name = u8"";
        std::filesystem::path file {};

        std::string artistsDisplay = "unknown";
        std::string tagsDisplay = "[none]";

        friend bool operator==(const FoundMusic&, const FoundMusic&) = default;
        friend auto operator<=>(const FoundMusic&, const FoundMusic&) = default;
    };
private:
    static inline std::map<std::u8string, std::vector<FoundMusic>> playlists;
    static inline std::set<std::u8string> lastPlaylistReorderWasReshuffle;
public:
    MusicPlayer() = delete;

    static sys::result<float, ma_result> currentTime()
    {
        _retif(MA_INVALID_OPERATION, !MusicPlayer::audio);
        float ret = 0.0f;
        if (ma_result res = ma_sound_get_cursor_in_seconds(&MusicPlayer::audio->sound, &ret); res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to get cursor in seconds, with error code {}.", _as(int, res));
            return res;
        return ret;
    }
    static sys::result<float, ma_result> totalTime()
    {
        _retif(MA_INVALID_OPERATION, !MusicPlayer::audio);
        return MusicPlayer::audio->audioLen;
    }
    _pure_const static std::string formatTime(float seconds)
    {
        return std::format("{}:{:02}", *i32(seconds / 60.0f), *i32(std::fmod(seconds, 60.0f))); // NOLINT(readability-magic-numbers)
    }

    /// @brief Checks if there is music loaded.
    /// @note Thread-safe.
    [[nodiscard]] static bool loaded() { return MusicPlayer::hasAudio.load(); }
    /// @brief Checks if music is currently playing.
    /// @note Thread-safe.
    [[nodiscard]] static bool playing() { return MusicPlayer::isPlaying.load(); }
    /// @brief Checks if music should autoplay.
    /// @note Thread-safe.
    [[nodiscard]] static bool autoplay() { return MusicPlayer::shouldAutoplay.load(); }
    /// @brief Sets whether music should autoplay.
    /// @note Thread-safe.
    static void autoplay(const bool value) { MusicPlayer::shouldAutoplay.store(value); }

    static sys::result<std::filesystem::path, std::error_code> musicLookup(std::u8string_view name)
    {
        namespace fs = std::filesystem;
        std::error_code ec; // NOLINT(misc-const-correctness) TODO(halloimdragon): Use this everywhere.

        if (!fs::exists("music/", ec))
            return std::make_error_code(std::errc::not_a_directory);
        if (ec)
            // CmdInv::println("[log.error] Failed to check if music directory exists, with error code {}.", ec.value());
            return ec;

        const auto tryFindWithCompare = [&ec](auto&& pred) -> sys::result<fs::path, std::error_code>
        {
            for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
            {
                if (dir.is_regular_file(ec) && pred(dir.path().stem().generic_u8string()))
                    return dir.path();
                if (ec)
                    // CmdInv::println("[log.error] Failed to iterate through music directory, with error code {}.", ec.value());
                    return ec;
            }

            return std::make_error_code(std::errc::no_such_file_or_directory);
        };

        sys::str compare = sys::str(name);
        sys::result<fs::path, std::error_code> res = tryFindWithCompare([&compare](const std::u8string_view trackName) { return sys::str(trackName) == compare; });
        _retif(res.move(), res);

        compare.fold();
        res = tryFindWithCompare([&compare](const std::u8string_view trackName) { return sys::str(trackName).fold() == compare; });
        _retif(res.move(), res);
        res = tryFindWithCompare([&compare](const std::u8string_view trackName) { return sys::str(trackName).fold().starts_with(compare); });
        _retif(res.move(), res);
        res = tryFindWithCompare([&compare](const std::u8string_view trackName) { return sys::str(trackName).fold().contains(compare); });
        _retif(res.move(), res);

        return res;
    }

    static inline sys::str playlistTag = u8"all";
    static inline i32 currentTrack = 0_i32;
    [[nodiscard]] static const std::vector<FoundMusic>& playlistWithTag(const std::u8string_view tag)
    {
        static const std::vector<FoundMusic> notFound;
        _retif(notFound, tag.empty());

        if (auto it = MusicPlayer::playlists.find(std::u8string(tag)); it != MusicPlayer::playlists.end())
            return it->second;

        return notFound;
    }
    [[nodiscard]] static const std::vector<FoundMusic>& currentPlaylist() { return MusicPlayer::playlistWithTag(MusicPlayer::playlistTag); }
    [[nodiscard]] static const std::map<std::u8string, std::vector<FoundMusic>>& allPlaylists() { return MusicPlayer::playlists; }

    static std::vector<std::error_code> searchForTracks()
    {
        namespace fs = std::filesystem;

        std::error_code ec;
        MusicPlayer::playlists.clear();

        std::vector<std::error_code> ret;
        for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
        {
            if (dir.is_regular_file(ec))
            {
                TagLib::FileRef f(native_string(dir.path().generic_u8string()).c_str());

                std::vector<sys::str> tags { u8"all" };
                auto tagsRes = TrackMetadata::readTrackTags(f);
                if (tagsRes)
                    tags.append_range(tagsRes.move());
                if (tags.size() == 1uz)
                    tags.emplace_back(u8"uncategorized");

                sys::result<TrackMetadata::TrackArtists> artistsRes = TrackMetadata::readArtists(f);
                FoundMusic track { .name = sys::str(dir.path().stem().generic_u8string()),
                                   .file = dir.path(),
                                   .artistsDisplay = artistsRes ? [&, as = artistsRes.move()]
                {
                    sys::str ret= sys::str::join(as.artists, u8"; ");
                    if (!as.feats.empty())
                        ret.append(u8" ft. ").append(sys::str::join(as.feats, u8"; "));
                    return _as(std::string, sys::cstr(ret));
                }()
                                                                : std::string("unknown"),
                                   .tagsDisplay = _as(std::string, _as(sys::cstr, sys::str::join(tags, u8"; "))) };

                for (const sys::str& tag : tags)
                    MusicPlayer::playlists[std::u8string(tag)].emplace_back(track);
            }
            if (ec)
            {
                // CmdInv::println("[log.warn] Couldn't fully iterate through music directory, got error code {}.", ec.value());
                ret.emplace_back(ec);
                ec.clear();
            }
        }

        if (MusicPlayer::playlists.empty())
        {
            // CmdInv::println("[log.warn] Couldn't find any tracks to play! (Did you add any under `music/`?)");
            ret.emplace_back(std::make_error_code(std::errc::no_such_file_or_directory));
            return ret;
        }

        for (auto& [_, playlist] : MusicPlayer::playlists)
            std::ranges::sort(playlist);

        return ret;
    }

    [[nodiscard]] static bool rearrangeCurrentPlaylist(auto&& reorder, std::u8string tag)
    {
        auto it = MusicPlayer::playlists.find(tag);
        _retif(false, it == MusicPlayer::playlists.end());

        std::vector<FoundMusic>& playlist = it->second;
        const MusicPlayer::FoundMusic toFind =
            MusicPlayer::currentTrack >= 0_i32 && MusicPlayer::currentTrack < playlist.size() ? playlist[sz(MusicPlayer::currentTrack)] : MusicPlayer::FoundMusic {};

        reorder(playlist, std::move(tag));

        auto foundIt = std::ranges::find_if(playlist, [&](const MusicPlayer::FoundMusic& v) { return v == toFind; });
        _retif(true, foundIt == playlist.end());

        MusicPlayer::currentTrack = std::distance(playlist.begin(), foundIt);
        return true;
    }
    [[nodiscard]] static bool shufflePlaylist(std::u8string tag)
    {
        return MusicPlayer::rearrangeCurrentPlaylist([](std::vector<FoundMusic>& playlist, std::u8string tag)
        {
            std::shuffle(playlist.begin(), playlist.end(), MusicPlayer::randEngine);
            if (MusicPlayer::playlists.contains(tag))
                MusicPlayer::lastPlaylistReorderWasReshuffle.insert(std::move(tag));
        }, std::move(tag));
    }
    [[nodiscard]] static bool sortPlaylistLexicographically(std::u8string tag)
    {
        return MusicPlayer::rearrangeCurrentPlaylist([](std::vector<FoundMusic>& playlist, std::u8string tag)
        {
            std::ranges::sort(playlist);
            MusicPlayer::lastPlaylistReorderWasReshuffle.erase(tag);
        }, std::move(tag));
    }
    [[nodiscard]] static bool sortPlaylistReverseLexicographically(std::u8string tag)
    {
        return MusicPlayer::rearrangeCurrentPlaylist([](std::vector<FoundMusic>& playlist, std::u8string tag)
        {
            std::ranges::sort(playlist, std::greater<>());
            MusicPlayer::lastPlaylistReorderWasReshuffle.erase(tag);
        }, std::move(tag));
    }

    [[nodiscard]] static sys::result<void, ma_result> resume()
    {
        _retif(MA_SUCCESS, !MusicPlayer::audio);

        Audio& aud = *MusicPlayer::audio;
        if (ma_result res = ma_sound_start(&aud.sound); res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to resume track, with error code {}.", _as(int, res));
            return res;

        MusicPlayer::isPlaying = true;
        return {};
    }
    [[nodiscard]] static sys::result<void, ma_result> pause()
    {
        _retif(MA_INVALID_OPERATION, !MusicPlayer::audio);

        Audio& aud = *MusicPlayer::audio;
        if (ma_result res = ma_sound_stop(&aud.sound); res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to pause track, with error code {}.", _as(int, res));
            return res;

        MusicPlayer::isPlaying = false;
        return {};
    }
    [[nodiscard]] static sys::result<void, ma_result> seek(float querySeconds)
    {
        _retif(MA_INVALID_OPERATION, !MusicPlayer::audio);
        Audio& aud = *MusicPlayer::audio;

        const sys::integer<ma_uint64> seekQuery(_as(float, ma_engine_get_sample_rate(&MusicPlayer::audioEngine())) * querySeconds);
        if (seekQuery < 0 || seekQuery > aud.frameLen)
            // CmdInv::println("[log.error] Seek query out of duration of media!");
            return MA_OUT_OF_RANGE;

        if (ma_result res = ma_sound_seek_to_pcm_frame(&aud.sound, _as(ma_uint64, seekQuery)); res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to seek track, with error code {}.", _as(int, res));
            return res;

        return {};
    }

    [[nodiscard]] static float volume() { return ma_engine_get_volume(&MusicPlayer::audioEngine()); }
    [[nodiscard]] static sys::result<void, ma_result> volume(float linear)
    {
        if (ma_result res = ma_engine_set_volume(&MusicPlayer::audioEngine(), linear); res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to set volume, with error code {}.", _as(int, res));
            return res;

        return {};
    }

    [[nodiscard]] static sys::result<void, ma_result> startMusic(const std::filesystem::path& foundMusicFile)
    {
        if (!MusicPlayer::audio)
        {
            MusicPlayer::audio = Audio();
            MusicPlayer::hasAudio = true;
        }

        Audio& aud = *MusicPlayer::audio;
        sys::optional_destructor aud_dtor = [] noexcept
        {
            MusicPlayer::audio = std::nullopt;
            MusicPlayer::hasAudio = false;
        };

#if _libcxxext_os_windows
        if (const ma_result res = ma_sound_init_from_file_w(&MusicPlayer::audioEngine(), foundMusicFile.c_str(), MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, nullptr, &aud.sound);
#else
        if (const ma_result res =
                ma_sound_init_from_file(&MusicPlayer::audioEngine(), foundMusicFile.string().c_str(), MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, nullptr, &aud.sound);
#endif
            res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to load track `{}`, with error code {}.", _as(sys::cstr, sys::str(fs::path(foundMusicFile).generic_u8string())),
            //                            _as(int, res));
            return res;
        sys::optional_destructor sound_dtor = [] noexcept { ma_sound_uninit(&MusicPlayer::audio->sound); };

        if (ma_result res = ma_sound_get_length_in_pcm_frames(&aud.sound, &*aud.frameLen); res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to get track length in PCM frames, with error code {}.", _as(int, res));
            return res;
        if (ma_result res = ma_sound_get_length_in_seconds(&aud.sound, &aud.audioLen); res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to get track length in seconds, with error code {}.", _as(int, res));
            return res;

        if (ma_result res = ma_sound_set_end_callback(&aud.sound,
                                                      [](void*, ma_sound*)
        {
            if (MusicPlayer::autoplay())
                Screen().Post([]
                {
                    if (!MusicPlayer::next()) [[unlikely]]
                    {
                        // CmdInv::println("[log.error] Failed to play next track.");
                        MusicPlayer::isPlaying.store(false);
                        return;
                    }

                    MusicPlayer::isPlaying.store(true);
                });
            else
                Screen().Post([] { MusicPlayer::isPlaying.store(false); });
        }, nullptr);
            res != MA_SUCCESS)
            // CmdInv::println("[log.error] Failed to set track end callback, with error code {}.", _as(int, res));
            return res;

        if (MusicPlayer::isPlaying.load() && !MusicPlayer::resume())
            // CmdInv::println("[log.error] Failed to resume track.");
            return MA_ERROR;

        sound_dtor.release();
        aud_dtor.release();
        return {};
    }
    [[nodiscard]] static sys::result<void, ma_result> queryStartMusic(std::u8string_view query)
    {
        namespace fs = std::filesystem;

        sys::result<fs::path, std::error_code> foundRes = MusicPlayer::musicLookup(query);
        _retif(MA_ERROR, !foundRes);

        const fs::path found = foundRes.move();
        const std::vector<FoundMusic>& playlist = MusicPlayer::currentPlaylist();
    Again:
        const sz foundIndex(std::distance(playlist.begin(), std::ranges::find_if(playlist, [&found](const FoundMusic& music) { return music.file == found; })));
        if (MusicPlayer::playlistTag != u8"all" && foundIndex == playlist.size())
        {
            MusicPlayer::playlistTag = u8"all";
            goto Again;
        }
        MusicPlayer::currentTrack = foundIndex < playlist.size() ? i32(foundIndex) : i32::sentinel();

        return MusicPlayer::startMusic(found);
    }
    [[nodiscard]] static sys::result<void, ma_result> stopMusic()
    {
        _retif({}, !MusicPlayer::audio);

        Audio& aud = *MusicPlayer::audio;
        ma_result res = ma_sound_stop(&aud.sound);
        // if (res != MA_SUCCESS) [[unlikely]]
        // CmdInv::println("[log.warn] Couldn't stop track, with error code {}.", _as(int, res));

        ma_sound_uninit(&aud.sound);
        MusicPlayer::audio = std::nullopt;
        MusicPlayer::hasAudio.store(false);
        MusicPlayer::isPlaying.store(false);

        if (res != MA_DEVICE_NOT_STARTED)
            return res;
        return {};
    }

    [[nodiscard]] static sys::result<void, ma_result> play()
    {
        auto stopRes = MusicPlayer::stopMusic();
        _retif(stopRes, !stopRes);

        const std::vector<FoundMusic>& playlist = MusicPlayer::currentPlaylist();
        if (MusicPlayer::currentTrack < 0_i32 || MusicPlayer::currentTrack >= playlist.size())
        {
            MusicPlayer::currentTrack = 0_i32;
            _retif(MA_INVALID_OPERATION, playlist.empty());

            std::u8string tag(MusicPlayer::playlistTag);
            if (MusicPlayer::lastPlaylistReorderWasReshuffle.contains(tag) && !MusicPlayer::shufflePlaylist(std::move(tag)))
                // CmdInv::println("[log.warn] Couldn't to re-shuffle playlist! (Was at end of shuffled playlist.)");
                return MA_ERROR;
        }

        Screen().PostEvent(ui::Event::Custom);

        return MusicPlayer::startMusic(playlist[sz(MusicPlayer::currentTrack)].file);
    }
    [[nodiscard]] static sys::result<void, ma_result> next()
    {
        const bool wasPlaying = MusicPlayer::playing();

        if (MusicPlayer::loaded())
        {
            auto stopRes = MusicPlayer::stopMusic();
            _retif(stopRes, !stopRes);
            ++MusicPlayer::currentTrack;
        }

        auto playRes = MusicPlayer::play();
        _retif(playRes, !playRes);
        if (!wasPlaying)
        {
            auto resRes = MusicPlayer::resume();
            _retif(resRes, !resRes);
        }

        return {};
    }
};
