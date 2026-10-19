// Copyright (c) 2026 DialogueOutline contributors
// SPDX-License-Identifier: MIT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "dialogue_layout.h"
#include "runtime_config.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_map>

namespace {
template<class T> T Read(const unsigned char* node, std::size_t at) noexcept {
    T value;
    std::memcpy(&value, node + at, sizeof(value));
    return value;
}
template<class T> void Write(unsigned char* node, std::size_t at, T value) noexcept {
    std::memcpy(node + at, &value, sizeof(value));
}
bool Named(const unsigned char* node, const char* name, std::size_t length) noexcept {
    return node && Read<std::uint32_t>(node, 0xB8) == length &&
        std::memcmp(node + 0x78, name, length) == 0;
}
struct Roles {
    unsigned char* parent = nullptr;
    unsigned char* name = nullptr;
    unsigned char* previous = nullptr;
    unsigned char* belt = nullptr;
};
bool MainBody(unsigned char* node, Roles& roles) noexcept {
    // Script-controlled outline thickness is not a persistent node identity.
    if (Read<std::uint32_t>(node, 0x6C) != 0xF0000006 || !Named(node, "text", 4)) return false;
    roles.parent = Read<unsigned char*>(node, 0x70);
    if (!roles.parent || Read<std::uint32_t>(roles.parent, 0x6C) != 0xF0000000) return false;
    const auto count = Read<std::size_t>(roles.parent, 0x260);
    auto** children = Read<unsigned char**>(roles.parent, 0x258);
    if (!children) return false;
    bool containsBody = false;
    // These are the direct, named siblings in subtitle.lay's Root.
    for (std::size_t i = 0; i < count; ++i) {
        auto* child = children[i];
        if (child == node) containsBody = true;
        if (Named(child, "name_text", 9) && Read<std::uint32_t>(child, 0x6C) == 0xF0000007)
            roles.name = child;
        else if (Named(child, "prev_name_text", 14) && Read<std::uint32_t>(child, 0x6C) == 0xF0000009)
            roles.previous = child;
        else if (Named(child, "belt", 4) && Read<std::uint32_t>(child, 0x6C) == 0xF0000001 &&
                 *reinterpret_cast<const std::uintptr_t*>(child) ==
                     reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr)) + 0x9CF4B8)
            roles.belt = child;
    }
    return containsBody && roles.name && roles.previous && roles.belt;
}
template<class T> struct Field {
    T base{};
    T applied{};
    void Observe(T actual) noexcept {
        if (actual != applied) base = actual;
    }
};
struct Position {
    unsigned char* node = nullptr;
    Field<float> y;
};
struct NameFont {
    unsigned char* node = nullptr;
    Field<std::int32_t> size;
};
struct State {
    Roles roles;
    dialogue::Settings settings;
    Field<std::int32_t> font, spacing;
    Field<float> height, beltHeight;
    Position body, name, previous, belt;
    NameFont nameFont, previousFont;
};
std::atomic<bool> gEnabled{false};
SRWLOCK gLock = SRWLOCK_INIT;
std::unordered_map<unsigned char*, State> gStates;
struct Lock {
    Lock() noexcept { AcquireSRWLockExclusive(&gLock); }
    ~Lock() { ReleaseSRWLockExclusive(&gLock); }
};
std::int32_t Scale(std::int32_t value, std::uint32_t percent) noexcept {
    return static_cast<std::int32_t>(std::lround(static_cast<double>(value) * percent / 100.0));
}
bool ResizeName(NameFont& font, unsigned char* node, std::uint32_t percent) noexcept {
    if (font.node != node) font = {node, {}};
    const auto current = Read<std::int32_t>(node, 0x35C);
    font.size.Observe(current);
    font.size.applied = (std::max)(1, Scale(font.size.base, percent));
    if (current == font.size.applied) return false;
    Write(node, 0x35C, font.size.applied);
    Write<std::uint16_t>(node, 0x6E0, 0x0101);
    return true;
}
void Move(Position& position, unsigned char* node, float delta) noexcept {
    if (position.node != node) position = {node, {}};
    // Track the animation's start position, which stays stable while its live
    // position interpolates. Translate start/live/target together, preserving
    // native duration, elapsed time and curve.
    const float current = Read<float>(node, 0xF0);
    position.y.Observe(current);
    const float desired = position.y.base + delta;
    position.y.applied = desired;
    if (current == desired) return;
    const float difference = desired - current;
    for (const auto at : {0x110, 0xF0, 0x100}) Write(node, at, Read<float>(node, at) + difference);
}
}

namespace dialogue {
void EnableLayout() noexcept { gEnabled.store(true, std::memory_order_release); }
}

