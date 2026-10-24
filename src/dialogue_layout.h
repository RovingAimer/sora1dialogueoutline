// Copyright (c) 2026 DialogueOutline contributors
// SPDX-License-Identifier: MIT
#pragma once
#include <windows.h>

namespace dialogue {
void EnableLayout() noexcept;
void SetNativeTargets(const void* caller, const void* beltType, void (*measure)(unsigned char*)) noexcept;
}
extern "C" {
void SoraLayoutPrepare(unsigned char* node, const void* nativeCaller) noexcept;
void SoraLayoutPosition(unsigned char* node) noexcept;
void SoraLayoutSpacing(unsigned char* node) noexcept;
void SoraLayoutDestroy(unsigned char* node) noexcept;
}
