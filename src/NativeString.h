#pragma once

/// @file

#include <module/sys>
#include <module/sys.Text>

#if _libcxxext_os_windows
using native_string = sys::wstr; // NOLINT(readability-identifier-naming)
#else
// NOLINTNEXTLINE(readability-identifier-naming)
using native_string = sys::cstr; ///< Platform-specific string type for system interop.
#endif
