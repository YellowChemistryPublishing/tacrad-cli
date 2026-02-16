#pragma once

#include <filesystem>
#include <mp4/mp4item.h>
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

class TrackMetadata
{
public:
    TrackMetadata() = delete;

    static sys::result<std::vector<sys::str>> readTags(const std::filesystem::path& file)
    {
        TagLib::FileRef f(native_string(file.generic_u8string()).c_str());
        _retif(nullptr, f.isNull() || !f.tag());

        sys::str comment { f.tag()->comment().to8Bit(true) };

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
