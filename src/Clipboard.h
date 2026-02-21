#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <module/sys>

#include <Screen.h>

#if !_libcxxext_os_windows

#include <iostream>

#include <Utility.h>

#else

#include <cstring>

#include <module/sys.Text>

#undef NOMINMAX
#define NOMINMAX 1 // NOLINT(readability-identifier-naming)
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1 // NOLINT(readability-identifier-naming)
#include <Windows.h>

#include <Errhandlingapi.h>
#include <Minwindef.h>
#include <Winerror.h>

#endif

namespace ui = ftxui;

#if _libcxxext_os_windows

/// @brief Set clipboard text on Windows.
inline void setClipboardWin32(sys::cstr str)
{
    if (!OpenClipboard(nullptr))
        return;
    const sys::destructor _ = [] noexcept { CloseClipboard(); };

    if (!EmptyClipboard())
        return;

    sys::wstr uniStr(str);
    const sz sizeBytes = (uniStr.size() + 1_uz) * sz(sizeof(wchar_t));
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, sizeBytes);
    if (!hGlob)
        return;

    void* pData = GlobalLock(hGlob);
    if (!pData)
    {
        GlobalFree(hGlob);
        return;
    }

    std::memcpy(pData, uniStr.data(), sizeBytes);    // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    *(_as(wchar_t*, pData) + uniStr.size()) = L'\0'; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    while (GlobalUnlock(hGlob) != 0)
        _retif(, GetLastError() != NO_ERROR);

    (void)SetClipboardData(CF_UNICODETEXT, hGlob); // If this fails there's nothing we can do anyways.
}

#endif

/// @brief Create an event catcher that for handling Ctrl + C like clipboard copy.
inline ui::ComponentDecorator /* NOLINT(readability-identifier-naming) */ ClipboardHandler()
{
    return ui::CatchEvent([](const ui::Event& event)
    {
        if (event == ui::Event::CtrlC)
        {
            if (const sys::cstr selection(Screen().GetSelection()); !selection.empty())
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
