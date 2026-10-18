// Copyright (c) 2026 DialogueOutline contributors
// SPDX-License-Identifier: MIT
#pragma once
#include <windows.h>

// A bitmask, not a Win32 error code. Only HookActive confirms this module's hook.
enum SoraDialogueStatus : DWORD {
    HostNameMatch = 0x001, HostHashMatch = 0x002, OriginalBytesMatch = 0x004,
    HookActive = 0x008, HostHashRejected = 0x010, HookConflict = 0x020,
    CorePinned = 0x040, HookFailed = 0x080, InitializationFinished = 0x100,
    CorePinFailed = 0x200, ForeignHost = 0x400, MemoryFinalizeFailed = 0x800,
    LayoutActive = 0x1000, LayoutFailed = 0x2000, ConfigurationUnavailable = 0x4000
};

extern "C" {
// Call synchronously outside DllMain/loader lock. One immutable result per module.
DWORD WINAPI SoraDialogueInitialize() noexcept;
DWORD WINAPI SoraGetStatus() noexcept;
// Internal offline inspection surface: 0 = black, 1 = white; no initialization.
DWORD WINAPI SoraChooseOutline(float r, float g, float b) noexcept;
// Internal offline inspection surface; caller owns valid node/fill/outline buffers.
void SoraOutlineCallback(const unsigned char* node, const unsigned char* fill,
                         unsigned char* outline) noexcept;
// Optional ASI entry; requires a loader invoking this outside its DllMain.
void WINAPI InitializeASI() noexcept;
}
