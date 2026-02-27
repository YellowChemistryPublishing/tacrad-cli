#pragma once

/// @file Metadata.h

#include <StringEx.h>
#include <string_view>
#include <taglib/fileref.h>
#include <taglib/flac/flacfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tpropertymap.h>
#include <utility>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

/// @brief Track metadata functions.
/// @note Static class.
class TrackMetadata final
{
public:
    TrackMetadata() = delete;

    /// @brief Title info for a track.
    struct Title
    {
        sys::str primary;
        sys::str sub;
    };
    /// @brief Read the title and subtitle of a track.
    static sys::result<Title> readTitle(const TagLib::FileRef& f)
    {
        _retif(nullptr, f.isNull() || !f.tag() || !f.file());

        std::vector<sys::cstr> subtitles;
        for (const auto& s : f.file()->properties()["SUBTITLE"])
            subtitles.emplace_back(s.to8Bit(true));
        return Title { .primary = sys::str(f.tag()->title().to8Bit(true)), .sub = sys::str::join(subtitles, u8" | ") };
    }

    /// @brief Artist info for a track.
    struct Artists
    {
        std::vector<sys::str> artists;
        std::vector<sys::str> feats;
    };
    /// @brief Read all (if listed) artists, and parse any featured artists.
    static sys::result<Artists> readArtists(const TagLib::FileRef& f)
    {
        _retif(nullptr, f.isNull() || !f.file());

        std::vector<sys::str> artists;
        for (const auto& s : f.file()->properties()["ARTIST"])
            for (sys::str& a : sys::str(s.to8Bit(true)).split(u8'/'))
                sys::meta::append_to(artists, std::move(a));

        Artists ret;
        for (sys::str& s : artists)
        {
            const auto prefixLen = [](const std::u8string_view s) -> sz
            {
                using namespace std::string_view_literals;

                // NOLINTBEGIN(misc-include-cleaner)
                if (s.starts_with(u8"ft."))
                    return u8"ft."sv.size();
                if (s.starts_with(u8"feat."))
                    return u8"feat."sv.size();
                if (s.starts_with(u8"featuring "))
                    return u8"featuring "sv.size();
                // NOLINTEND(misc-include-cleaner)
                return 0_uz;
            };

            const sz l = prefixLen(s.trim());
            if (l == 0_uz)
                ret.artists.emplace_back(std::move(s));
            else
                ret.feats.emplace_back(s.substr(l, s.size() - l).trim());
        }

        return ret;
    }

    /// @brief Read a track's field as a single long string.
    /// @note Normally, fields are lists of strings, but you may want to read them as a single long string too!
    static sys::result<sys::str> readFieldRaw(const TagLib::FileRef& f, const std::u8string_view fieldName)
    {
        _retif(nullptr, f.isNull() || !f.tag());
        sys::str comment(f.tag()->comment().to8Bit(true));

        const sys::str fieldPrefix = sys::str(fieldName).append(u8'=');

        sz tagInfoBegin = comment.find_index(fieldPrefix);
        _push_nowarn_msvc(_clwarn_msvc_discard_nodiscard); // Spurious.
        _retif(nullptr, tagInfoBegin == comment.size() || (tagInfoBegin != 0_uz && comment[(tagInfoBegin - 1_uz), unsafe()] != u8'\n'));
        _pop_nowarn_msvc();
        tagInfoBegin += fieldPrefix.size();
        const sz tagInfoEnd = comment.find_index(u8'\n', tagInfoBegin);

        return comment.substr(tagInfoBegin, tagInfoEnd - tagInfoBegin).trim();
    }
    /// @brief Read all (if any) values of a track's field.
    static sys::result<std::vector<sys::str>> readField(const TagLib::FileRef& f, const std::u8string_view fieldName)
    {
        _res_movret(const sys::str fieldRaw, TrackMetadata::readFieldRaw(f, fieldName));

        std::vector<sys::str> ret = fieldRaw.split(u8';');
        _retif(nullptr, ret.empty());

        for (sys::str& s : ret)
            s.trim();
        for (auto it = ret.begin(); it != ret.end();)
        {
            if (it->empty())
            {
                if (it != ret.end() - 1_z)
                    std::swap(*it, ret.back());
                ret.pop_back();
            }
            else
                ++it;
        }

        return ret;
    }
};
