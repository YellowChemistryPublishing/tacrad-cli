#pragma once

/// @file

#include <ftxui/screen/box.hpp>
#include <memory>
#include <set>
#include <span>
#include <string_view>
#include <vector>

#include <module/sys>
#include <module/sys.Text>

namespace ui = ftxui;

/// @brief Split a string by a delimiter, or until a maximum line length is reached.
constexpr void stringSplitLengthConstrained(const std::string_view str, const sz len, std::vector<sys::cstr>& out)
{
    const sys::codepoint_view v = std::span(str);
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

/// @brief Truncate a string, adding ellipses if trimmed.
[[nodiscard]] _pure_const constexpr sys::cstr truncateStrForDisplay(const std::string_view str, const ui::Box& bounds)
{
    u64 ccount = 0_u64;
    ssz trimFrom = 0_z;
    const sys::codepoint_view v(str);
    for (auto it = v.begin(); it != v.end();)
    {
        // Six space characters: ` * [text]|  `.
        if (++ccount > i32(bounds.x_max) - i32(bounds.x_min) - 6_i32) // NOLINT(readability-magic-numbers)
            break;

        ++it;
        trimFrom = std::to_address(it) - std::to_address(v.begin());
    }

    if (trimFrom < str.size() && trimFrom > 0_z) // Trimming a tiny string to `...` is worthless.
    {
        sys::cstr ret(str.substr(0, sz(trimFrom)));
        ret.append("...");
        return ret;
    }

    return sys::cstr(str);
}

/// @brief Match a string vector against a stringset vector.
[[nodiscard]] _pure_const constexpr bool setVecStartsWith(const std::vector<std::set<std::string_view>>& setVec, const std::vector<sys::cstr>& with)
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
[[nodiscard]] _pure_const constexpr sys::cstr base64Encode(std::string_view input)
{
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    sys::cstr ret;
    ret.reserve(((input.size() + 2) / 3) * 4); // NOLINT(readability-magic-numbers)

    for (sz i = 0_uz; i < input.size(); i += 3_uz) // NOLINT(readability-magic-numbers)
    {
        u32 n = u32(_as(byte, input[i])) << 16_u32; // NOLINT(readability-magic-numbers)
        if (i + 1 < input.size())
            n |= u32(_as(byte, input[i + 1])) << 8_u32; // NOLINT(readability-magic-numbers)
        if (i + 2 < input.size())
            n |= u32(_as(byte, input[i + 2]));

        // NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)
        ret.append(table[(n >> 18_u32) & 0x3F_u32])
            .append(table[(n >> 12_u32) & 0x3F_u32])
            .append(i + 1 < input.size() ? table[(n >> 6_u32) & 0x3F_u32] : '=')
            .append(i + 2 < input.size() ? table[n & 0x3F_u32] : '=');
        // NOLINTEND(readability-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)
    }

    return ret;
}
