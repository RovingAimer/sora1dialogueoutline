// Copyright (c) 2026 DialogueOutline contributors
// SPDX-License-Identifier: MIT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "runtime_config.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace dialogue {
namespace {
constexpr wchar_t kFileName[] = L"DialogueOutline.ini";
constexpr DWORD kQuietMs = 250;
constexpr DWORD kRetryMs = 100;
constexpr unsigned kMaxRetries = 3;
constexpr std::size_t kMaxIniBytes = 65536;
constexpr std::uint64_t Pack(Settings value) noexcept {
    return value.textPercent | (std::uint64_t{value.namePercent} << 16) |
        (std::uint64_t{value.heightPercent} << 32);
}
static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "The render path requires a lock-free x64 settings snapshot.");
std::atomic<std::uint64_t> gSettings{Pack(Settings{})};
std::atomic<DWORD> gReadStatus{ERROR_SUCCESS};
std::atomic<DWORD> gWatchStatus{ERROR_SUCCESS};
INIT_ONCE gStartOnce = INIT_ONCE_STATIC_INIT;

struct FileHandle {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~FileHandle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};

std::wstring_view Trim(std::wstring_view value) noexcept {
    const auto first = value.find_first_not_of(L" \t\r");
    if (first == std::wstring_view::npos) return {};
    const auto last = value.find_last_not_of(L" \t\r");
    return value.substr(first, last - first + 1);
}

bool Equal(std::wstring_view left, std::wstring_view right) noexcept {
    return left.size() == right.size() &&
        CompareStringOrdinal(left.data(), static_cast<int>(left.size()),
                             right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

bool Percent(std::wstring_view value, std::uint32_t& result) noexcept {
    value = Trim(value);
    if (!value.empty() && value.back() == L'%') {
        value.remove_suffix(1);
        value = Trim(value);
    }
    if (value.empty()) return false;
    std::uint32_t number = 0;
    for (const wchar_t digit : value) {
        if (digit < L'0' || digit > L'9') return false;
        number = number * 10 + static_cast<std::uint32_t>(digit - L'0');
        if (number > 200) return false;
    }
    if (!number) return false;
    result = number;
    return true;
}

DWORD Decode(const std::string& bytes, std::wstring& text) {
    const auto* data = reinterpret_cast<const unsigned char*>(bytes.data());
    if (bytes.size() >= 2 && data[0] == 0xff && data[1] == 0xfe) {
        if (bytes.size() % 2) return ERROR_NO_UNICODE_TRANSLATION;
        text.resize((bytes.size() - 2) / 2);
        for (std::size_t i = 0; i < text.size(); ++i) {
            text[i] = static_cast<wchar_t>(data[2 + 2 * i] | (data[3 + 2 * i] << 8));
        }
        if (!text.empty() && !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr))
            return ERROR_NO_UNICODE_TRANSLATION;
    } else {
        const std::size_t skip = bytes.size() >= 3 && data[0] == 0xef &&
            data[1] == 0xbb && data[2] == 0xbf ? 3 : 0;
        const int length = static_cast<int>(bytes.size() - skip);
        if (!length) return ERROR_INVALID_DATA;
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                            bytes.data() + skip, length, nullptr, 0);
        if (!count) return ERROR_NO_UNICODE_TRANSLATION;
        text.resize(static_cast<std::size_t>(count));
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                bytes.data() + skip, length, text.data(), count))
            return ERROR_NO_UNICODE_TRANSLATION;
    }
    return text.empty() || text.find(L'\0') != std::wstring::npos
        ? ERROR_INVALID_DATA : ERROR_SUCCESS;
}

DWORD Parse(std::wstring_view text, Settings& result) noexcept {
    bool mainSection = false;
    bool haveText = false;
    bool haveHeight = false;
    bool haveName = false;
    while (!text.empty()) {
        const auto end = text.find(L'\n');
        auto line = text.substr(0, end);
        text = end == std::wstring_view::npos ? std::wstring_view{} : text.substr(end + 1);
        line = Trim(line.substr(0, line.find_first_of(L";#")));
        if (line.empty()) continue;
        if (line.front() == L'[') {
            if (line.back() != L']') return ERROR_INVALID_DATA;
            mainSection = Equal(Trim(line.substr(1, line.size() - 2)), L"MainDialogue");
            continue;
        }
        if (!mainSection) continue;
        const auto equals = line.find(L'=');
        if (equals == std::wstring_view::npos) return ERROR_INVALID_DATA;
        const auto key = Trim(line.substr(0, equals));
        const auto value = line.substr(equals + 1);
        if (Equal(key, L"TextSize")) {
            if (haveText || !Percent(value, result.textPercent)) return ERROR_INVALID_DATA;
            haveText = true;
        } else if (Equal(key, L"BoxHeight")) {
            if (haveHeight || !Percent(value, result.heightPercent)) return ERROR_INVALID_DATA;
            haveHeight = true;
        } else if (Equal(key, L"NameSize")) {
            if (haveName || !Percent(value, result.namePercent)) return ERROR_INVALID_DATA;
            haveName = true;
        }
    }
    return haveText && haveHeight ? ERROR_SUCCESS : ERROR_INVALID_DATA;
}

