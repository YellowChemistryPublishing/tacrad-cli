#pragma once

#include <format>
#include <fstream>
#include <iosfwd>
#include <new>
#include <ostream>
#include <print>
#include <stdexcept>
#include <thread>

class Debug
{
public:
    Debug() = delete;

    template <typename... Args>
    static void log(std::format_string<Args...> fmt, Args&&... args /* NOLINT(readability-identifier-naming) */)
    {
    PrintAgain:
        try
        {
            static std::ofstream out("out.log", std::ofstream::out | std::ofstream::trunc);
            std::println(out, fmt, std::forward<Args>(args)...);
            std::flush(out);
        }
        catch (const std::format_error&)
        {
            std::this_thread::yield();
            goto PrintAgain;
        }
        catch (const std::length_error&)
        {
            std::this_thread::yield();
            goto PrintAgain;
        }
        catch (const std::bad_alloc&)
        {
            std::this_thread::yield();
            goto PrintAgain;
        }
    }
};
