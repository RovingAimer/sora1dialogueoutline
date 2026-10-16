# 构建与实现

## 环境和源码

核心需要 Windows x64、Python 3.10+、Visual Studio 2022 C++ Build Tools（MSVC x64、MASM）和 Windows SDK。完整源码位于 [sora1dialogueoutline](https://github.com/RovingAimer/sora1dialogueoutline)。

```powershell
git clone https://github.com/RovingAimer/sora1dialogueoutline.git
cd sora1dialogueoutline
python build.py
```

核心输出到 `build/DialogueOutline.dll`，构建报告为 `build/core_report.json`。构建使用当前源码、MSVC 和 Windows SDK，不需要游戏 EXE 或游戏运行时。

资源补丁需要四个工作副本：`inputs/layout/subtitle.lay`、`inputs/layout/active_voice.lay`、`inputs/reference/window8.dds`、`inputs/reference/active_voice.dds`。从自己的游戏和参考资源副本准备输入后运行：

```powershell
python tools/build_resources.py
```

脚本按 `resource_patches.json` 核对输入 SHA256 和每项修改，再写入 `build/resources/`。主包文件位于 `build/resources/main/`；即时气泡文件位于 `build/resources/optional_active_voice/`。

兼容加载器单独维护和构建。递归克隆会取得其固定 Detours 子模块：

```powershell
git clone --recursive https://github.com/RovingAimer/sora1looseload.git
cd sora1looseload
python build.py
```

加载器输出 `build/xinput1_4.dll` 和 `build/report.json`。它使用 Microsoft Detours，固定在提交 `9764cebcb1a75940e68fa83d6730ffaf0f669401`，并保留 Detours 自己的 MIT 版权与许可。独立描边核心不使用 Detours。加载器原始上游为 [Hinkiii/sora1looseload](https://github.com/Hinkiii/sora1looseload)，集成源码、构建及许可见 [RovingAimer fork](https://github.com/RovingAimer/sora1looseload)。

## 当前 hook

核心仅接受文件名 `sora_1st.exe` 和 SHA256 `9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac`。原始指令窗口位于 RVA `0x46BDF8`，hook 入口为 RVA `0x46BDF9`。调用现场的 fill、node、outline 分别在 RDI、RSI、RBX；原生字形尺度来自 XMM8，桥接后作为第四参数放入 XMM3。

节点 `+0x368` 的 `0.28125f`（`0x3E900000`）是专属标记。hook 接受描边模式 `0x0001` 和 `0x0100`：

- `0x0001` 项会把 fill 的字节 `0x08..0x47` 复制到 outline，改为模式 `0x0100`，并以标记值除以字形尺度生成线宽。
- 两种模式都在本次原生字形项构建时将线宽乘以 `0.60`。模式 `0x0100` 使用原有线宽后应用同一倍率。
- fill 的 RGB 与 alpha 保留；outline 的 RGB 写入自动选出的黑色或白色。

选色以每个字形项的有效填充色为输入，计算过程如下：

1. 每通道取 `itemRGB * nodeRGB` 并夹到 `0..1`。
2. 按 sRGB 转线性值：`v <= 0.04045` 时为 `v / 12.92`；否则为 `((v + 0.055) / 1.055)^2.4`。
3. 计算相对亮度 `Y = 0.2126R + 0.7152G + 0.0722B`。
4. 黑色对比度为 `(Y + 0.05) / 0.05`；白色对比度为 `1.05 / (Y + 0.05)`。取较大者，相同时取黑色。返回值 `0` 表示黑，`1` 表示白。

选色发生在字形项创建时。若后续颜色动画没有重建字形项，描边颜色不会重新计算。

## 快速调整描边粗细

修改 `src/core.cpp` 顶部的常量：

```cpp
constexpr float kOutlineWidthMultiplier = 0.60f;
```

默认值为 `0.60f`。减小数值可变细，增大数值可加粗；例如 `0.50f` 较细，`0.70f` 较粗。该倍率同时作用于带标记的正文、姓名和即时气泡。布局的 `0.28125f` 标记保持原值。

修改后运行 `python build.py`，退出游戏，再替换 `mods/DialogueOutline/DialogueOutline.dll`。资源文件和加载器保持原样。

## 固定描边颜色

`src/core.cpp` 的 `SoraOutlineCallback` 中，自动选择由下面这行完成：

```cpp
const float selected = static_cast<float>(SoraChooseOutline(item[0] * modulation[0],
    item[1] * modulation[1], item[2] * modulation[2]));
const float rgb[] = {selected, selected, selected};
```

固定纯白时，将完整的两行 `selected` 声明替换为 `const float selected = 1.0f;`；固定纯黑时替换为 `const float selected = 0.0f;`。自定义描边 RGB 时，删除完整的 `selected` 声明，并将数组行替换为例如：

```cpp
const float rgb[] = {0.20f, 0.60f, 1.00f};
```

每个分量使用 `0..1`。修改后运行 `python build.py`，退出游戏，再替换游戏目录中的 `mods/DialogueOutline/DialogueOutline.dll`。颜色和粗细由源码常量及选色函数控制，修改后重新构建生效。
