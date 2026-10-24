// Copyright (c) 2026 DialogueOutline contributors
// SPDX-License-Identifier: MIT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "DialogueOutline.h"
#include "LicenseNotice.h"
#include "dialogue_layout.h"
#include "runtime_config.h"
#include "target_patterns.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
constexpr std::uint32_t kMarker = 0x3E900000; // Explicit resource marker: 0.28125f.
constexpr float kOutlineWidthMultiplier = 0.60f;
volatile LONG gStatus = 0;
INIT_ONCE gInitializeOnce = INIT_ONCE_STATIC_INIT;

void status(DWORD value) noexcept { InterlockedOr(&gStatus, static_cast<LONG>(value)); }

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
PVOID volatile gSoraPrepareTrampoline = nullptr;
PVOID volatile gSoraPositionTrampoline = nullptr;
PVOID volatile gSoraSpacingTrampoline = nullptr;
PVOID volatile gSoraDestroyTrampoline = nullptr;
void SoraHookBridge();
void SoraPrepareBridge();
void SoraPositionBridge();
void SoraSpacingBridge();
void SoraDestroyBridge();

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

void SoraOutlineCallbackWithScale(const unsigned char* node, const unsigned char* fill,
                                  unsigned char* outline, float glyphScale) noexcept {
    std::uint32_t marker = 0;
    std::uint16_t mode = 0;
    std::memcpy(&marker, node + 0x368, sizeof(marker));
    std::memcpy(&mode, outline + 0xAC, sizeof(mode));
    if (marker != kMarker || (mode != 0x0001 && mode != 0x0100)) return;
    if (mode == 0x0001) {
        // Dialogue scripts can clear the native outline flag; replace its shadow item.
        std::memcpy(outline + 0x08, fill + 0x08, 0x40);
        float thickness = 0;
        std::memcpy(&thickness, node + 0x368, sizeof(thickness));
        const float width = thickness / glyphScale; // Native construction uses live XMM8.
        std::memcpy(outline + 0xB0, &width, sizeof(width));
        mode = 0x0100;
        std::memcpy(outline + 0xAC, &mode, sizeof(mode));
    }
    float width = 0;
    std::memcpy(&width, outline + 0xB0, sizeof(width));
    const auto percent = dialogue::CurrentSettings().outlinePercent;
    width *= kOutlineWidthMultiplier * (static_cast<float>(percent) / 100.0f);
    std::memcpy(outline + 0xB0, &width, sizeof(width));
    if (!percent) {
        // Hide only the freshly constructed secondary item's RGBA alpha.
        // A zero-width distance-field outline can otherwise still cover the fill.
        const float alpha = 0;
        std::memcpy(outline + 0xA4, &alpha, sizeof(alpha));
    }
    SoraOutlineCallback(node, fill, outline);
}
}

