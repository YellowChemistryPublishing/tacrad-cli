#pragma once

#include <StringEx.h>
#include <mp4/mp4item.h>
#include <string_view>
#include <taglib/fileref.h>
#include <taglib/flac/flacfile.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4/mp4file.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mpeg/id3v2/frames/textidentificationframe.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/mpegfile.h>
#include <taglib/mpegfile.h>
#include <taglib/ogg/xiphcomment.h>
#include <taglib/tag.h>
#include <taglib/xiphcomment.h>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

#include <NativeString.h>

/// @brief Track metadata functions.
/// @note Static class.
class TrackMetadata
{
public:
    TrackMetadata() = delete;

    struct TrackArtists
    {
        std::vector<sys::str> artists;
        std::vector<sys::str> feats;
    };
    /// @brief Read all (if listed) artists, and parse any featured artists.
    static sys::result<TrackArtists> readArtists(const TagLib::FileRef& f)
    {
        _retif(nullptr, f.isNull() || !f.tag());
        sys::str artists(f.tag()->artist().to8Bit(true));

        const auto prefixLen = [](const std::u8string_view s) -> sz
        {
            using namespace std::literals;

            if (s.starts_with(u8"ft. "))
                return u8"ft. "sv.size();
            if (s.starts_with(u8"feat. "))
                return u8"feat. "sv.size();
            if (s.starts_with(u8"featuring "))
                return u8"featuring "sv.size();
            return 0_uz;
        };

        TrackArtists ret;
        for (sys::str& s : artists.split(u8'/'))
        {
            const sz l = prefixLen(s.trim());
            if (l == 0_uz)
                ret.artists.emplace_back(std::move(s));
            else
                ret.feats.emplace_back(s.substr(l, s.size() - l));
        }

        return ret;
    }

    /// @brief Read all (if any) track tags, which is used to form playlists of the same names.
    static sys::result<std::vector<sys::str>> readTrackTags(const TagLib::FileRef& f)
    {
        _retif(nullptr, f.isNull() || !f.tag());
        sys::str comment(f.tag()->comment().to8Bit(true));

        sz tagInfoBegin = comment.find_index(u8"TAGS=");
        _retif(nullptr, tagInfoBegin == comment.size() || (tagInfoBegin != 0_uz && comment[tagInfoBegin - 1_uz] != u8'\n'));
        tagInfoBegin += std::u8string_view(u8"TAGS=").size();
        const sz tagInfoEnd = comment.find_index(u8'\n', tagInfoBegin);

        std::vector<sys::str> ret = comment.substr(tagInfoBegin, tagInfoEnd - tagInfoBegin).split(u8';');
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
