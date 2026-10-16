# Standalone native core: offline build and integration

Copyright (c) 2026 DialogueOutline contributors. Code is MIT licensed; see
`LICENSE`. This is an offline candidate, **not game-validated**.

## Build

Requirements: Windows x64, Python 3.10+ (standard library only), Visual Studio
2022 Build Tools or Visual Studio with **Desktop development with C++**, x64
MSVC tools, MASM and a Windows SDK. The installed Visual Studio `vswhere.exe`
locates the toolchain; `vcvars64.bat` supplies compiler and SDK paths.

Copy this entire `source` directory into any ordinary writable workspace.
From that copy, run:

```powershell
python build.py
# Optional alternative binary name for a compatible ASI loader:
python build.py --asi
```

All output is fixed to the adjacent `build/` directory. There is no install or
arbitrary output-path option. The build reads no game executable, research
copy, game DLL, original loader or game resources, and never loads the produced
DLL or starts the game. Resource generation is a separate tool/workflow.
Existing `build/resources/` is left intact.

The compiler uses `/MT /O2 /W4 /std:c++17 /EHsc /utf-8`; linking uses `/Brepro`,
ASLR and NX. `/Brepro` removes the normal link timestamp; it does not by itself
prove byte-for-byte reproduction across different compiler/SDK installations.
`build/core_report.json` records source SHA256 values, compiler/version, SDK,
flags, output SHA256, every export and every direct import. Build diagnostics,
PE inspection, bridge disassembly, map, objects and import library stay in
`build/`. Only Windows `KERNEL32.dll` and `bcrypt.dll` direct dependencies are
allowed by the build check. There are no XInput/D3D exports or imports, no
original-loader dependency, and no arbitrary third-party DLL loader in the core.
Windows may load its own dependency/provider modules for these APIs.

`DialogueOutline.dll` is the main product. `--asi` additionally creates
`DialogueOutline.asi` with exactly the same bytes. Use one attachment method;
do not install both copies. A default build removes a stale ASI product only.

## Primary attachment: original loader source integration

The preferred route is an explicit call from the existing original loader's
source integration. Its own outer worker/entry must already be **outside
DllMain and the loader lock** before loading the core and calling:

```cpp
using Initialize = DWORD (WINAPI*)();
// Resolve SoraDialogueInitialize from the explicitly loaded DialogueOutline.dll.
// Call synchronously outside DllMain; retain and inspect the returned bitmask.
```

The companion loader integration uses the fixed path
`mods/DialogueOutline/DialogueOutline.dll` relative to the loader/game directory.
It owns that load; the core itself does not load the original loader or other
third-party DLLs. The core is not a replacement `xinput1_4.dll`. This document
does not authorize installation or execution in a game directory.

`src/DialogueOutline.h` is the API/status contract. Exports:

| Export | Contract |
| --- | --- |
| `DWORD WINAPI SoraDialogueInitialize()` | Explicit, synchronous, thread-safe `InitOnce` initialization; cached success or refusal per module. Call outside DllMain/loader lock. |
| `DWORD WINAPI SoraGetStatus()` | Read current status; zero before explicit initialization. A concurrent observer may see intermediate bits until `InitializationFinished`. |
| `DWORD WINAPI SoraChooseOutline(float r,float g,float b)` | Internal/offline inspection helper. Returns 0 black or 1 white. Does not initialize. |
| `void SoraOutlineCallback(const unsigned char*,const unsigned char*,unsigned char*)` | Internal/offline inspection helper. Requires valid node (at least `0x36C` bytes), fill and outline buffers (at least `0xAE` bytes). No pointer validation or stable general-purpose public buffer ABI. |
| `void WINAPI InitializeASI()` | Calls the same initializer synchronously; optional loader entry. |

