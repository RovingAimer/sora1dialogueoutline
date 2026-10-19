// Copyright (c) 2026 DialogueOutline contributors
// SPDX-License-Identifier: MIT
#pragma once
#include <Windows.h>
#include <cstdint>

namespace dialogue {
struct Settings {
    std::uint32_t textPercent = 100;
    std::uint32_t heightPercent = 100;
    std::uint32_t namePercent = 100;
    std::uint32_t outlinePercent = 100;
};

// Call once after pinning this module. Failure leaves the baseline usable.
// The return value indicates whether save notifications are active.
bool StartConfig(HMODULE module) noexcept;
Settings CurrentSettings() noexcept;
// Watch failure takes precedence; otherwise reports the latest read/parse result.
DWORD ConfigurationStatus() noexcept;
}
