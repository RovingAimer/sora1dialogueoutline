# sora1dialogueoutline

《空之轨迹 the 1st》透明对白背景与文字描边 Mod。它保留对白原色，并把原有阴影副层改为贴近字形的黑白描边。主对白和可选即时对话气泡使用专属布局标记；菜单和日志没有这些标记。

## 方案与当前效果

资源补丁为对白节点写入精确标记值 `0.28125f`（位模式 `0x3E900000`），并提供透明背景 DDS。原生 CPU hook 在构建字形副层时检查标记，将对白阴影副层转为原生描边几何。这个位置能跟随实际字形与颜色工作，也减少对其他 Mod 资源的改动。

描边颜色在每个字形项创建时，根据填充文字 RGB 与节点 RGB 的调制结果自动选择纯黑或纯白，保留填充 RGB 与 alpha。之后颜色变化若没有重建字形项，不会重新选色。

用户已确认主对白和即时气泡正常，偏移阴影消失。当前两种标记描边模式都将最终线宽乘以 `0.60`；该厚度是当前视觉调整，尚未单独进行长期验证。标记值保持不变，用来识别对白节点。

核心只接受 `sora_1st.exe`，SHA256：

```text
9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac
```

## 安装包

主包合并到游戏目录后包含：

```text
mods/DialogueOutline/DialogueOutline.dll
layout/subtitle.lay
asset/dx11/image/window8.dds
```

即时对话气泡是可选包，提供 `layout/active_voice.lay` 和 `asset/dx11/image/active_voice.dds`。核心由兼容加载器从固定路径加载。配套 `xinput1_4.dll` 加载器单独提供，安装、卸载步骤见 [INSTALL.md](INSTALL.md)。

发布 ZIP 只放安装所需文件。完整源码、构建步骤和授权说明见公开仓库：[sora1dialogueoutline](https://github.com/RovingAimer/sora1dialogueoutline) 与独立加载器 fork [sora1looseload](https://github.com/RovingAimer/sora1looseload)。

## 开发与授权

从 Windows x64 源码构建见 [BUILDING.md](BUILDING.md)。当前视觉状态和线宽见 [VISUAL_FIX.md](VISUAL_FIX.md)。核心、加载器、Detours、DDS 与游戏资源的来源和许可各自列于 [THIRD_PARTY.md](THIRD_PARTY.md)。

核心与构建代码采用 MIT，见 [LICENSE](LICENSE)。DLL包含完整MIT许可通知。DDS与游戏资源的来源和授权见[THIRD_PARTY.md](THIRD_PARTY.md)。
