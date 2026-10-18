# 安装与卸载

## 正式版本与支持程序

当前 DialogueOutline 正式版本为 v0.2.0，配合通用加载器 v0.2.0 使用。核心支持文件名为 `sora_1st.exe` 且 SHA256 完全匹配的程序：

    9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac

在游戏程序目录核对：

    Get-FileHash -Algorithm SHA256 sora_1st.exe

v0.2.0 核心 DLL SHA256：

    30f3f7ab9ee6cca278c8b72d45bf283942b84026b4d5b8c39748d24887dc2d80

## 下载文件

| 下载 | 文件与用途 |
| --- | --- |
| [DialogueOutline-v0.2.0-main.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.0/DialogueOutline-v0.2.0-main.zip) | 主包，含 `mods/DialogueOutline/DialogueOutline.dll`、`mods/DialogueOutline/sora_mod.ini`、`mods/DialogueOutline/DialogueOutline.ini`、`layout/subtitle.lay`、`asset/dx11/image/window8.dds`。 |
| [DialogueOutline-v0.2.0-active-voice.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.0/DialogueOutline-v0.2.0-active-voice.zip) | 可选包，含 `layout/active_voice.lay` 与 `asset/dx11/image/active_voice.dds`，提供带头像的即时对白横条。 |
| [DialogueOutline-v0.2.0-source.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.0/DialogueOutline-v0.2.0-source.zip) | 完整源码、构建步骤、文档与许可证。 |

另需通用加载器 [v0.2.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.2.0)，安装文件为游戏目录下的 `xinput1_4.dll`。

## 全新安装

1. 退出游戏，核对 `sora_1st.exe` 的 SHA256，并备份将被覆盖的文件。
2. 安装通用加载器 v0.2.0。
3. 将主包解压并合并到 `sora_1st.exe` 所在目录，保留 `mods`、`layout` 和 `asset` 的目录结构。
4. 需要带头像的即时对白横条时，再合并可选包。

    游戏目录/
    ├─ sora_1st.exe
    ├─ xinput1_4.dll
    ├─ mods/DialogueOutline/
    │  ├─ DialogueOutline.dll
    │  ├─ sora_mod.ini
    │  └─ DialogueOutline.ini
    ├─ layout/
    │  ├─ subtitle.lay
    │  └─ active_voice.lay        ← 可选包
    └─ asset/dx11/image/
       ├─ window8.dds
       └─ active_voice.dds        ← 可选包

主包中的 `sora_mod.ini` 声明模块入口：

    [SoraMod]
    Api=1
    DLL=DialogueOutline.dll
    Initialize=SoraDialogueInitialize

## 从 v0.1.1 升级

退出游戏并备份当前文件后，将 v0.2.0 主包合并到游戏目录。覆盖 `DialogueOutline.dll`；若已有 `DialogueOutline.ini`，选择跳过并保留原文件，也可在解压后恢复备份。v0.1.1 安装没有 `DialogueOutline.ini` 时，复制主包中的新 INI，初始值为 `TextSize=75%`、`NameSize=75%`、`BoxHeight=80%`。保留个人配置即可继续使用自己的字号和高度。通用加载器仍为 v0.2.0。

## 卸载与恢复

退出游戏后按备份逐文件恢复；安装前不存在的文件逐个移除。若安装过 active-voice 可选包，也恢复或移除其中的两个文件。保留其他 Mod 文件。