`DllMain` only returns `TRUE`: no threads, host inspection, module loads, logs,
or hooks. Loading into Python therefore starts no game logic. Explicit
initialization in a host not named `sora_1st.exe` returns `0x500`
(`ForeignHost | InitializationFinished`), without hashing, pinning or hooking.
Calling the initializer under any DllMain/loader lock violates its contract;
the core does not attempt to detect loader-lock ownership.

## Optional ASI attachment

Only a compatible loader that invokes the exported `InitializeASI` **outside
DllMain**, such as current Ultimate ASI Loader with
`DontLoadFromDllMain=1`, is supported. Legacy ASI loaders relying solely on
plugin `DllMain` do not activate this module. ASI and replacement-DLL routes
are optional alternatives; original-loader source integration remains primary.
There is no XInput proxy/replacement binary in this source tree.

## Initialization and status

Initialization requires the host filename `sora_1st.exe`, SHA256
`9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac`, and the exact
original instruction window `00 FF 86 88 03 00 00 48` at RVA `0x46BDF8`.
Only after host name/hash verification is the core pinned for process lifetime.
A matching host with a failed install can also remain pinned; no unload API is
provided. The executable hook starts at RVA `0x46BDF9` and is published by an
aligned 8-byte compare-and-exchange after preparing the relay/trampoline.

| Bit | Name | Meaning |
| --- | --- | --- |
| `0x001` | HostNameMatch | Expected host filename. |
| `0x002` | HostHashMatch | Exact executable file SHA256. |
| `0x004` | OriginalBytesMatch | Expected original instruction window observed. |
| `0x008` | HookActive | **This module** successfully published its hook. |
| `0x010` | HostHashRejected | Hash mismatch or hash/read failure. |
| `0x020` | HookConflict | Original bytes differ or CAS publication lost to another patch. |
| `0x040` | CorePinned | This module has been pinned for process lifetime. |
| `0x080` | HookFailed | Hook was refused or could not be installed. |
| `0x100` | InitializationFinished | Cached initialization result is complete. |
| `0x200` | CorePinFailed | Could not pin callback/bridge code. |
| `0x400` | ForeignHost | Wrong host name or unavailable host path; no hook attempted. |
| `0x800` | MemoryFinalizeFailed | Protection/cache maintenance failed after patch attempt; inspect with other bits. |

Normal successful installation is `0x14F`. `HookConflict` is not proof another
DialogueOutline copy works: its owner is unknown. If both a DLL and ASI copy
initialize, the second refuses the changed site and does **not** set
`HookActive`. Same-module repeated calls return their original final result.
Once published, relay/trampoline memory is retained even if later cache or
protection maintenance fails. `HookActive | MemoryFinalizeFailed` needs review;
it must not be presented as clean activation.

## Behavior and current limits

The original native CPU hook, field offsets and bridge have been retained.
Only an exact resource marker `0.28125f` (`0x3E900000`) at node `+0x368` together
with outline mode `0x0100` is eligible; screen position/color/size heuristics
are not used. Marked resources must be supplied separately. The callback
leaves fill RGB, fill alpha and outline alpha unchanged. At item construction,
it multiplies fill RGB by node RGB and chooses whichever of black/white has
higher sRGB relative-luminance contrast. Later node tint or global brightness
changes without rebuilding the item may not trigger a new selection. This
is not proof of draw-time adaptation to all shader/material effects.

The bridge preserves volatile GPRs, flags, MXCSR and XMM0–XMM5, then executes
the original increment through the trampoline. Its existing limitation remains:
no synthetic SEH unwind metadata and no AVX upper-register preservation.
`noexcept` prevents C++ exception propagation but does not make invalid memory
or asynchronous exceptions unwind-safe. Do not extend the callback to throw.

No in-game execution or original-loader execution is performed here. Marked
node persistence, native material behavior, dialogue-only appearance, runtime
performance, and compatibility with other patches still require user game
validation. Only the specified 1st executable is accepted; no 2nd compatibility
is claimed. A successful offline build proves compilation and PE contract,
not game behavior.
