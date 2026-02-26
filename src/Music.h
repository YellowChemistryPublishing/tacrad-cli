#pragma once

/// @file Music.h

#include <CompilerWarnings.h>
_push_nowarn_c_cast(); ///< @private
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

_pop_nowarn_c_cast(); ///< @private

#include <module/sys>
#include <module/sys.Text>

#include <Debug.h>
#include <Error.h>
#include <Metadata.h>
#include <NativeString.h>
#include <Screen.h>

namespace ui = ftxui;

/// @brief Music track loading and playing functionality.
/// @note Static class.
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
    /// @brief Music track metadata.
    struct Track
    {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)

        std::filesystem::path file; ///< Path to the audio file.

        sys::cstr titleDisplay = "[empty]";   ///< Rendered title of the track.
        sys::cstr subtitleDisplay = "";       ///< Rendered subtitle of the track.
        sys::cstr artistsDisplay = "unknown"; ///< Rendered artists of the track.
        sys::cstr tagsDisplay = "[none]";     ///< Rendered tags of the track.
        sys::cstr akaDisplay = "";            ///< Rendered aka. of the track.

        std::vector<sys::str> aka; ///< Aliases for the track.

        // NOLINTEND(misc-non-private-member-variables-in-classes)

        friend bool operator==(const Track&, const Track&) = default;
        friend auto operator<=>(const Track&, const Track&) = default;

        /// @brief Retrieve the full title of the track.
        [[nodiscard]] sys::cstr fullDisplayTitle() const
        {
            sys::cstr ret = this->titleDisplay;
            if (!this->subtitleDisplay.empty())
                ret.append(u8' ').append(this->subtitleDisplay);
            return ret;
        }
    };
private:
    static inline std::map<sys::str, std::vector<Track>> playlists;
    static inline std::set<sys::str> lastPlaylistReorderWasReshuffle;
