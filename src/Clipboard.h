#pragma once

#include <Preamble.h>

#include <string>
#include <string_view>

#include <module/sys>

#include <Screen.h>

#if _libcxxext_os_windows

#undef NOMINMAX
#define NOMINMAX 1
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

inline void setClipboard(std::string_view str)
{
    if (!OpenClipboard(nullptr))
        return;
    sys::destructor _ = [&] noexcept { CloseClipboard(); };

    if (!EmptyClipboard())
        return;

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, str.size() + 1);
    if (!hGlob)
        return;

    void* pData = GlobalLock(hGlob);
    if (!pData)
    {
        GlobalFree(hGlob);
        return;
    }

    std::memcpy(pData, str.data(), str.size());
    *(_as(char*, pData) + str.size()) = '\0';

    if (GlobalUnlock(hGlob) != 0)
        return;

    SetClipboardData(CF_TEXT, hGlob);
}

#endif

/// @brief Clipboard utility functions.
/// @note Static class.
struct Clipboard
{
private:
    /// @brief Base64 encode a string for OSC 52 clipboard.
    static std::string base64Encode(std::string_view input)
    {
        static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string output;
        output.reserve(((input.size() + 2) / 3) * 4); // NOLINT(readability-magic-numbers)

        for (sz i = 0_uz; i < input.size(); i += 3_uz) // NOLINT(readability-magic-numbers)
        {
            u32 n = u32(_as(byte, input[i])) << 16_u32; // NOLINT(readability-magic-numbers)
            if (i + 1 < input.size())
                n |= u32(_as(byte, input[i + 1])) << 8_u32; // NOLINT(readability-magic-numbers)
            if (i + 2 < input.size())
                n |= u32(_as(byte, input[i + 2]));

            // NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)
            output.append(1, table[(n >> 18_u32) & 0x3F_u32])
                .append(1, table[(n >> 12_u32) & 0x3F_u32])
                .append(1, (i + 1 < input.size()) ? table[(n >> 6_u32) & 0x3F_u32] : '=')
                .append(1, (i + 2 < input.size()) ? table[n & 0x3F_u32] : '=');
            // NOLINTEND(readability-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)
        }

        return output;
    }
public:
    Clipboard() = delete;

    /// @brief Create an event catcher that for handling Ctrl + C like clipboard copy.
    static ui::ComponentDecorator clipboardHandler()
    {
        return ui::CatchEvent([](ui::Event event)
        {
            if (event == ui::Event::CtrlC)
            {
                if (const std::string selection = Screen().GetSelection(); !selection.empty())
                {
#if !_libcxxext_os_windows
                    std::cout << "\033]52;c;" << Clipboard::base64Encode(selection) << "\a" << std::flush;
#else
                    setClipboard(selection);
#endif
                }
                return true;
            }

            return false;
        });
    }
};
