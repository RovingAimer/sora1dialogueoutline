# 原 loose loader 的最小接入

主源码形式是独立 `mods/DialogueOutline/DialogueOutline.dll` 与本目录的固定路径接入补丁。原版 Hinkiii/sora1looseload 没有插件 API，不能仅放入 DLL 或 ASI 就运行新代码。第一次使用必须让用户自己的加载器具备本接入支持。`build/optional_loader/xinput1_4.dll` 是明确自选的整合替代加载器，不是资源文件，也不能在说明中默认为必须覆盖现有加载器。

补丁只针对 `04e898e369e3019d5aa2cb13a7209de39c643a4a`，上游文件原样保存在 `third_party/sora1looseload/`，校验和在该目录的 `UPSTREAM_LOCK.json`。未附带任何来自私人游戏目录的 DLL 或游戏资源。

## 接口与时机

`dialogue_outline_loader.h` 加入固定 helper，原 `dllmain.cpp` 只增加 include、检查 `DetourTransactionCommit()` 结果、成功后调用 helper。`sora1looseload-dialogue-outline.patch` 同时包含这两项，可在锁定上游检出目录执行 `git apply <补丁绝对路径>`；保留原 `proxy.cpp` 和工程文件即可。

加载路径以 `GetModuleFileNameW(nullptr)` 所得可执行文件目录为起点，固定为 `mods\DialogueOutline\DialogueOutline.dll`，不依赖当前目录、不扫描任何插件目录、不占用其他代理 DLL 名称。加载标志为 `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS`，不修改进程全局搜索路径。

helper 在现有工作线程里、Detours 事务提交成功之后运行。它先 `LoadLibraryExW`，返回后查找并同步调用 `DWORD WINAPI SoraDialogueInitialize()`；这时已经离开本次系统 DLL 初始化的 loader lock。调用时不持有 `g_localeMutex`，也不在 Detours 事务内。核心必须保持 `DllMain` 最小化并令显式初始化幂等。返回值是状态位图，只有 `HookActive=0x8` 表示本核心 hook 激活；非零不是通用成功码。

缺少可选核心文件时静默跳过。路径错误、加载失败、缺导出及初始化状态写入原加载器日志，不撤销已成功建立的资源 detours。所有成功加载的模块保留到进程结束，不在核心初始化后 `FreeLibrary`；核心会在发布 hook 前 pin 自身。接入点或 hook 失败不等于资源安装成功，具体界面仍需用户验证。

## 离线构建可选整合加载器

需要 Windows、Python 3、带 x64 C++ 工具的 Visual Studio Build Tools（本次为 MSVC 14.51）及 Windows SDK。运行：

```powershell
python .\integration\build_loader.py
```

脚本仅用 Python 标准库，自动读取 MSVC 安装位置，先核对锁定源码，再在 `build/optional_loader` 生成打过补丁的 `dllmain.cpp`、编译完整 Detours 静态库、编译原 loader 和 helper，最后链接 x64 `xinput1_4.dll`。不改写 vendored 上游源码，不安装、不执行所生成 DLL。头文件与生成 patch 必须一致，否则构建失败。Detours 的 `src/`、构建 makefile、README、credits、两份 MIT license 均已附带；不依赖其样例、测试、CI 或预编译二进制。

原工程使用 VS v143、Windows SDK 10.0、x64，x64 目标名为 `xinput1_4`；预构建步骤通过 `nmake` 构建 Detours。这里的独立脚本改用相同 Detours 源文件清单直接调用 MSVC/lib/link，使用 `/MT` 静态 CRT，避免要求用户安装 Git 子模块或取得上游二进制。

`report.json` 记录源码 commit、依赖 commit、编译器、集成文件校验值、DLL 校验值和未执行声明；`pe_inspection.txt` 是只读 PE 检查，确认七个原 XInput 导出及序号。不要把构建通过称为游戏中验证通过。

## 许可证和边界

Hinkiii/sora1looseload 为 MIT，Copyright (c) 2025 Hinkiii；Microsoft Detours 为 MIT，Copyright (c) Microsoft Corporation。分发修改源码、依赖源码或二进制时保留相应版权与许可全文，本项目新增接入文件沿用源码包根目录 MIT。许可证原文是依据，本说明不替代它。

本补丁保留上游资源扫描、日志、locale hook 和 XInput 转发逻辑。上游自身的 detached worker 生命周期、运行时签名扫描、资源缓存首次加载时机与线程协调没有在本补丁重做；因此没有保证任意游戏版本或其他 native Mod 均兼容。现有 stock loader 文件也未证明就是此上游 commit 的同一构建。可选 ASI 需要实际 ASI loader 调用 `InitializeASI`，并配置从其 `DllMain` 外调用；本补丁不安装 ASI loader。
