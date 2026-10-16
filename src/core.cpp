// Copyright (c) 2026 DialogueOutline contributors
// SPDX-License-Identifier: MIT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "DialogueOutline.h"
#include <bcrypt.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <limits>

namespace {
constexpr std::uintptr_t kHookRva = 0x46BDF9;
constexpr std::uint32_t kMarker = 0x3E900000; // Explicit resource marker: 0.28125f.
constexpr unsigned char kOriginal8[] = {0x00, 0xFF, 0x86, 0x88, 0x03, 0x00, 0x00, 0x48};
constexpr unsigned char kExeHash[] = {
    0x95,0x85,0xd3,0xd9,0x93,0x87,0x4d,0x65,0x15,0x6a,0xba,0x63,0x3c,0x00,0xb8,0xa2,
    0x5f,0xaa,0xb3,0xaf,0xde,0x89,0x53,0xa8,0xa1,0x83,0xc8,0x0f,0xb8,0xf6,0x57,0xac
};
volatile LONG gStatus = 0;
INIT_ONCE gInitializeOnce = INIT_ONCE_STATIC_INIT;

void status(DWORD value) noexcept { InterlockedOr(&gStatus, static_cast<LONG>(value)); }

bool host_path(wchar_t (&path)[32768]) noexcept {
    const DWORD count = GetModuleFileNameW(nullptr, path, 32768);
    if (!count || count >= 32768) return false;
    const wchar_t* name = std::wcsrchr(path, L'\\');
    return _wcsicmp(name ? name + 1 : path, L"sora_1st.exe") == 0;
}

bool verified_file_hash(const wchar_t* path) noexcept {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    unsigned char digest[32] = {};
    unsigned char buffer[65536];
    bool okay = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
    if (okay) okay = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0;
    while (okay) {
        DWORD bytes = 0;
        if (!ReadFile(file, buffer, sizeof(buffer), &bytes, nullptr)) { okay = false; break; }
        if (!bytes) break;
        okay = BCryptHashData(hash, buffer, bytes, 0) >= 0;
    }
    if (okay) okay = BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return okay && std::memcmp(digest, kExeHash, sizeof(digest)) == 0;
}

bool relative_jump(unsigned char* instruction, const void* destination) noexcept {
    const auto delta = reinterpret_cast<std::intptr_t>(destination) -
                       reinterpret_cast<std::intptr_t>(instruction + 5);
    if (delta < (std::numeric_limits<std::int32_t>::min)() ||
        delta > (std::numeric_limits<std::int32_t>::max)()) return false;
    instruction[0] = 0xE9;
    const auto displacement = static_cast<std::int32_t>(delta);
    std::memcpy(instruction + 1, &displacement, sizeof(displacement));
    return true;
}

unsigned char* allocate_near(std::uintptr_t target) noexcept {
    SYSTEM_INFO system = {};
    GetSystemInfo(&system);
    const auto granularity = static_cast<std::uintptr_t>(system.dwAllocationGranularity);
    const auto origin = target & ~(granularity - 1);
    const auto minimum = reinterpret_cast<std::uintptr_t>(system.lpMinimumApplicationAddress);
    const auto maximum = reinterpret_cast<std::uintptr_t>(system.lpMaximumApplicationAddress);
    for (std::uintptr_t distance = granularity; distance <= 0x7FFF0000; distance += granularity) {
        for (int direction = -1; direction <= 1; direction += 2) {
            const auto address = direction < 0 ? origin - distance : origin + distance;
            if ((direction < 0 && origin < distance) || address < minimum || address > maximum) continue;
            void* page = VirtualAlloc(reinterpret_cast<void*>(address), system.dwPageSize,
                                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (page) return static_cast<unsigned char*>(page);
        }
    }
    return nullptr;
}

double linear_channel(float value) noexcept {
    const double v = std::fmin(1.0, std::fmax(0.0, static_cast<double>(value)));
    return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
}
} // namespace

extern "C" {
PVOID volatile gSoraTrampoline = nullptr;
void SoraHookBridge();

DWORD WINAPI SoraChooseOutline(float r, float g, float b) noexcept {
    const double luminance = .2126 * linear_channel(r) + .7152 * linear_channel(g) + .0722 * linear_channel(b);
    const double blackContrast = (luminance + .05) / .05;
    const double whiteContrast = 1.05 / (luminance + .05);
    return blackContrast >= whiteContrast ? 0 : 1;
}

DWORD WINAPI SoraGetStatus() noexcept { return static_cast<DWORD>(InterlockedCompareExchange(&gStatus, 0, 0)); }

void SoraOutlineCallback(const unsigned char* node, const unsigned char* fill,
                         unsigned char* outline) noexcept {
    std::uint32_t marker = 0;
    std::uint16_t mode = 0;
    std::memcpy(&marker, node + 0x368, sizeof(marker));
    std::memcpy(&mode, outline + 0xAC, sizeof(mode));
    if (marker != kMarker || mode != 0x0100) return;
    float item[3], modulation[3];
    std::memcpy(item, fill + 0x98, sizeof(item));
    std::memcpy(modulation, node + 0x48, sizeof(modulation));
    const float selected = static_cast<float>(SoraChooseOutline(item[0] * modulation[0],
        item[1] * modulation[1], item[2] * modulation[2]));
    const float rgb[] = {selected, selected, selected};
    std::memcpy(outline + 0x98, rgb, sizeof(rgb));
}
}

namespace {
bool install_hook() noexcept {
    auto* target = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr)) + kHookRva;
    auto* aligned = target - 1;
    if (reinterpret_cast<std::uintptr_t>(aligned) % 8 != 0 ||
        std::memcmp(aligned, kOriginal8, sizeof(kOriginal8)) != 0) {
        // A second core/ASI or another patch owns this site. Do not claim its hook.
        status(HookConflict);
        return false;
    }
    status(OriginalBytesMatch);
    auto* page = allocate_near(reinterpret_cast<std::uintptr_t>(target));
    if (!page) return false;
    // Relay is an absolute indirect JMP; its pointer is inline, so it clobbers no registers.
    const unsigned char absoluteJump[] = {0xFF, 0x25, 0, 0, 0, 0};
    std::memcpy(page, absoluteJump, sizeof(absoluteJump));
    const auto bridge = reinterpret_cast<std::uintptr_t>(&SoraHookBridge);
    std::memcpy(page + 6, &bridge, sizeof(bridge));
    auto* trampoline = page + 0x20;
    std::memcpy(trampoline, target, 6);
    unsigned char replacement[8];
    std::memcpy(replacement, kOriginal8, sizeof(replacement));
    // Encode the target jump using the actual target address, not this stack buffer.
    const auto delta = reinterpret_cast<std::intptr_t>(page) - reinterpret_cast<std::intptr_t>(target + 5);
    if (delta < (std::numeric_limits<std::int32_t>::min)() ||
        delta > (std::numeric_limits<std::int32_t>::max)() ||
        !relative_jump(trampoline + 6, target + 6)) {
        VirtualFree(page, 0, MEM_RELEASE); return false;
    }
    replacement[1] = 0xE9;
    const auto displacement = static_cast<std::int32_t>(delta);
    std::memcpy(replacement + 2, &displacement, sizeof(displacement));
    replacement[6] = 0x90;
    DWORD oldPageProtection = 0;
    if (!VirtualProtect(page, 4096, PAGE_EXECUTE_READ, &oldPageProtection) ||
        !FlushInstructionCache(GetCurrentProcess(), page, 4096)) {
        VirtualFree(page, 0, MEM_RELEASE); return false;
    }
    DWORD oldProtection = 0;
    if (!VirtualProtect(aligned, 8, PAGE_EXECUTE_READWRITE, &oldProtection)) {
        VirtualFree(page, 0, MEM_RELEASE); return false;
    }
    LONG64 expected, desired;
    std::memcpy(&expected, kOriginal8, sizeof(expected));
    std::memcpy(&desired, replacement, sizeof(desired));
    InterlockedExchangePointer(&gSoraTrampoline, trampoline);
    const bool installed = InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(aligned),
                                                        desired, expected) == expected;
    DWORD ignored = 0;
    const bool restored = VirtualProtect(aligned, 8, oldProtection, &ignored) != FALSE;
    const bool flushed = FlushInstructionCache(GetCurrentProcess(), aligned, 8) != FALSE;
    if (!installed) {
        status(HookConflict);
        if (!restored || !flushed) status(MemoryFinalizeFailed);
        InterlockedExchangePointer(&gSoraTrampoline, nullptr);
        VirtualFree(page, 0, MEM_RELEASE);
        return false;
    }
    // Once published, the relay/trampoline remain valid even if a later maintenance API fails.
    if (!restored || !flushed) status(MemoryFinalizeFailed);
    status(HookActive);
    return true;
}

BOOL CALLBACK initialize(PINIT_ONCE, PVOID, PVOID*) noexcept {
    wchar_t path[32768] = {};
    if (!host_path(path)) {
        status(ForeignHost | InitializationFinished);
        return TRUE;
    }
    status(HostNameMatch);
    if (!verified_file_hash(path)) {
        status(HostHashRejected | HookFailed | InitializationFinished);
        return TRUE;
    }
    status(HostHashMatch);
    // Pin only after host verification, before any callback destination is published.
    HMODULE pinned = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           reinterpret_cast<LPCWSTR>(&initialize), &pinned)) {
        status(CorePinFailed | HookFailed | InitializationFinished);
        return TRUE;
    }
    status(CorePinned);
    if (!install_hook()) status(HookFailed);
    status(InitializationFinished);
    return TRUE; // Cache both success and refusal; never race a retry against a hook.
}
} // namespace

extern "C" {
DWORD WINAPI SoraDialogueInitialize() noexcept {
    InitOnceExecuteOnce(&gInitializeOnce, initialize, nullptr, nullptr);
    return SoraGetStatus();
}

void WINAPI InitializeASI() noexcept { (void)SoraDialogueInitialize(); }

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
}
