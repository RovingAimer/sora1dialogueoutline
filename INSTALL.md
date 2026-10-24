# 安装与卸载

## 正式版本与支持程序

当前 DialogueOutline 版本为 v0.2.2，配合通用加载器 v0.2.0。不再按 EXE 文件名、文件大小或整文件 SHA256 设置白名单。采用与 Hinkiii 原版加载器相同的掩码特征扫描方式定位实际代码；找不到对应特征时跳过相应接入。其他版本是否可用取决于目标代码及对象布局，不要求整份 EXE 与某一版本完全相同。

本次兼容性调整仅做静态验证；原字幕配置和 CG 修复功能保持。

## 下载文件

| 下载 | 文件与用途 |
| --- | --- |
| [DialogueOutline-v0.2.2-main.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.2/DialogueOutline-v0.2.2-main.zip) | 主包，含 `mods/DialogueOutline/DialogueOutline.dll`、`mods/DialogueOutline/sora_mod.ini`、`mods/DialogueOutline/DialogueOutline.ini`、`layout/subtitle.lay`、`asset/dx11/image/window8.dds`。 |
| [DialogueOutline-v0.2.2-active-voice.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.2/DialogueOutline-v0.2.2-active-voice.zip) | 可选包，含 `layout/active_voice.lay` 与 `asset/dx11/image/active_voice.dds`，提供带头像的即时对白横条。 |
| [DialogueOutline-v0.2.2-source.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.2/DialogueOutline-v0.2.2-source.zip) | 完整源码、构建步骤、文档与许可证。 |

另需通用加载器 [v0.2.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.2.0)，安装文件为游戏目录下的 `xinput1_4.dll`。

## 全新安装

1. 退出游戏，备份将被覆盖的模块与资源文件；已有个人 INI 时保留自己的配置。
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

## 从 v0.1.1 / v0.2.0 升级

退出游戏并备份当前 DLL 与 INI。已经完整安装 v0.2.0 时，从主包中只取 `mods/DialogueOutline/DialogueOutline.dll` 覆盖同路径文件即可；保留个人 `DialogueOutline.ini`，加载器、模块声明、LAY 和 DDS 无需重新安装，原有可选横条包可继续使用。替换 DLL 后重新启动游戏；INI 的热更新不表示 DLL 可以在运行中替换。

从 v0.1.1 升级时，安装主包；若已有 `DialogueOutline.ini`，跳过并保留原文件。没有 INI 时复制主包中的样例，初始值为 `TextSize=75%`、`NameSize=75%`、`BoxHeight=80%`、`OutlineWidth=100%`。

在原有 `[MainDialogue]` 节中新增 `OutlineWidth=100%` 可调整描边；省略时保持 v0.2.0 默认粗细，范围为 0–200%，0% 关闭描边。保存后在新字形生成时生效。通用加载器仍为 v0.2.0。

v0.2.1 同时修复特殊 CG 样式切换后主对白描边和正文字号失效的问题。若该 CG 使用同一主对白正文节点，也会采用配置中的描边粗细；原文字颜色和透明度保留。

## 卸载与恢复

退出游戏后按备份逐文件恢复；安装前不存在的文件逐个移除。若安装过 active-voice 可选包，也恢复或移除其中的两个文件。保留其他 Mod 文件。