public:
    MusicPlayer() = delete;

    /// @brief Retrieve the index of `track` in `playlist`.
    /// @note If `track` is not found in `playlist`, `i32::highest()` is returned.
    static i32 indexOf(const std::vector<Track>& playlist, const Track& track)
    {
        const auto foundIt = std::ranges::find_if(playlist, [&](const Track& v) { return v == track; });
        _retif(i32::highest(), foundIt == playlist.end());
        return std::distance(playlist.begin(), foundIt);
    }

    /// @brief Search and update `MusicPlayer::playlists` with music files recursively in the music directory.
    [[nodiscard]] static std::vector<std::error_code> searchForTracks()
    {
        namespace fs = std::filesystem;

        const std::vector<Track>& playlist = MusicPlayer::playlistWithTag(MusicPlayer::currentTag);
        const Track toFind = MusicPlayer::currentTrack >= 0_i32 && MusicPlayer::currentTrack < playlist.size() ? playlist[sz(MusicPlayer::currentTrack)] : Track {};

        std::error_code ec;
        MusicPlayer::playlists.clear();

        std::vector<std::error_code> ret;
        for (const auto& dir : fs::recursive_directory_iterator("music/", fs::directory_options::skip_permission_denied, ec))
        {
            if (dir.is_regular_file(ec))
            {
                const TagLib::FileRef f(native_string(dir.path().generic_u8string()).c_str());

                const auto [title, subtitle] = TrackMetadata::readTitle(f).move_or(
                    TrackMetadata::Title { .primary = sys::str(dir.path().stem().generic_u8string()), .sub = u8"(^ Title metadata couldn't be read, fallback to filename!)" });
                const auto [artists, feats] = TrackMetadata::readArtists(f).move_or(TrackMetadata::Artists { .artists { sys::str(u8"unknown") }, .feats {} });

                std::vector<sys::str> tags { u8"all" };
                auto tagsRes = TrackMetadata::readField(f, u8"TAGS");
                if (tagsRes)
                    for (sys::str& t : tagsRes.move())
                        sys::meta::append_to(tags, std::move(t));
                if (tags.size() == 1uz)
                    tags.emplace_back(u8"uncategorized");
                std::vector<sys::str> aka = TrackMetadata::readField(f, u8"AKA").move_or(std::vector<sys::str> {});

                const Track track { .file = dir.path(),
                                    .titleDisplay = sys::cstr(title),
                                    .subtitleDisplay = sys::cstr(subtitle),
                                    .artistsDisplay =
                                        [&]
                {
                    sys::str ret = sys::str::join(artists, u8"; ");
                    if (!feats.empty())
                        ret.append(u8" ft. ").append(sys::str::join(feats, u8"; "));
                    return sys::cstr(ret);
                }(),
                                    .tagsDisplay = sys::cstr(sys::str::join(tags, u8"; ")),
                                    .akaDisplay = sys::cstr(sys::str::join(aka, u8"; ")),
                                    .aka = std::move(aka) };

                for (const sys::str& tag : tags)
                    MusicPlayer::playlists[sys::str(tag)].emplace_back(track);
            }
            if (ec)
            {
                ret.emplace_back(ec);
                ec.clear();
            }
        }

        for (auto& [_, pl] : MusicPlayer::playlists)
            std::ranges::sort(pl);

        if (MusicPlayer::playlists.empty())
            ret.emplace_back(std::make_error_code(std::errc::no_such_file_or_directory));

        MusicPlayer::currentTrack = MusicPlayer::indexOf(MusicPlayer::currentPlaylist(), toFind);
        return ret;
    }

    static inline sys::str currentTag = u8"all"; ///< Currently selected tag.
    static inline i32 currentTrack = 0_i32;      ///< Currently selected track index.

    /// @brief Retrieve the playlist with `tag`.
    /// @return Playlist, or static empty vector if not found.
    [[nodiscard]] static const std::vector<Track>& playlistWithTag(const std::u8string_view tag)
    {
        static const std::vector<Track> notFound;
        _retif(notFound, tag.empty());

        if (auto it = MusicPlayer::playlists.find(sys::str(tag)); it != MusicPlayer::playlists.end())
            return it->second;

        return notFound;
    }
    /// @brief Retrieve the current playlist, based on `MusicPlayer::currentTag`.
    [[nodiscard]] static const std::vector<Track>& currentPlaylist() { return MusicPlayer::playlistWithTag(MusicPlayer::currentTag); }
    /// @brief Retrieve every parsed playlist.
    [[nodiscard]] static const std::map<sys::str, std::vector<Track>>& allPlaylists() { return MusicPlayer::playlists; }

    /// @brief Reorder current playlist with reordering function.
    /// @return Whether the playlist existed to reorder.
    [[nodiscard]] static bool reorderCurrentPlaylist(auto&& reorder, sys::str tag, const bool followPrevious)
    {
        auto it = MusicPlayer::playlists.find(tag);
        _retif(false, it == MusicPlayer::playlists.end());

        if (followPrevious)
        {
            std::vector<Track>& playlist = it->second;
            const Track toFind = MusicPlayer::currentTrack >= 0_i32 && MusicPlayer::currentTrack < playlist.size() ? playlist[sz(MusicPlayer::currentTrack)] : Track {};

            reorder(playlist, std::move(tag));
            MusicPlayer::currentTrack = MusicPlayer::indexOf(playlist, toFind);
        }
        else
            reorder(it->second, std::move(tag));

        return true;
    }
    /// @brief Shuffle the current playlist.
    /// @return Whether the playlist existed to shuffle.
    [[nodiscard]] static bool shufflePlaylist(sys::str tag, const bool followPrevious = true)
    {
        return MusicPlayer::reorderCurrentPlaylist([](std::vector<Track>& playlist, sys::str tag)
        {
            std::shuffle(playlist.begin(), playlist.end(), MusicPlayer::randEngine);
            if (MusicPlayer::playlists.contains(tag))
                MusicPlayer::lastPlaylistReorderWasReshuffle.insert(std::move(tag));
        }, std::move(tag), followPrevious);
    }
    /// @brief Sort the current playlist lexicographically.
    /// @return Whether the playlist existed to sort.
    [[nodiscard]] static bool sortPlaylistLexicographically(sys::str tag, const bool followPrevious = true)
    {
        return MusicPlayer::reorderCurrentPlaylist([](std::vector<Track>& playlist, sys::str tag)
        {
            std::ranges::sort(playlist);
            MusicPlayer::lastPlaylistReorderWasReshuffle.erase(tag);
        }, std::move(tag), followPrevious);
    }
    /// @brief Sort the current playlist reverse lexicographically.
    /// @return Whether the playlist existed to sort.
    [[nodiscard]] static bool sortPlaylistReverseLexicographically(sys::str tag, const bool followPrevious = true)
    {
        return MusicPlayer::reorderCurrentPlaylist([](std::vector<Track>& playlist, sys::str tag)
        {
            std::ranges::sort(playlist, std::greater<>());
            MusicPlayer::lastPlaylistReorderWasReshuffle.erase(tag);
        }, std::move(tag), followPrevious);
    }

    /// @brief Check if there is music loaded.
    /// @note Thread-safe.
    [[nodiscard]] static bool loaded() { return MusicPlayer::hasAudio.load(); }
    /// @brief Check if music is currently playing.
    /// @note Thread-safe.
    [[nodiscard]] static bool playing() { return MusicPlayer::isPlaying.load(); }
    /// @brief Check if music should autoplay.
    /// @note Thread-safe.
    [[nodiscard]] static bool autoplay() { return MusicPlayer::shouldAutoplay.load(); }
    /// @brief Set whether music should autoplay.
    /// @note Thread-safe.
    static void autoplay(const bool value) { MusicPlayer::shouldAutoplay.store(value); }

    /// @brief Retrieve the current time in seconds of the currently playing track.
    static sys::result<float, Error> currentTime()
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);
        float ret = 0.0f;
        if (const ma_result res = ma_sound_get_cursor_in_seconds(&MusicPlayer::audio->sound, &ret); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);
        return ret;
    }
    /// @brief Retrieve the total time in seconds of the currently playing track.
    static sys::result<float, Error> totalTime()
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);
        return MusicPlayer::audio->audioLen;
    }
    /// @brief Format a duration in `seconds` for display.
    _pure_const static sys::cstr formatTime(float seconds)
    {
        return std::format("{}:{:02}", *i32(seconds / 60.0f), *i32(std::fmod(seconds, 60.0f))); // NOLINT(readability-magic-numbers)
    }

    /// @brief Resume the currently playing track.
    [[nodiscard]] static sys::result<void, Error> resume()
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);

        Audio& aud = *MusicPlayer::audio;
        if (const ma_result res = ma_sound_start(&aud.sound); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);

        MusicPlayer::isPlaying = true;
        return {};
    }
    /// @brief Pause the currently playing track.
    [[nodiscard]] static sys::result<void, Error> pause()
    {
        _retif(Error::TrackNotLoaded, !MusicPlayer::audio);

        Audio& aud = *MusicPlayer::audio;
        if (const ma_result res = ma_sound_stop(&aud.sound); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);

        MusicPlayer::isPlaying = false;
        return {};
    }
    /// @brief Seek to a specific time in the currently playing track.
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

    /// @brief Retrieve the volume of the audio engine.
    [[nodiscard]] static float volume() { return ma_engine_get_volume(&MusicPlayer::audioEngine()); }
    /// @brief Set the volume of the audio engine.
    [[nodiscard]] static sys::result<void, Error> volume(float linear)
    {
        if (const ma_result res = ma_engine_set_volume(&MusicPlayer::audioEngine(), linear); res != MA_SUCCESS)
            return Error::fromAudioEngineResult(res);
        return {};
    }

    /// @brief Lookup a track by name in the current playlist.
    [[nodiscard]] static sys::result<std::filesystem::path, Error> musicLookup(const std::u8string_view title) // NOLINT(readability-function-cognitive-complexity)
    {
        namespace fs = std::filesystem;
        std::error_code ec;

        if (!fs::exists("music/", ec))
            return Error::DirectoryNotFound;
        if (ec)
            return Error::fromCategory(ec.category());

        const auto tryFindWithCompare = [&ec](auto&& pred) -> sys::result<fs::path, Error>
        {
            for (const Track& track : MusicPlayer::playlistWithTag(u8"all"))
            {
                if (pred(track.fullDisplayTitle()))
                    return track.file;
                if (pred(track.titleDisplay))
                    return track.file;
                for (const auto& alt : track.aka)
                    if (pred(sys::cstr(alt)))
                        return track.file;

                if (ec)
                    return Error::fromCategory(ec.category());
            }

            return Error::TrackNotFound;
        };

        sys::cstr compare(title);
        sys::result<fs::path, Error> res = tryFindWithCompare([&compare](const std::string_view trackName) { return trackName == compare; });
        _retif(res.move(), res);

        compare.fold();
        res = tryFindWithCompare([&compare](sys::cstr trackName) { return trackName.fold() == compare; });
        _retif(res.move(), res);
        res = tryFindWithCompare([&compare](sys::cstr trackName) { return trackName.fold().starts_with(compare); });
        _retif(res.move(), res);
        res = tryFindWithCompare([&compare](sys::cstr trackName) { return trackName.fold().contains(compare); });
        _retif(res.move(), res);

        return Error::TrackNotFound;
    }
    /// @brief Start playing a music file.
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

    /// @brief Lookup a track by name in the current playlist and play it.
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
    /// @brief Stop the current music.
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

    /// @brief Start playing the current track at `MusicPlayer::currentTrack`.
    [[nodiscard]] static sys::result<void, Error> play()
    {
        _retif(stopRes, auto stopRes = MusicPlayer::stopMusic(); !stopRes);

        const std::vector<Track>& playlist = MusicPlayer::currentPlaylist();
        if (MusicPlayer::currentTrack < 0_i32 || MusicPlayer::currentTrack >= playlist.size())
        {
            MusicPlayer::currentTrack = 0_i32;
            _retif(Error::PlaylistEmpty, playlist.empty());

            const sys::str tag(MusicPlayer::currentTag);
            if (MusicPlayer::lastPlaylistReorderWasReshuffle.contains(tag) && !MusicPlayer::shufflePlaylist(tag, false))
            {
                MusicPlayer::lastPlaylistReorderWasReshuffle.erase(tag);
                return Error::PlaylistEmpty;
            }
        }

        Screen().PostEvent(ui::Event::Custom);

        return MusicPlayer::startMusic(playlist[sz(MusicPlayer::currentTrack)].file);
    }
    /// @brief Start playing the next track in the current playlist.
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
