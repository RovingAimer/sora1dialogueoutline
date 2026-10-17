# 构建、实现与来源

## 环境与核心构建

核心需要 Windows x64、Python 3.10+、Visual Studio 2022 C++ Build Tools（MSVC x64、MASM）和 Windows SDK。源码仓库为 [sora1dialogueoutline](https://github.com/RovingAimer/sora1dialogueoutline)。在源码根目录运行：

```powershell
python build.py
```

核心输出到 `build/DialogueOutline.dll`，构建报告为 `build/core_report.json`。资源补丁需准备四个工作副本：`inputs/layout/subtitle.lay`、`inputs/layout/active_voice.lay`、`inputs/reference/window8.dds` 和 `inputs/reference/active_voice.dds`，然后运行：

```powershell
python tools/build_resources.py
```

脚本按 `resource_patches.json` 检查输入 SHA256 和每项修改，并将结果写入 `build/resources/`：主包文件位于 `build/resources/main/`，即时对白横条文件位于 `build/resources/optional_active_voice/`。

## 加载器兼容与当前工具

对白 v0.1.0 配合 [通用加载器 v0.2.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.2.0) 使用，组合已通过用户实机验收。加载器安装包中的模块声明指向 `SoraDialogueInitialize`；兼容来源见 [integration/README.md](integration/README.md) 和 [loader.lock.json](loader.lock.json)。

`src/DialogueOutline.def` 保留 `InitializeASI` 导出；通用模块入口为 `SoraDialogueInitialize`。原生构建命令为 `python build.py`。

字幕 v0.1.0 的 `release_plan.json` 是原发布来源记录，保持不变，记录原安装附件、90afbc DLL 和其原编译源码提交 `d8cf964fe1914cecc95296b86f752cc0eef9abbb`。当前文档更新不代表重编译了该 DLL。

## 当前描边实现

核心只接受文件名 `sora_1st.exe` 和 SHA256 `9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac`。原始指令窗口位于 RVA `0x46BDF8`，hook 入口为 RVA `0x46BDF9`。调用现场的 fill、node、outline 分别位于 RDI、RSI、RBX；原生字形尺度来自 XMM8，桥接后作为第四参数放入 XMM3。

节点 `+0x368` 的 `0.28125f`（`0x3E900000`）为专属标记。hook 接受描边模式 `0x0001` 和 `0x0100`：

- `0x0001` 项将 fill 的字节 `0x08..0x47` 复制到 outline，改为模式 `0x0100`，并以标记值除以字形尺度生成线宽。
- 两种模式都在字形项构建时将线宽乘以 `0.60`；模式 `0x0100` 使用原线宽后应用同一倍率。
- fill 的 RGB 与 alpha 保留；outline 的 RGB 写为自动选择的黑色或白色。

选色以每个字形项的有效填充色为输入：每通道先取 `itemRGB * nodeRGB` 并夹到 `0..1`，再按 sRGB 转线性值（`v <= 0.04045` 时为 `v / 12.92`，否则为 `((v + 0.055) / 1.055)^2.4`）。相对亮度为 `Y = 0.2126R + 0.7152G + 0.0722B`，黑白对比度分别为 `(Y + 0.05) / 0.05` 和 `1.05 / (Y + 0.05)`，选择较大者，相同时取黑色。选色在字形项创建时计算；动画颜色后续变化只有在字形项重建时才重新计算。

`src/core.cpp` 中的 `kOutlineWidthMultiplier` 默认值为 `0.60f`，同时作用于带标记的正文、姓名和即时对白横条。标记值保持 `0.28125f`。颜色与线宽可通过当前源码调整并重新运行 `python build.py`。

## 固定描边颜色

在 `src/core.cpp` 的 `SoraOutlineCallback` 中，当前自动选色代码为：

```cpp
const float selected = static_cast<float>(SoraChooseOutline(item[0] * modulation[0],
    item[1] * modulation[1], item[2] * modulation[2]));
const float rgb[] = {selected, selected, selected};
```

固定纯黑时，将完整的跨两行 `selected` 声明替换为 `const float selected = 0.0f;`；固定纯白时替换为 `const float selected = 1.0f;`，保留后面的 `rgb` 数组行。自定义颜色时，删除完整的 `selected` 声明，并将数组行替换为例如：

```cpp
const float rgb[] = {0.20f, 0.60f, 1.00f};
```

RGB 每个分量使用 `0..1`。修改后重新运行 `python build.py`；只有这类实际源码修改才会生成新的 DLL，原已发布 90afbc 二进制不随文档改变。

## 当前源码快照与发布工具

先提交干净的源码快照，并保留现有 `build/DialogueOutline.dll` 与 `build/core_report.json`。命令核对其编译输入后使用：

```powershell
python tools/package_release.py --source-only
```

该命令只准备源码 ZIP，不生成或重发 v0.1.0 安装 ZIP。输出位于 `build/packages/`，包含按当前提交命名的 `DialogueOutline-v0.1.0-source-<commit-12位>.zip`、`release_manifest.json` 和原 `native-build-report.json`。清单记录当前源码提交、复用的 90afbc DLL、原二进制来源提交和编译输入。

`python tools/publish_release.py` 默认验证已准备的本地附件并输出预览，不访问网络。未来版本的公开发布入口需要显式的 `--publish --plan PATH`；历史 `release_plan.json` 仅保留 v0.1.0 来源记录。

## 依赖与许可

核心和构建代码采用 MIT，完整许可见 [LICENSE](LICENSE)。加载器 fork、Detours、DDS 和游戏资源的来源及许可见 [THIRD_PARTY.md](THIRD_PARTY.md)。加载器的固定 Detours 提交为 `9764cebcb1a75940e68fa83d6730ffaf0f669401`；完整加载器源码包含该子模块及许可，GitHub 自动生成的加载器 Source code 归档不含子模块内容。
