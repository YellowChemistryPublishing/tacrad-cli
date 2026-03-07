#pragma once

/// @file

// NOLINTBEGIN(portability-template-virtual-member-function, hicpp-signed-bitwise)

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iosfwd>
#include <new>
#include <ostream>
#include <stdexcept>
#include <thread>

#include <module/sys>

/// @private
/// @def _impl_debug_log(streamType)
/// @brief Implementation of `debugLog` and `wdebugLog`.
/// @param streamType The type of stream to log to.
#define _impl_debug_log(streamType)                                                                                    \
    i32 retryCount = 0;                                                                                                \
    std::chrono::milliseconds retryDelay = std::chrono::milliseconds(32); /* NOLINT(readability-magic-numbers) */      \
    PrintAgain:                                                                                                        \
    if (retryCount > 20) /* NOLINT(readability-magic-numbers) */                                                       \
    {                                                                                                                  \
        std::exit(0xBADC0DE); /* NOLINT(concurrency-mt-unsafe, readability-magic-numbers): We're giving up anyways. */ \
    }                                                                                                                  \
                                                                                                                       \
    try                                                                                                                \
    {                                                                                                                  \
        streamType out("out.log", streamType::out | streamType::app);                                                  \
        out << std::format(std::move(fmt), std::forward<Args>(args)...) << std::endl /* Deliberate. */;                \
        return;                                                                                                        \
    }                                                                                                                  \
    catch (const std::format_error&) /* NOLINT(bugprone-empty-catch) */                                                \
    { }                                                                                                                \
    catch (const std::length_error&) /* NOLINT(bugprone-empty-catch) */                                                \
    { }                                                                                                                \
    catch (const std::bad_alloc&) /* NOLINT(bugprone-empty-catch) */                                                   \
    { }                                                                                                                \
                                                                                                                       \
    ++retryCount;                                                                                                      \
    std::this_thread::sleep_for(retryDelay);                                                                           \
    retryDelay *= 2;                                                                                                   \
    goto PrintAgain;

/// @brief Logs a formatted message to the debug log file.
template <typename... Args>
inline void debugLog(std::format_string<Args...> fmt, Args&&... args /* NOLINT(readability-identifier-naming) */) noexcept
{
    _impl_debug_log(std::ofstream); ///< @private
}
/// @brief Logs a formatted message to the debug log file.
template <typename... Args>
inline void wdebugLog(std::wformat_string<Args...> fmt, Args&&... args /* NOLINT(readability-identifier-naming) */) noexcept
{
    _impl_debug_log(std::wofstream); ///< @private
}

// NOLINTEND(portability-template-virtual-member-function, hicpp-signed-bitwise)