namespace {
bool install_site(unsigned char* target, unsigned span, const unsigned char* instructions, const void* bridgeAddress,
                  PVOID volatile* destination, bool draw = false) noexcept {
    if (!target) return false;
    const auto prefix = reinterpret_cast<std::uintptr_t>(target) % 8;
    auto* aligned = target - prefix;
    if (prefix + span > 8) {
        // A second core/ASI or another patch owns this site. Do not claim its hook.
        status(HookConflict);
        return false;
    }
    unsigned char original[8];
    std::memcpy(original, aligned, sizeof(original));
    if (std::memcmp(original+prefix, instructions, span) != 0) {
        status(HookConflict); return false;
    }
    if (draw) status(OriginalBytesMatch);
    auto* page = allocate_near(reinterpret_cast<std::uintptr_t>(target));
    if (!page) return false;
    // Relay is an absolute indirect JMP; its pointer is inline, so it clobbers no registers.
    const unsigned char absoluteJump[] = {0xFF, 0x25, 0, 0, 0, 0};
    std::memcpy(page, absoluteJump, sizeof(absoluteJump));
    const auto bridge = reinterpret_cast<std::uintptr_t>(bridgeAddress);
    std::memcpy(page + 6, &bridge, sizeof(bridge));
    auto* trampoline = page + 0x20;
    std::memcpy(trampoline, original + prefix, span);
    unsigned char replacement[8];
    std::memcpy(replacement, original, sizeof(replacement));
    // Encode the target jump using the actual target address, not this stack buffer.
    const auto delta = reinterpret_cast<std::intptr_t>(page) - reinterpret_cast<std::intptr_t>(target + 5);
    if (delta < (std::numeric_limits<std::int32_t>::min)() ||
        delta > (std::numeric_limits<std::int32_t>::max)() ||
        !relative_jump(trampoline + span, target + span)) {
        VirtualFree(page, 0, MEM_RELEASE); return false;
    }
    replacement[prefix] = 0xE9;
    const auto displacement = static_cast<std::int32_t>(delta);
    std::memcpy(replacement + prefix + 1, &displacement, sizeof(displacement));
    for (unsigned i = 5; i < span; ++i) replacement[prefix + i] = 0x90;
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
    std::memcpy(&expected, original, sizeof(expected));
    std::memcpy(&desired, replacement, sizeof(desired));
    InterlockedExchangePointer(destination, trampoline);
    const bool installed = InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(aligned),
                                                        desired, expected) == expected;
    DWORD ignored = 0;
    const bool restored = VirtualProtect(aligned, 8, oldProtection, &ignored) != FALSE;
    const bool flushed = FlushInstructionCache(GetCurrentProcess(), aligned, 8) != FALSE;
    if (!installed) {
        status(HookConflict);
        if (!restored || !flushed) status(MemoryFinalizeFailed);
        InterlockedExchangePointer(destination, nullptr);
        VirtualFree(page, 0, MEM_RELEASE);
        return false;
    }
    // Once published, the relay/trampoline remain valid even if a later maintenance API fails.
    if (!restored || !flushed) status(MemoryFinalizeFailed);
    if (draw) status(HookActive);
    return true;
}

bool install_layout() noexcept {
    auto* prepare = targets::Prepare();
    auto* spacing = targets::Spacing();
    auto* position = targets::Position();
    auto* destroy = targets::Destroy();
    auto* caller = targets::MainCaller();
    auto* measure = targets::Measure();
    auto* type = targets::BeltType();
    if (!prepare || !spacing || !position || !destroy || !caller || !measure || !type) {
        status(PatternMissing);
        return false;
    }
    std::int32_t displacement = 0;
    std::memcpy(&displacement, type+3, sizeof(displacement));
    dialogue::SetNativeTargets(caller+5, type+7+displacement,
        reinterpret_cast<void (*)(unsigned char*)>(measure));
    if (!install_site(prepare, 5, targets::kPrepare, &SoraPrepareBridge, &gSoraPrepareTrampoline) ||
        !install_site(spacing, 7, targets::kSpacing, &SoraSpacingBridge, &gSoraSpacingTrampoline) ||
        !install_site(position, 7, targets::kPosition, &SoraPositionBridge, &gSoraPositionTrampoline) ||
        !install_site(destroy, 5, targets::kDestroy, &SoraDestroyBridge, &gSoraDestroyTrampoline)) return false;
    // Partial installation remains pass-through; callbacks activate only together.
    dialogue::EnableLayout();
    return true;
}

BOOL CALLBACK initialize(PINIT_ONCE, PVOID, PVOID*) noexcept {
    auto* draw = targets::Draw();
    if (!draw) { status(PatternMissing | HookFailed | InitializationFinished); return TRUE; }
    status(TargetPatternFound);
    // Pin before any callback destination is published.
    HMODULE pinned = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           reinterpret_cast<LPCWSTR>(&initialize), &pinned)) {
        status(CorePinFailed | HookFailed | InitializationFinished);
        return TRUE;
    }
    status(CorePinned);
    if (!dialogue::StartConfig(pinned)) status(ConfigurationUnavailable);
    if (install_layout()) status(LayoutActive); else status(LayoutFailed);
    if (!install_site(draw, 6, targets::kDraw, &SoraHookBridge, &gSoraTrampoline, true)) status(HookFailed);
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