DWORD ReadSettings(const wchar_t* path, Settings& result) noexcept {
    try {
        FileHandle file{CreateFileW(path, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
        if (file.value == INVALID_HANDLE_VALUE) return GetLastError();
        std::string bytes;
        char block[4096];
        for (;;) {
            DWORD count = 0;
            if (!ReadFile(file.value, block, sizeof(block), &count, nullptr)) return GetLastError();
            if (!count) break;
            if (bytes.size() + count > kMaxIniBytes) return ERROR_FILE_TOO_LARGE;
            bytes.append(block, count);
        }
        std::wstring text;
        const DWORD error = Decode(bytes, text);
        return error == ERROR_SUCCESS ? Parse(text, result) : error;
    } catch (...) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

void Publish(Settings value) noexcept {
    const auto packed = Pack(value);
    if (gSettings.load(std::memory_order_relaxed) != packed)
        gSettings.store(packed, std::memory_order_release);
}

struct WatchState {
    std::wstring path;
    HANDLE directory = INVALID_HANDLE_VALUE;
    PTP_IO io = nullptr;
    PTP_TIMER timer = nullptr;
    OVERLAPPED overlapped{};
    alignas(DWORD) std::array<unsigned char, 16384> buffer{};
    SRWLOCK watchLock = SRWLOCK_INIT;
    SRWLOCK readLock = SRWLOCK_INIT;
    SRWLOCK requestLock = SRWLOCK_INIT;
    std::uint64_t generation = 0;
    ULONGLONG due = 0;
    unsigned retries = 0;
    bool pending = false;

    // Only failed startup destroys this state, before callbacks are registered.
    // Successful startup keeps it alive with the pinned module for the process.
    ~WatchState() {
        if (timer) CloseThreadpoolTimer(timer);
        if (io) CloseThreadpoolIo(io);
        if (directory != INVALID_HANDLE_VALUE) CloseHandle(directory);
    }
};

struct ExclusiveLock {
    SRWLOCK& lock;
    explicit ExclusiveLock(SRWLOCK& value) noexcept : lock(value) { AcquireSRWLockExclusive(&lock); }
    ~ExclusiveLock() { ReleaseSRWLockExclusive(&lock); }
};

// The caller holds requestLock. This is always a one-shot timer.
void ArmTimer(WatchState& state, ULONGLONG delay) noexcept {
    LARGE_INTEGER time;
    time.QuadPart = -static_cast<LONGLONG>(delay ? delay * 10000 : 1);
    FILETIME due{time.LowPart, static_cast<DWORD>(time.HighPart)};
    SetThreadpoolTimer(state.timer, &due, 0, 0);
}

void RequestReload(WatchState& state) noexcept {
    ExclusiveLock lock(state.requestLock);
    ++state.generation;
    state.due = GetTickCount64() + kQuietMs;
    state.retries = 0;
    state.pending = true;
    ArmTimer(state, kQuietMs);
}

void CALLBACK TimerReady(PTP_CALLBACK_INSTANCE, void* context, PTP_TIMER) noexcept {
    auto& state = *static_cast<WatchState*>(context);
    ExclusiveLock reader(state.readLock);
    std::uint64_t generation;
    {
        ExclusiveLock request(state.requestLock);
        if (!state.pending) return;
        const auto now = GetTickCount64();
        if (now < state.due) {
            ArmTimer(state, state.due - now);
            return;
        }
        generation = state.generation;
    }
    Settings value;
    const DWORD error = ReadSettings(state.path.c_str(), value);
    ExclusiveLock request(state.requestLock);
    // A later save has its own timer. Never publish the superseded read.
    if (generation != state.generation) return;
    gReadStatus.store(error, std::memory_order_relaxed);
    if (error == ERROR_SUCCESS) {
        Publish(value);
        state.pending = false;
    } else if (state.retries < kMaxRetries) {
        ++state.retries;
        state.due = GetTickCount64() + kRetryMs;
        ArmTimer(state, kRetryMs);
    } else {
        state.pending = false;
    }
}

DWORD QueueRead(WatchState& state) noexcept {
    state.overlapped = {};
    // Bind requests to the completion port, so the loader's initializing
    // thread can exit without canceling the pending directory notification.
    StartThreadpoolIo(state.io);
    if (!ReadDirectoryChangesW(state.directory, state.buffer.data(),
            static_cast<DWORD>(state.buffer.size()), FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME,
            nullptr, &state.overlapped, nullptr)) {
        const DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING) {
            CancelThreadpoolIo(state.io);
            return error;
        }
    }
    return ERROR_SUCCESS;
}

bool ContainsTarget(const WatchState& state, DWORD bytes) noexcept {
    std::size_t offset = 0;
    constexpr std::size_t header = offsetof(FILE_NOTIFY_INFORMATION, FileName);
    // Only NextEntryOffset and FileNameLength delimit the variable records.
    while (offset < bytes) {
        if (bytes - offset < header) return true; // Incomplete information: reread the one INI.
        const auto* entry = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(state.buffer.data() + offset);
        if (entry->FileNameLength % sizeof(wchar_t) || entry->FileNameLength > bytes - offset - header)
            return true;
        if (Equal(std::wstring_view(entry->FileName, entry->FileNameLength / sizeof(wchar_t)), kFileName))
            return true;
        if (!entry->NextEntryOffset) return false;
        if (entry->NextEntryOffset < header + entry->FileNameLength ||
            entry->NextEntryOffset % alignof(DWORD) || entry->NextEntryOffset >= bytes - offset)
            return true;
        offset += entry->NextEntryOffset;
    }
    return false;
}

void CALLBACK DirectoryReady(PTP_CALLBACK_INSTANCE, void* context, void*,
                             ULONG error, ULONG_PTR transferred, PTP_IO) noexcept {
    auto& state = *static_cast<WatchState*>(context);
    ExclusiveLock watcher(state.watchLock);
    if (error != ERROR_SUCCESS && error != ERROR_NOTIFY_ENUM_DIR) {
        gWatchStatus.store(error, std::memory_order_relaxed);
        return;
    }
    const bool target = error == ERROR_NOTIFY_ENUM_DIR || !transferred ||
        transferred > state.buffer.size() || ContainsTarget(state, static_cast<DWORD>(transferred));
    // The old buffer has been consumed completely before it is reused.
    const DWORD queued = QueueRead(state);
    if (queued != ERROR_SUCCESS) {
        gWatchStatus.store(queued, std::memory_order_relaxed);
    }
    if (target) RequestReload(state);
}

DWORD StartWatch(WatchState& state, const std::wstring& directory) noexcept {
    state.directory = CreateFileW(directory.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
    if (state.directory == INVALID_HANDLE_VALUE) return GetLastError();
    state.timer = CreateThreadpoolTimer(TimerReady, &state, nullptr);
    if (!state.timer) return GetLastError();
    state.io = CreateThreadpoolIo(state.directory, DirectoryReady, &state, nullptr);
    if (!state.io) return GetLastError();
    return QueueRead(state);
}

BOOL CALLBACK InitializeConfig(PINIT_ONCE, void* context, void**) noexcept {
    try {
        if (!context) {
            gWatchStatus.store(ERROR_INVALID_PARAMETER, std::memory_order_relaxed);
            return TRUE;
        }
        std::wstring path(32768, L'\0');
        const DWORD length = GetModuleFileNameW(static_cast<HMODULE>(context), path.data(),
                                               static_cast<DWORD>(path.size()));
        if (!length || length >= path.size()) {
            gWatchStatus.store(length ? ERROR_INSUFFICIENT_BUFFER : GetLastError(), std::memory_order_relaxed);
            return TRUE;
        }
        path.resize(length);
        const auto slash = path.find_last_of(L"\\/");
        if (slash == std::wstring::npos) {
            gWatchStatus.store(ERROR_BAD_PATHNAME, std::memory_order_relaxed);
            return TRUE;
        }
        const auto directory = path.substr(0, slash + 1);
        auto state = std::make_unique<WatchState>();
        state->path = directory + kFileName;
        {
            // IOCP completion may run immediately after QueueRead. Hold this
            // until the initial snapshot/status and process lifetime are set.
            ExclusiveLock watcher(state->watchLock);
            const DWORD watchError = StartWatch(*state, directory);
            gWatchStatus.store(watchError, std::memory_order_relaxed);
            Settings initial;
            const DWORD readError = ReadSettings(state->path.c_str(), initial);
            gReadStatus.store(readError, std::memory_order_relaxed);
            if (readError == ERROR_SUCCESS) Publish(initial);
            if (watchError == ERROR_SUCCESS) {
                state.release(); // Process lifetime: core has pinned the module.
            }
        }
    } catch (...) {
        gWatchStatus.store(ERROR_NOT_ENOUGH_MEMORY, std::memory_order_relaxed);
    }
    return TRUE; // Initialization is one-shot, including failures.
}
} // namespace

bool StartConfig(HMODULE module) noexcept {
    if (!InitOnceExecuteOnce(&gStartOnce, InitializeConfig, module, nullptr)) {
        gWatchStatus.store(GetLastError(), std::memory_order_relaxed);
        return false;
    }
    return gWatchStatus.load(std::memory_order_relaxed) == ERROR_SUCCESS;
}

Settings CurrentSettings() noexcept {
    const auto value = gSettings.load(std::memory_order_acquire);
    return {static_cast<std::uint32_t>(value & 0xFFFF), static_cast<std::uint32_t>(value >> 32),
            static_cast<std::uint32_t>((value >> 16) & 0xFFFF)};
}

DWORD ConfigurationStatus() noexcept {
    const DWORD watchError = gWatchStatus.load(std::memory_order_relaxed);
    return watchError != ERROR_SUCCESS ? watchError : gReadStatus.load(std::memory_order_relaxed);
}
} // namespace dialogue
