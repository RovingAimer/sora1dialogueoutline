# DialogueOutline

当前版本是《空之轨迹 the 1st》的**初版开发候选**，尚未实机验证，不是正式发布成品。安装前请阅读[安装说明](INSTALL.md)；构建信息见[BUILDING.md](BUILDING.md)，第三方来源与授权边界见[THIRD_PARTY.md](THIRD_PARTY.md)。

## 效果范围

仅处理带有明确标记的剧情对白正文和角色名。可选包另加即时对话气泡。保留文字原色，在生成文字绘制内容时按有效文字颜色选择对比度较高的黑色或白色描边。描边颜色选择发生在文字生成时；之后的调色、亮度变化若未重新生成文字，不保证重新选色。菜单、日志及其他未标记文字不属于目标范围。

核心仅接受文件名为 `sora_1st.exe` 且 SHA256 为 `9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac` 的目标程序。更新版、其他版本以及《空之轨迹 the 2nd》均未支持。实际画面、性能、与其他 hook 的兼容性尚未实机验证。

## 核心模块与资源

`source/src` 构建为独立的 `DialogueOutline.dll` 核心。它不含 XInput 或 D3D 接口，不加载原资源加载器，不写日志。`DllMain` 不执行初始化工作；接入方必须在 DllMain 与 loader lock 之外显式调用 `DWORD WINAPI SoraDialogueInitialize()`。调用是同步且幂等的，同一模块会缓存初始化结果。

主资源安装路径如下：

```text
游戏目录/
├─ mods/DialogueOutline/DialogueOutline.dll
├─ layout/subtitle.lay
└─ asset/dx11/image/window8.dds
```

可选即时对话气泡另提供：

```text
游戏目录/
├─ layout/active_voice.lay
└─ asset/dx11/image/active_voice.dds
```

核心本身不会扫描或加载 `mods` 目录。上面的模块路径由接入它的 loader 负责加载；资源则按游戏相对路径读取。

## 选择一种接入方式

同一个核心只能通过一种入口初始化。loader 接入路线（下述 A 或 C）与 ASI 路线（B）互斥；可选即时对话资源包可以与任一路线搭配。

