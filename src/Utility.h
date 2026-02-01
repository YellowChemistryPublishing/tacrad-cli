#pragma once

#include <cctype>
#include <codecvt>
#include <filesystem>
#include <locale>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <module/sys>

#include <Debug.h>

/// @brief Convert a `std::string` to `std::u32string`.
[[nodiscard]] inline std::u32string u32stringFrom(std::string_view str)
{
    try
    {
        _push_nowarn_deprecated();
        return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>().from_bytes(std::to_address(str.begin()), std::to_address(str.end()));
        _pop_nowarn_deprecated();
    }
    catch (const std::range_error&)
    {
        debugLog("Range error: {}.", str);
        return U"";
    }
}
/// @brief Convert a `std::u8string` to `std::string`.
[[nodiscard]] inline std::string stringFrom(std::u8string_view str) { return { str.begin(), str.end() }; }

inline std::u32string u32stringToLower(std::u32string_view str)
{
    std::u32string ret;
    ret.reserve(str.size());
    for (const char32_t c : str)
        ret.push_back(_as(char32_t, std::tolower(_as(int, c))));
    return ret;
}

inline void wstringSplitLengthConstrained(std::string_view str, sz len, std::vector<std::string>& out)
{
    for (sz i = 0_uz; i < str.size();)
    {
        sz iEnd = i;
        while (iEnd < str.size() && str[iEnd] != '\n' && sz(iEnd - i) < sz(len))
            iEnd++;
        out.emplace_back(str.substr(sz(i), sz(iEnd - i)));
        i = (iEnd == i || (iEnd < str.size() && str[iEnd] == '\n')) ? iEnd + 1_uz : iEnd;
    }
}

[[nodiscard]] inline std::string wstringLastLineTrimmed(std::string_view str)
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
[[nodiscard]] inline std::string base64Encode(std::string_view input)
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

/// @brief Get a UTF-8 string from a path.
[[nodiscard]] inline std::string pathToString(const std::filesystem::path& path)
{
    const std::u8string u8 = path.u8string();
    return { u8.begin(), u8.end() };
}
