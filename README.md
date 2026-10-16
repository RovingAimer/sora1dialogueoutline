# sora1dialogueoutline

《空之轨迹 the 1st》对白透明背景与自适应文字描边 Mod。保留文字原色，以黑色或白色描边提高可读性，并移除对白文字的偏移阴影。

## 下载与安装

当前发布为 [v0.1.0](https://github.com/RovingAimer/sora1dialogueoutline/releases/tag/v0.1.0)。

| 下载 | 用途 |
| --- | --- |
| [配套加载器 v0.1.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.1.0) | 加载对白核心，安装文件为 `xinput1_4.dll`。 |
| [主包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.0/DialogueOutline-v0.1.0-main.zip) | 必装：主对白正文、姓名描边及透明背景。 |
| [即时对白横条包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.0/DialogueOutline-v0.1.0-active-voice.zip) | 可选：带头像的即时对白横条，背景透明、文字描边；与主包一起使用。 |

退出游戏并备份将被覆盖的文件，先安装配套加载器，再将主包解压合并到 `sora_1st.exe` 所在目录；需要即时对白横条效果时，再合并可选包。保留 ZIP 内的目录结构。

仅支持 [INSTALL.md](INSTALL.md) 列出的游戏 EXE 版本。该文档包含完整文件清单、版本核对和卸载恢复步骤。人物头顶跟随角色移动的气泡暂未纳入此发布。

## 方案与当前实现

布局补丁为主对白正文、姓名及可选即时对白横条的文字节点写入专属标记；DDS 提供对应背景的透明贴图。核心在游戏构建字形副层时检查标记，将偏移阴影项转换为原生描边几何。这一接入点能使用实际字形和有效文字色，并将描边范围限定到带标记的对白节点。

核心保留填充文字的 RGB 与 alpha，在每个字形项创建时按文字色选择对比度更高的纯黑或纯白描边。选色使用填充 RGB 与节点 RGB 的乘积；后续颜色变化在字形项重建时重新计算。默认线宽系数为 `0.60`。

粗细、固定颜色、自定义 RGB 和自适应算法的调整方式见 [BUILDING.md](BUILDING.md)。当前资源覆盖与显示行为见 [VISUAL_FIX.md](VISUAL_FIX.md)，加载器接入约定见 [integration/README.md](integration/README.md)。

## 源码与授权

核心源码在本仓库，配套加载器源码在 [sora1looseload](https://github.com/RovingAimer/sora1looseload)。构建步骤和资源输入说明见 [BUILDING.md](BUILDING.md)。安装 ZIP 仅包含安装文件，GitHub 下载区的 `Source code` 为源码归档。

核心与构建代码采用 [MIT](LICENSE)，完整许可通知嵌入 DLL。加载器、依赖、DDS 与游戏资源的来源和许可见 [THIRD_PARTY.md](THIRD_PARTY.md)。
