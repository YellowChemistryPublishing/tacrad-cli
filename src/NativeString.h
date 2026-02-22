#pragma once

#include <module/sys>
#include <module/sys.Text>

#if _libcxxext_os_windows
using native_string = sys::wstr; // NOLINT(readability-identifier-naming)
#else
using native_string = sys::cstr; // NOLINT(readability-identifier-naming)
#endif
