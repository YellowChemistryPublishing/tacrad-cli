#pragma once

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

#define _impl_debug_log(stream_type)                                                                                   \
    i32 retryCount = 0;                                                                                                \
    std::chrono::milliseconds retryDelay = std::chrono::milliseconds(32); /* NOLINT(readability-magic-numbers) */      \
    PrintAgain:                                                                                                        \
    if (retryCount > 20) /* NOLINT(readability-magic-numbers) */                                                       \
    {                                                                                                                  \
        assert(!0xBADC0DE);   /* NOLINT(readability-magic-numbers) */                                                  \
        std::exit(0xBADC0DE); /* NOLINT(concurrency-mt-unsafe, readability-magic-numbers): We're giving up anyways. */ \
    }                                                                                                                  \
                                                                                                                       \
    try                                                                                                                \
    {                                                                                                                  \
        stream_type out("out.log", stream_type::out | stream_type::app);                                               \
        out << std::format(std::move(fmt), std::forward<Args>(args)...) << std::flush;                                 \
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

template <typename... Args>
inline void debugLog(std::format_string<Args...> fmt, Args&&... args /* NOLINT(readability-identifier-naming) */) noexcept
{
    _impl_debug_log(std::ofstream);
}
template <typename... Args>
inline void wdebugLog(std::wformat_string<Args...> fmt, Args&&... args /* NOLINT(readability-identifier-naming) */) noexcept
{
    _impl_debug_log(std::wofstream);
}
