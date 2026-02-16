#pragma once

#include <cctype>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

inline void stringSplitLengthConstrained(std::string_view str, sz len, std::vector<std::string>& out)
{
    const sys::str32_view v = std::span(str);
    for (auto it = v.begin(); it < v.end();)
    {
        auto itEnd = it;
        sz counter = 0_uz;
        while (itEnd < v.end() && *itEnd != '\n' && counter < len)
        {
            ++itEnd;
            ++counter;
        }

        out.emplace_back(std::to_address(it), std::to_address(itEnd));
        it = (itEnd == it || (itEnd < v.end() && *itEnd == '\n')) ? ++itEnd : itEnd;
    }
}
[[nodiscard]] inline std::string stringLastLineTrimmed(std::string_view str)
{
    const sz lastNonNewline = str.find_last_not_of('\n');
    if (lastNonNewline == std::string::npos)
        return "";

    sz end = lastNonNewline + 1_uz;
    sz beg = str.rfind('\n', lastNonNewline);
    beg = (beg == std::string::npos) ? 0_uz : beg + 1_uz;

    while (beg < end && std::isspace(str[beg]))
        ++beg;
    while (end > beg && std::isspace(str[end - 1_uz]))
        --end;

    return std::string(str.substr(beg, end - beg));
}

[[nodiscard]] inline bool setVecStartsWith(const std::vector<std::set<std::string_view>>& setVec, const std::vector<std::string>& with)
{
    sz i = 0_uz;
    for (const auto& tokenSet : setVec)
    {
        if (i >= with.size() || !tokenSet.contains(with[i]))
            return false;
        ++i;
    }
    return true;
}

/// @brief Base64 encode a string for OSC 52 clipboard.
[[nodiscard]] inline std::string base64Encode(std::u8string_view input)
{
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    ret.reserve(((input.size() + 2) / 3) * 4); // NOLINT(readability-magic-numbers)

    for (sz i = 0_uz; i < input.size(); i += 3_uz) // NOLINT(readability-magic-numbers)
    {
        u32 n = u32(_as(byte, input[i])) << 16_u32; // NOLINT(readability-magic-numbers)
        if (i + 1 < input.size())
            n |= u32(_as(byte, input[i + 1])) << 8_u32; // NOLINT(readability-magic-numbers)
        if (i + 2 < input.size())
            n |= u32(_as(byte, input[i + 2]));

        // NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)
        ret.append(1, table[(n >> 18_u32) & 0x3F_u32])
            .append(1, table[(n >> 12_u32) & 0x3F_u32])
            .append(1, (i + 1 < input.size()) ? table[(n >> 6_u32) & 0x3F_u32] : '=')
            .append(1, (i + 2 < input.size()) ? table[n & 0x3F_u32] : '=');
        // NOLINTEND(readability-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)
    }

    return ret;
}
