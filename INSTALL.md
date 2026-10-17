# v0.1.1 安装与卸载

## 支持版本

本发布支持《空之轨迹 the 1st》的 `sora_1st.exe`，SHA256 为：

```text
9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac
```

在游戏程序目录可用 PowerShell 核对：

```powershell
Get-FileHash -LiteralPath .\sora_1st.exe -Algorithm SHA256
```

核心以文件名、SHA256 和原始指令字节验证支持版本。

## 下载与文件清单

所有路径均相对于 `sora_1st.exe` 所在目录。

| 安装包 | 文件 | 用途 |
| --- | --- | --- |
| [通用加载器 v0.2.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.2.0) | `xinput1_4.dll`、`mods/DialogueOutline/sora_mod.ini` | 加载 loose 资源与声明的独立模块，包含字幕模块声明。 |
| [配置更新包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.1/DialogueOutline-v0.1.1-config.zip) | `mods/DialogueOutline/sora_mod.ini` | 已安装字幕 Mod 时补充通用加载器配置。 |
| [DialogueOutline v0.1.1 主包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.1/DialogueOutline-v0.1.1-main.zip) | `mods/DialogueOutline/DialogueOutline.dll`、`mods/DialogueOutline/sora_mod.ini`、`layout/subtitle.lay`、`asset/dx11/image/window8.dds` | 新安装使用：主对白正文、姓名描边及透明背景，包含模块配置。 |
| [DialogueOutline v0.1.1 即时对白横条包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.1/DialogueOutline-v0.1.1-active-voice.zip) | `layout/active_voice.lay`、`asset/dx11/image/active_voice.dds` | 可选：带头像即时对白横条背景透明和文字描边；与主包一起使用。 |

人物头顶随角色移动的气泡不在本次资源补丁范围。

## 已安装玩家更新

v0.1.1 仅新增 `mods/DialogueOutline/sora_mod.ini`，DLL、布局和贴图保持原字节。使用通用加载器 v0.2.0 时，退出游戏，备份已有同名配置，再将配置更新包合并到游戏目录。

已安装加载器包中的同一字幕配置时，无需重复安装。当前配置如下：

```ini
[SoraMod]
Api=1
DLL=DialogueOutline.dll
Initialize=SoraDialogueInitialize
```

## 安装

1. 退出游戏并核对支持版本。
2. 备份清单中将被覆盖的文件，并记录安装前不存在的路径，供卸载时恢复。
3. 安装通用加载器 v0.2.0。
4. 将主包解压合并到游戏程序目录，保留 `mods`、`layout` 和 `asset` 层级。
5. 需要即时对白横条效果时，再合并可选包。

```text
游戏目录/
├─ sora_1st.exe
├─ xinput1_4.dll
├─ mods/DialogueOutline/
│  ├─ DialogueOutline.dll
│  └─ sora_mod.ini
├─ layout/
│  ├─ subtitle.lay
│  └─ active_voice.lay        ← 可选包
└─ asset/dx11/image/
   ├─ window8.dds
   └─ active_voice.dds        ← 可选包
```

安装后应存在清单中的文件路径。游戏启动后，正文和姓名保留原色并显示描边，原偏移阴影移除，对应背景透明。通用加载器按模块名记录 `SoraDialogueInitialize` 的原始 32 位返回值，正常值为 `0x0000014F`。

## 卸载与恢复

退出游戏，按清单逐文件恢复备份；安装前不存在的文件逐个移除。若安装过即时对白横条包，也一并恢复或移除其文件。保留其他 Mod。

源码、构建、颜色与粗细说明见 [BUILDING.md](BUILDING.md)，来源与许可见 [THIRD_PARTY.md](THIRD_PARTY.md)。