extern "C" void SoraLayoutPrepare(unsigned char* node, const void* nativeCaller) noexcept {
    if (!gEnabled.load(std::memory_order_acquire)) return;
    Roles roles;
    if (!MainBody(node, roles)) return;
    const auto flags = Read<std::uint32_t>(node, 0x340);
    const bool restoredOutline = Read<std::uint32_t>(node, 0x368) != 0x3E900000 || !(flags & 0x80);
    if (restoredOutline) {
        // The native controller applies CG/dialogue styling before this Prepare.
        // Restore only this verified main-body node, then let native construction
        // rebuild the secondary item with the configured width and contrast.
        Write<std::uint32_t>(node, 0x368, 0x3E900000);
        Write<std::uint32_t>(node, 0x340, flags | 0x80);
        Write<std::uint16_t>(node, 0x6E0, 0x0101);
    }
    bool remeasure = false;
    bool measureName = false;
    bool measurePrevious = false;
    try {
        Lock lock;
        auto& state = gStates[node];
        if (state.roles.parent != roles.parent) state = {};
        if (state.roles.belt != roles.belt) state.beltHeight = {};
        state.roles = roles;
        state.settings = dialogue::CurrentSettings();
        const auto oldFont = Read<std::int32_t>(node, 0x35C);
        const auto oldSpacing = Read<std::int32_t>(node, 0x34C);
        const auto oldHeight = Read<float>(node, 0x334);
        const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        if (nativeCaller == reinterpret_cast<const void*>(base + 0x457FA4)) {
            // The controller just assigned its original 31/33/35 size. A new
            // original can equal our last result, so value comparison is insufficient.
            state.font.base = oldFont;
        } else {
            state.font.Observe(oldFont);
        }
        state.spacing.Observe(oldSpacing);
        state.height.Observe(oldHeight);
        state.font.applied = (std::max)(1, Scale(state.font.base, state.settings.textPercent));
        state.spacing.applied = Scale(state.spacing.base, state.settings.textPercent);
        state.height.applied = state.height.base * (state.settings.heightPercent / 100.0f);
        remeasure = restoredOutline || oldFont != state.font.applied || oldSpacing != state.spacing.applied ||
                    oldHeight != state.height.applied;
        Write(node, 0x35C, state.font.applied);
        Write(node, 0x34C, state.spacing.applied);
        Write(node, 0x334, state.height.applied);
        // This body preparation precedes both name setters and the controller's
        // width-based name positioning, even when the name text is unchanged.
        measureName = ResizeName(state.nameFont, roles.name, state.settings.namePercent);
        measurePrevious = ResizeName(state.previousFont, roles.previous, state.settings.namePercent);
    } catch (...) {
        return; // An allocation failure leaves the native preparation path available.
    }
    if (remeasure || measureName || measurePrevious) {
        // Text setters already measure before Prepare. Recalculate those metrics
        // with the new sizes, then let the original Prepare build its persistent
        // layout/typewriter state normally. Do not reset it from a render callback.
        const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        const auto measure = reinterpret_cast<void (*)(unsigned char*)>(base + 0x46E760);
        if (remeasure) measure(node);
        if (measureName) measure(roles.name);
        if (measurePrevious) measure(roles.previous);
    }
}

extern "C" void SoraLayoutPosition(unsigned char* node) noexcept {
    if (!gEnabled.load(std::memory_order_acquire)) return;
    Roles roles;
    if (!MainBody(node, roles)) return;
    Lock lock;
    const auto found = gStates.find(node);
    if (found == gStates.end()) return;
    auto& state = found->second;
    if (state.roles.parent != roles.parent) return;
    if (state.roles.belt != roles.belt) state.beltHeight = {};
    state.roles = roles;
    const float shift = state.height.base - state.height.applied;
    Move(state.body, node, shift);
    Move(state.name, state.roles.name, shift);
    Move(state.previous, state.roles.previous, shift);
    auto* belt = state.roles.belt;
    state.beltHeight.Observe(Read<float>(belt, 0x384));
    state.beltHeight.applied = state.beltHeight.base * (state.settings.heightPercent / 100.0f);
    Write(belt, 0x384, state.beltHeight.applied);
    Move(state.belt, belt, state.beltHeight.base - state.beltHeight.applied);
    // okuri_icon remains at its original bottom-aligned position.
}

extern "C" void SoraLayoutSpacing(unsigned char* node) noexcept {
    if (!gEnabled.load(std::memory_order_acquire)) return;
    if (Read<std::uint32_t>(node, 0x368) != 0x3E900000 ||
        Read<std::uint32_t>(node, 0x6C) != 0xF0000006) return;
    Lock lock;
    const auto found = gStates.find(node);
    if (found == gStates.end()) return;
    auto& state = found->second;
    const auto current = Read<std::int32_t>(node, 0x34C);
    state.spacing.Observe(current);
    state.spacing.applied = Scale(state.spacing.base, state.settings.textPercent);
    if (state.spacing.applied != current) {
        Write(node, 0x34C, state.spacing.applied);
        // The controller may set 8/-6 after Prepare. Its normal dirty path now
        // measures the final spacing without resetting the typewriter packet.
        Write<std::uint16_t>(node, 0x6E0, 0x0101);
    }
}

extern "C" void SoraLayoutDestroy(unsigned char* node) noexcept {
    Lock lock;
    gStates.erase(node); // The actual text destructor precedes every node release.
    for (auto& entry : gStates) {
        auto& state = entry.second;
        if (state.nameFont.node == node) state.nameFont = {};
        if (state.previousFont.node == node) state.previousFont = {};
        if (state.name.node == node) state.name = {};
        if (state.previous.node == node) state.previous = {};
    }
}
