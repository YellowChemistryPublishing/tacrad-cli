#pragma once

#include <CompilerWarnings.h>
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
#include <functional>
#include <iterator>
#include <map>
#include <miniaudio.h>
#include <new>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <taglib/fileref.h>
#include <utility>
#include <vector>

_pop_nowarn_c_cast();

#include <module/sys>
#include <module/sys.Text>

#include <Debug.h>
#include <Error.h>
#include <Metadata.h>
#include <NativeString.h>
#include <Screen.h>

namespace ui = ftxui;

class MusicPlayer final
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

    static inline std::atomic<bool> isPlaying = true; // First track starts playing!
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
    struct Track
    {
        sys::str name = u8"";
        std::filesystem::path file;

        sys::cstr artistsDisplay = "unknown";
        sys::cstr tagsDisplay = "[none]";

        friend bool operator==(const Track&, const Track&) = default;
        friend auto operator<=>(const Track&, const Track&) = default;
    };
private:
    static inline std::map<sys::str, std::vector<Track>> playlists;
    static inline std::set<sys::str> lastPlaylistReorderWasReshuffle;
public:
    MusicPlayer() = delete;

    static sys::result<float, Error> currentTime()
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);
        float ret = 0.0f;
        if (const ma_result res = ma_sound_get_cursor_in_seconds(&MusicPlayer::audio->sound, &ret); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);
        return ret;
    }
    static sys::result<float, Error> totalTime()
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);
        return MusicPlayer::audio->audioLen;
    }
    _pure_const static sys::cstr formatTime(float seconds)
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

    static sys::result<std::filesystem::path, Error> musicLookup(const std::u8string_view name)
    {
        namespace fs = std::filesystem;
        std::error_code ec;

        if (!fs::exists("music/", ec))
            return Error::DirectoryNotFound;
        if (ec)
            return Error::fromCategory(ec.category());

        const auto tryFindWithCompare = [&ec](auto&& pred) -> sys::result<fs::path, Error>
        {
            for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
            {
                if (dir.is_regular_file(ec) && pred(dir.path().stem().generic_u8string()))
                    return dir.path();
                if (ec)
                    return Error::fromCategory(ec.category());
            }

            return Error::TrackNotFound;
        };

        sys::str compare = sys::str(name);
        sys::result<fs::path, Error> res = tryFindWithCompare([&compare](const std::u8string_view trackName) { return sys::str(trackName) == compare; });
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

    static inline sys::str currentTag = u8"all";
    static inline i32 currentTrack = 0_i32;
    [[nodiscard]] static const std::vector<Track>& playlistWithTag(const std::u8string_view tag)
    {
        static const std::vector<Track> notFound;
        _retif(notFound, tag.empty());

        if (auto it = MusicPlayer::playlists.find(sys::str(tag)); it != MusicPlayer::playlists.end())
            return it->second;

        return notFound;
    }
    [[nodiscard]] static const std::vector<Track>& currentPlaylist() { return MusicPlayer::playlistWithTag(MusicPlayer::currentTag); }
    [[nodiscard]] static const std::map<sys::str, std::vector<Track>>& allPlaylists() { return MusicPlayer::playlists; }

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
                const TagLib::FileRef f(native_string(dir.path().generic_u8string()).c_str());

                std::vector<sys::str> tags { u8"all" };
                auto tagsRes = TrackMetadata::readTrackTags(f);
                if (tagsRes)
                    tags.append_range(tagsRes.move());
                if (tags.size() == 1uz)
                    tags.emplace_back(u8"uncategorized");

                auto artistsRes = TrackMetadata::readArtists(f);
                const Track track { .name = sys::str(dir.path().stem().generic_u8string()),
                                   .file = dir.path(),
                                   .artistsDisplay = artistsRes ? [&, as = artistsRes.move()]
                {
                    sys::str ret= sys::str::join(as.artists, u8"; ");
                    if (!as.feats.empty())
                        ret.append(u8" ft. ").append(sys::str::join(as.feats, u8"; "));
                    return _as(sys::cstr, sys::cstr(ret));
                }()
                                                                : sys::cstr("unknown"),
                                   .tagsDisplay = _as(sys::cstr, _as(sys::cstr, sys::str::join(tags, u8"; "))) };

                for (const sys::str& tag : tags)
                    MusicPlayer::playlists[sys::str(tag)].emplace_back(track);
            }
            if (ec)
            {
                ret.emplace_back(ec);
                ec.clear();
            }
        }

        for (auto& [_, playlist] : MusicPlayer::playlists)
            std::ranges::sort(playlist);

        if (MusicPlayer::playlists.empty())
        {
            ret.emplace_back(std::make_error_code(std::errc::no_such_file_or_directory));
            return ret;
        }

        return ret;
    }

    /// @brief Reorders current playlist with reordering function.
    /// @return Whether the playlist existed to reorder.
    [[nodiscard]] static bool reorderCurrentPlaylist(auto&& reorder, sys::str tag)
    {
        auto it = MusicPlayer::playlists.find(tag);
        _retif(false, it == MusicPlayer::playlists.end());

        std::vector<Track>& playlist = it->second;
        const MusicPlayer::Track toFind =
            MusicPlayer::currentTrack >= 0_i32 && MusicPlayer::currentTrack < playlist.size() ? playlist[sz(MusicPlayer::currentTrack)] : MusicPlayer::Track {};

        reorder(playlist, std::move(tag));

        auto foundIt = std::ranges::find_if(playlist, [&](const MusicPlayer::Track& v) { return v == toFind; });
        _retif(true, foundIt == playlist.end());

        MusicPlayer::currentTrack = std::distance(playlist.begin(), foundIt);
        return true;
    }
    [[nodiscard]] static bool shufflePlaylist(sys::str tag)
    {
        return MusicPlayer::reorderCurrentPlaylist([](std::vector<Track>& playlist, sys::str tag)
        {
            std::shuffle(playlist.begin(), playlist.end(), MusicPlayer::randEngine);
            if (MusicPlayer::playlists.contains(tag))
                MusicPlayer::lastPlaylistReorderWasReshuffle.insert(std::move(tag));
        }, std::move(tag));
    }
    [[nodiscard]] static bool sortPlaylistLexicographically(sys::str tag)
    {
        return MusicPlayer::reorderCurrentPlaylist([](std::vector<Track>& playlist, sys::str tag)
        {
            std::ranges::sort(playlist);
            MusicPlayer::lastPlaylistReorderWasReshuffle.erase(tag);
        }, std::move(tag));
    }
    [[nodiscard]] static bool sortPlaylistReverseLexicographically(sys::str tag)
    {
        return MusicPlayer::reorderCurrentPlaylist([](std::vector<Track>& playlist, sys::str tag)
        {
            std::ranges::sort(playlist, std::greater<>());
            MusicPlayer::lastPlaylistReorderWasReshuffle.erase(tag);
        }, std::move(tag));
    }

    [[nodiscard]] static sys::result<void, Error> resume()
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);

        Audio& aud = *MusicPlayer::audio;
        if (const ma_result res = ma_sound_start(&aud.sound); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);

        MusicPlayer::isPlaying = true;
        return {};
    }
    [[nodiscard]] static sys::result<void, Error> pause()
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);

        Audio& aud = *MusicPlayer::audio;
        if (const ma_result res = ma_sound_stop(&aud.sound); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);

        MusicPlayer::isPlaying = false;
        return {};
    }
    [[nodiscard]] static sys::result<void, Error> seek(float querySeconds)
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);
        Audio& aud = *MusicPlayer::audio;

        const sys::integer<ma_uint64> seekQuery(_as(float, ma_engine_get_sample_rate(&MusicPlayer::audioEngine())) * querySeconds);
        if (seekQuery < 0 || seekQuery > aud.frameLen)
            return Error::TrackSeekOutOfRange;

        if (const ma_result res = ma_sound_seek_to_pcm_frame(&aud.sound, _as(ma_uint64, seekQuery)); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);

        return {};
    }

    [[nodiscard]] static float volume() { return ma_engine_get_volume(&MusicPlayer::audioEngine()); }
    [[nodiscard]] static sys::result<void, Error> volume(float linear)
    {
        if (const ma_result res = ma_engine_set_volume(&MusicPlayer::audioEngine(), linear); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);
        return {};
    }

    [[nodiscard]] static sys::result<void, Error> startMusic(const std::filesystem::path& foundMusicFile)
    {
        if (!MusicPlayer::audio)
        {
            MusicPlayer::audio = Audio();
            MusicPlayer::hasAudio = true;
        }

        Audio& aud = *MusicPlayer::audio;
        sys::optional_destructor audDtor = [] noexcept
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
            return Error::fromAudioEngineResult(res);
        sys::optional_destructor soundDtor = [] noexcept { ma_sound_uninit(&MusicPlayer::audio->sound); };

        if (const ma_result res = ma_sound_get_length_in_pcm_frames(&aud.sound, &*aud.frameLen); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);
        if (const ma_result res = ma_sound_get_length_in_seconds(&aud.sound, &aud.audioLen); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);

        if (const ma_result res = ma_sound_set_end_callback(&aud.sound,
                                                            [](void*, ma_sound*)
        {
            if (MusicPlayer::autoplay())
                Screen().Post([]
                {
                    if (auto nextRes = MusicPlayer::next(); !nextRes) [[unlikely]]
                    {
                        debugLog("[log.error] Failed to play next track. ({})", _as(std::string_view, nextRes.err()));
                        MusicPlayer::isPlaying.store(false);
                        return;
                    }

                    MusicPlayer::isPlaying.store(true);
                });
            else
                Screen().Post([] { MusicPlayer::isPlaying.store(false); });
        }, nullptr);
            res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);

        if (MusicPlayer::isPlaying.load())
            _retif(resRes.err(), auto resRes = MusicPlayer::resume(); !resRes);

        soundDtor.release();
        audDtor.release();
        return {};
    }
    [[nodiscard]] static sys::result<void, Error> queryStartMusic(std::u8string_view query)
    {
        namespace fs = std::filesystem;

        _res_movret(const fs::path found, MusicPlayer::musicLookup(query));
    Again:
        const std::vector<Track>& playlist = MusicPlayer::currentPlaylist();
        const sz foundIndex(sz(std::distance(playlist.begin(), std::ranges::find_if(playlist, [&](const Track& music) { return music.file == found; }))));
        if (foundIndex == playlist.size() && MusicPlayer::currentTag != u8"all")
        {
            MusicPlayer::currentTag = u8"all";
            goto Again;
        }
        MusicPlayer::currentTrack = foundIndex < playlist.size() ? i32(foundIndex) : i32::sentinel();

        return MusicPlayer::startMusic(found);
    }
    [[nodiscard]] static sys::result<void, Error> stopMusic()
    {
        _retif({}, !MusicPlayer::audio);

        Audio& aud = *MusicPlayer::audio;
        const ma_result res = ma_sound_stop(&aud.sound);

        ma_sound_uninit(&aud.sound);
        MusicPlayer::audio = std::nullopt;
        MusicPlayer::hasAudio.store(false);

        if (res != MA_SUCCESS && res != MA_DEVICE_NOT_STARTED)
            return Error::fromAudioEngineResult(res);
        return {};
    }

    [[nodiscard]] static sys::result<void, Error> play()
    {
        _retif(stopRes, auto stopRes = MusicPlayer::stopMusic(); !stopRes);

        const std::vector<Track>& playlist = MusicPlayer::currentPlaylist();
        if (MusicPlayer::currentTrack < 0_i32 || MusicPlayer::currentTrack >= playlist.size())
        {
            MusicPlayer::currentTrack = 0_i32;
            _retif(Error::PlaylistEmpty, playlist.empty());

            const sys::str tag(MusicPlayer::currentTag);
            if (MusicPlayer::lastPlaylistReorderWasReshuffle.contains(tag) && !MusicPlayer::shufflePlaylist(tag))
            {
                MusicPlayer::lastPlaylistReorderWasReshuffle.erase(tag);
                return Error::PlaylistEmpty;
            }
        }

        Screen().PostEvent(ui::Event::Custom);

        return MusicPlayer::startMusic(playlist[sz(MusicPlayer::currentTrack)].file);
    }
    [[nodiscard]] static sys::result<void, Error> next()
    {
        sys::optional_destructor playingDtor = [] noexcept { MusicPlayer::isPlaying.store(false); };

        if (MusicPlayer::loaded())
        {
            _retif(stopRes, auto stopRes = MusicPlayer::stopMusic(); !stopRes);
            ++MusicPlayer::currentTrack;
        }

        _retif(playRes, auto playRes = MusicPlayer::play(); !playRes);

        playingDtor.release();
        return {};
    }
};