**A. 使用已有兼容 loader 的主包**：主包 `1st_初版_主接入包.zip` 包含 `mods/DialogueOutline/DialogueOutline.dll` 和主资源。loader 必须从该固定路径加载核心，并在 DllMain 与 loader lock 外调用 `SoraDialogueInitialize()`。Hinkiii 的 [sora1looseload](https://github.com/Hinkiii/sora1looseload) 上游没有插件接入 API；对应最小补丁以提交 [`04e898e369e3019d5aa2cb13a7209de39c643a4a`](https://github.com/Hinkiii/sora1looseload/commit/04e898e369e3019d5aa2cb13a7209de39c643a4a) 为基线，见[补丁文件](integration/sora1looseload-dialogue-outline.patch)与[固定路径 helper](integration/dialogue_outline_loader.h)。维护者需采纳补丁，或用户自行构建含接入改动的 loader。把核心 DLL 放进原版 stockloader 目录不会使其自动生效。

**B. ASI（可选替代入口）**：`python build.py --asi` 生成与核心 DLL 字节相同的 `DialogueOutline.asi`。可选 ASI 包 `1st_初版_可选ASI入口.zip` 包含 `plugins/DialogueOutline.asi` 与主资源，不含 `mods/DialogueOutline/DialogueOutline.dll`，因此无需同时安装 A 主包。它需要兼容的 Ultimate ASI Loader (UAL)；当前实现导出 `InitializeASI`，UAL 默认 `DontLoadFromDllMain=1`。UAL 不随包提供。玩家可复用已有兼容 UAL；若没有，也可自行选择安装[官方 UAL](https://github.com/ThirteenAG/Ultimate-ASI-Loader)，先核对同名入口文件及其他 Mod 的冲突。目标 EXE 静态导入 `d3d11.dll` 与 `dxgi.dll`，对应 UAL 入口尚未实机验证；不要通过随意改名或覆盖代理 DLL 来假定入口可用。

**C. 集成 XInput loader（可选）**：主包不含 `xinput1_4.dll`；可选接入版 loader 单独生成并需玩家明确选择。该路线替换游戏当前使用的 `xinput1_4.dll`，必须与 A 主包一同使用，因为核心 DLL 和主资源仍由 A 提供。安装前备份现有 XInput DLL。可在 `source` 目录运行 `python integration/build_loader.py`，按锁定的上游源码和补丁构建 `source/build/optional_loader/xinput1_4.dll`；可选包名为 `1st_初版_可选接入版加载器.zip`。报告显示它未实机验证。不得把旧 v0.2 代理改名后当作集成 loader。

Hinkiii 原版 stockloader 不会因复制 A 主包而自动获得接入能力。首次让其支持核心，需要 loader 维护者采纳源码改动，或用户明确升级/构建兼容 loader。不要把这件事描述为普通资源复制。不要同时采用 B ASI 与 A/C loader 接入，以免同一核心初始化两次。

## 构建与资源

在 `source` 目录运行：

```powershell
python build.py
python build.py --asi
python tools/build_resources.py
```

核心构建要求 Windows x64、Python 3.10+、Visual Studio 2022 Build Tools（含 x64 MSVC、MASM 和 Windows SDK）。`build.py` 的输出固定在 `source/build/`，不读取游戏 EXE、PAC、原 loader 或游戏资源，也不运行生成的 DLL。`--asi` 是可选附件名，不是另一种核心实现。构建只证明本机编译和静态 PE 契约，不能证明游戏运行正常。具体构建报告若存在，应与二进制 SHA256 一起保存。

`tools/build_resources.py` 需要用户在工作副本中准备以下输入：

- `inputs/layout/subtitle.lay`
- `inputs/layout/active_voice.lay`
- `inputs/reference/window8.dds`
- `inputs/reference/active_voice.dds`

工具校验源文件 SHA256 和允许修改的布局字段，输出至 `build/resources/main/` 与 `build/resources/optional_active_voice/`。主包与可选资源可以分别安装。该工具不会从游戏目录提取资源。

公开源码分发不包含这些输入、游戏 EXE、PAC 文件或原 loader DLL；`inputs/` 与 `build/` 是本地工作数据。游戏资源和参考贴图的授权独立于代码的 MIT 许可证，详见[第三方说明](THIRD_PARTY.md)。

发布源码仓库时保留根目录 `.gitattributes`。它确保 Git 克隆时保留原始文件字节，避免 Windows 自动换行转换使锁定的上游源码校验失败。

## 构建证据与打包

当前离线构建报告记录核心 DLL 为 123392 字节，SHA256 为 `2c06dc46834d98d826797c50247d8589c73db32d0f579c294e784d108d6d6a20`；ASI 与核心字节相同。可选集成 loader 报告记录 `xinput1_4.dll` 为 205824 字节，SHA256 为 `4f389af61ed053087125986f5712a5550e5b59ec04b4b34a376265f5f78a3325`。这些报告均标记未实机验证；未执行生成的 DLL，也未启动游戏。

在 `source` 目录运行 `python tools/package_release.py` 可校验已有构建报告、当前源码与资源，再生成 `DialogueOutline_初版_MIT源码.zip`、`1st_初版_主接入包.zip` 和 `1st_初版_可选即时对话.zip`。即时对话包是资源覆盖包，需与主资源一起使用。若已有对应构建产物，还会生成 `1st_初版_可选ASI入口.zip` 和/或 `1st_初版_可选接入版加载器.zip`。输出位于 `build/packages/`；打包脚本不会构建或安装文件。当前附有公开构建工作流 `.github/workflows/build.yml`，但尚未在 GitHub 上运行。
