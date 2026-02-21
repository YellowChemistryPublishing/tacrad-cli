#pragma once

#include <cstddef>
#include <ios>
#include <miniaudio.h>
#include <string_view>
#include <system_error>

#include <module/sys>

/// @brief Common error type for `tacrad-cli`.
/// @note Holds a null-terminated `char` buffer of static storage duration.
struct Error final
{
private:
    std::string_view _str;

    template <size_t N>
    constexpr explicit Error(const char (&str)[N]) : _str(_as(const char*, str))
    { }
public:
    // NOLINTNEXTLINE(hicpp-explicit-conversions)
    constexpr operator std::string_view() const { return this->_str; }

    friend bool operator==(const Error& a, const Error& b) = default;

    static const Error DirectoryNotFound;

    static const Error PlaylistEmpty;
    static const Error TrackNotLoaded;
    static const Error TrackNotFound;
    static const Error TrackSeekOutOfRange;

    /// @brief Create an `Error` from a C++ error category.
    static Error fromCategory(const std::error_category& cat)
    {
        if (cat == std::generic_category())
            return Error("C++ generic error.");
        if (cat == std::system_category())
            return Error("C++ system error.");
        if (cat == std::iostream_category())
            return Error("C++ iostream error.");
        return Error("Unknown C++ error.");
    }
    /// @brief Create an `Error` from an `ma_result`.
    static Error fromAudioEngineResult(const ma_result res)
    {
        switch (res)
        {
        case MA_SUCCESS: return Error("[Audio Engine] Success; not (shouldn't be) an error.");
        case MA_ERROR: return Error("[Audio Engine] An generic error occurred.");
        case MA_INVALID_ARGS: return Error("[Audio Engine] Invalid argument(s).");
        case MA_INVALID_OPERATION: return Error("[Audio Engine] Invalid operation.");
        case MA_OUT_OF_MEMORY: return Error("[Audio Engine] Out of memory.");
        case MA_OUT_OF_RANGE: return Error("[Audio Engine] Out of range.");
        case MA_ACCESS_DENIED: return Error("[Audio Engine] Access denied.");
        case MA_DOES_NOT_EXIST: return Error("[Audio Engine] Resource does not exist.");
        case MA_ALREADY_EXISTS: return Error("[Audio Engine] Resource already exists.");
        case MA_TOO_MANY_OPEN_FILES: return Error("[Audio Engine] Too many open files.");
        case MA_INVALID_FILE: return Error("[Audio Engine] Invalid file.");
        case MA_TOO_BIG: return Error("[Audio Engine] Resource too big.");
        case MA_PATH_TOO_LONG: return Error("[Audio Engine] Path too long.");
        case MA_NAME_TOO_LONG: return Error("[Audio Engine] Name too long.");
        case MA_NOT_DIRECTORY: return Error("[Audio Engine] Not a directory.");
        case MA_IS_DIRECTORY: return Error("[Audio Engine] Is a directory.");
        case MA_DIRECTORY_NOT_EMPTY: return Error("[Audio Engine] Directory not empty.");
        case MA_AT_END: return Error("[Audio Engine] At end.");
        case MA_NO_SPACE: return Error("[Audio Engine] No space.");
        case MA_BUSY: return Error("[Audio Engine] Resource busy.");
        case MA_IO_ERROR: return Error("[Audio Engine] IO error.");
        case MA_INTERRUPT: return Error("[Audio Engine] Interrupt.");
        case MA_UNAVAILABLE: return Error("[Audio Engine] Resource unavailable.");
        case MA_ALREADY_IN_USE: return Error("[Audio Engine] Resource already in use.");
        case MA_BAD_ADDRESS: return Error("[Audio Engine] Bad address.");
        case MA_BAD_SEEK: return Error("[Audio Engine] Bad seek.");
        case MA_BAD_PIPE: return Error("[Audio Engine] Bad pipe.");
        case MA_DEADLOCK: return Error("[Audio Engine] Deadlock.");
        case MA_TOO_MANY_LINKS: return Error("[Audio Engine] Too many links.");
        case MA_NOT_IMPLEMENTED: return Error("[Audio Engine] Not implemented.");
        case MA_NO_MESSAGE: return Error("[Audio Engine] No message.");
        case MA_BAD_MESSAGE: return Error("[Audio Engine] Bad message.");
        case MA_NO_DATA_AVAILABLE: return Error("[Audio Engine] No data available.");
        case MA_INVALID_DATA: return Error("[Audio Engine] Invalid data.");
        case MA_TIMEOUT: return Error("[Audio Engine] Timeout.");
        case MA_NO_NETWORK: return Error("[Audio Engine] No network.");
        case MA_NOT_UNIQUE: return Error("[Audio Engine] Not unique.");
        case MA_NOT_SOCKET: return Error("[Audio Engine] Not a socket.");
        case MA_NO_ADDRESS: return Error("[Audio Engine] No address.");
        case MA_BAD_PROTOCOL: return Error("[Audio Engine] Bad protocol.");
        case MA_PROTOCOL_UNAVAILABLE: return Error("[Audio Engine] Protocol unavailable.");
        case MA_PROTOCOL_NOT_SUPPORTED: return Error("[Audio Engine] Protocol not supported.");
        case MA_PROTOCOL_FAMILY_NOT_SUPPORTED: return Error("[Audio Engine] Protocol family not supported.");
        case MA_ADDRESS_FAMILY_NOT_SUPPORTED: return Error("[Audio Engine] Address family not supported.");
        case MA_SOCKET_NOT_SUPPORTED: return Error("[Audio Engine] Socket not supported.");
        case MA_CONNECTION_RESET: return Error("[Audio Engine] Connection reset.");
        case MA_ALREADY_CONNECTED: return Error("[Audio Engine] Already connected.");
        case MA_NOT_CONNECTED: return Error("[Audio Engine] Not connected.");
        case MA_CONNECTION_REFUSED: return Error("[Audio Engine] Connection refused.");
        case MA_NO_HOST: return Error("[Audio Engine] No host.");
        case MA_IN_PROGRESS: return Error("[Audio Engine] In progress.");
        case MA_CANCELLED: return Error("[Audio Engine] Cancelled.");
        case MA_MEMORY_ALREADY_MAPPED: return Error("[Audio Engine] Memory already mapped.");

        case MA_CRC_MISMATCH: return Error("[Audio Engine] CRC mismatch.");

        case MA_FORMAT_NOT_SUPPORTED: return Error("[Audio Engine] Format not supported.");
        case MA_DEVICE_TYPE_NOT_SUPPORTED: return Error("[Audio Engine] Device type not supported.");
        case MA_SHARE_MODE_NOT_SUPPORTED: return Error("[Audio Engine] Share mode not supported.");
        case MA_NO_BACKEND: return Error("[Audio Engine] No backend.");
        case MA_NO_DEVICE: return Error("[Audio Engine] No device.");
        case MA_API_NOT_FOUND: return Error("[Audio Engine] API not found.");
        case MA_INVALID_DEVICE_CONFIG: return Error("[Audio Engine] Invalid device config.");
        case MA_LOOP: return Error("[Audio Engine] Loop.");
        case MA_BACKEND_NOT_ENABLED: return Error("[Audio Engine] Backend not enabled.");

        case MA_DEVICE_NOT_INITIALIZED: return Error("[Audio Engine] Device not initialized.");
        case MA_DEVICE_ALREADY_INITIALIZED: return Error("[Audio Engine] Device already initialized.");
        case MA_DEVICE_NOT_STARTED: return Error("[Audio Engine] Device not started.");
        case MA_DEVICE_NOT_STOPPED: return Error("[Audio Engine] Device not stopped.");

        case MA_FAILED_TO_INIT_BACKEND: return Error("[Audio Engine] Failed to initialize backend.");
        case MA_FAILED_TO_OPEN_BACKEND_DEVICE: return Error("[Audio Engine] Failed to open backend device.");
        case MA_FAILED_TO_START_BACKEND_DEVICE: return Error("[Audio Engine] Failed to start backend device.");
        case MA_FAILED_TO_STOP_BACKEND_DEVICE: return Error("[Audio Engine] Failed to stop backend device.");

        default: return Error("[Audio Engine] Unknown error.");
        }
    }
};

constexpr Error Error::DirectoryNotFound("Directory not found.");

constexpr Error Error::PlaylistEmpty("Playlist has no tracks. Add some!");
constexpr Error Error::TrackNotLoaded("Track not loaded.");
constexpr Error Error::TrackNotFound("Track not found.");
constexpr Error Error::TrackSeekOutOfRange("Seek query out of duration of media.");
