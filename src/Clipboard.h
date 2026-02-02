#pragma once

#include <Preamble.h>

#include <string>

#include <module/sys>

#include <Screen.h>

#if !_libcxxext_os_windows

#include <iostream>

#include <Utility.h>

#else

#include <cstring>
#include <string_view>

#undef NOMINMAX
#define NOMINMAX 1 // NOLINT(readability-identifier-naming)
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1 // NOLINT(readability-identifier-naming)
#include <Windows.h>

#include <Errhandlingapi.h>
#include <Minwindef.h>
#include <Winerror.h>

#endif

#if _libcxxext_os_windows
/// @brief Set clipboard text on Windows.
inline void setClipboardWin32(std::string_view str)
{
    if (!OpenClipboard(nullptr))
        return;
    const sys::destructor _ = [&] noexcept { CloseClipboard(); };

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

    std::memcpy(pData, str.data(), str.size()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    *(_as(char*, pData) + str.size()) = '\0';   // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    while (GlobalUnlock(hGlob) != 0)
        _retif(, GetLastError() != NO_ERROR);

    (void)SetClipboardData(CF_TEXT, hGlob); // If this fails there's nothing we can do anyways.
}
#endif

/// @brief Create an event catcher that for handling Ctrl + C like clipboard copy.
inline ui::ComponentDecorator /* NOLINT(readability-identifier-naming) */ ClipboardHandler()
{
    return ui::CatchEvent([](ui::Event event)
    {
        if (event == ui::Event::CtrlC)
        {
            if (const std::string selection = Screen().GetSelection(); !selection.empty())
            {
#if !_libcxxext_os_windows
                std::cout << "\033]52;c;" << base64Encode(selection) << "\a" << std::flush;
#else
                setClipboardWin32(selection);
#endif
            }
            return true;
        }

        return false;
    });
}
