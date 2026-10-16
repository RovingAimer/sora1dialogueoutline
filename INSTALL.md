# v0.1.0 安装与卸载

## 支持版本

本发布支持《空之轨迹 the 1st》的 `sora_1st.exe`，SHA256 为：

```text
9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac
```

可在游戏程序目录打开 PowerShell 核对：

```powershell
Get-FileHash -LiteralPath .\sora_1st.exe -Algorithm SHA256
```

核心通过 EXE 文件名、SHA256 和原始指令字节验证支持版本。

## 下载与文件清单

所有路径均相对于 `sora_1st.exe` 所在目录。

| 安装包 | 文件 | 用途 |
| --- | --- | --- |
| [配套加载器 v0.1.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.1.0) | `xinput1_4.dll` | 加载对白核心及 loose 资源 Mod。 |
| [主包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.0/DialogueOutline-v0.1.0-main.zip) | `mods/DialogueOutline/DialogueOutline.dll`、`layout/subtitle.lay`、`asset/dx11/image/window8.dds` | 必装：主对白正文、姓名描边及透明背景。 |
| [即时对白横条包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.0/DialogueOutline-v0.1.0-active-voice.zip) | `layout/active_voice.lay`、`asset/dx11/image/active_voice.dds` | 可选：带头像的即时对白横条背景透明、文字描边；需同时安装主包。 |

人物头顶跟随角色移动的气泡暂未纳入此发布。

## 安装

1. 退出游戏，核对支持版本。
2. 备份清单中将被覆盖的已有文件，并记录安装前不存在的路径，供卸载时恢复。
3. 将加载器 ZIP 中的 `xinput1_4.dll` 放到游戏程序目录。
4. 将主包解压合并到同一目录，保留 `mods`、`layout`、`asset` 的目录结构。
5. 需要即时对白横条效果时，再合并可选包。

安装后应直接存在 `mods/DialogueOutline/DialogueOutline.dll` 等清单路径。启动游戏后，对白保留原色并显示四周描边，原偏移阴影移除，对应背景透明。加载状态可在 `console.log` 中查看：`[DialogueOutline] Initialization status: 0x0000014F (hook active)`。

## 卸载与恢复

退出游戏，按上述清单逐文件恢复安装前的备份；安装前不存在的文件则逐个删除。可选包仅在实际安装过时处理。保留其他 Mod 的文件和目录。

源码、构建、粗细与颜色调整见 [BUILDING.md](BUILDING.md)，来源与授权见 [THIRD_PARTY.md](THIRD_PARTY.md)。
