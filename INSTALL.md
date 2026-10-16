# 安装与卸载

本 Mod 支持《空之轨迹 the 1st》指定版本。安装前核对游戏目录中的 `sora_1st.exe` SHA256：

```text
9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac
```

完整核心源码和安装包见 [sora1dialogueoutline](https://github.com/RovingAimer/sora1dialogueoutline)；配套加载器及其源码见 [sora1looseload](https://github.com/RovingAimer/sora1looseload)。发布 ZIP 只包含需要安装的文件，构建和授权说明在仓库中。

## 安装

1. 退出游戏，备份游戏根目录中现有的 `xinput1_4.dll`，以及将被覆盖的 Mod 资源文件。
2. 从加载器 fork 获取兼容版 `xinput1_4.dll`，放到游戏根目录。该加载器会在锁外加载 `mods/DialogueOutline/DialogueOutline.dll` 并初始化核心。
3. 将主包合并到游戏目录，保留 ZIP 中的相对路径：
   - `mods/DialogueOutline/DialogueOutline.dll`
   - `layout/subtitle.lay`
   - `asset/dx11/image/window8.dds`
4. 需要即时对话气泡描边时，再合并可选包中的 `layout/active_voice.lay` 与 `asset/dx11/image/active_voice.dds`。

检查加载器日志中的 `[DialogueOutline] status 0x14F`，确认核心已安装 hook。描边针对带标记的主对白正文、姓名和可选即时气泡。用户已确认这些对白正常显示且偏移阴影消失。

## 卸载

1. 退出游戏。
2. 删除本 Mod 新增的 `mods/DialogueOutline/DialogueOutline.dll`。
3. 对主包覆盖过的 `layout/subtitle.lay` 与 `asset/dx11/image/window8.dds`，从安装前备份逐个恢复；若安装前不存在，再逐个删除。
4. 若安装了即时对话包，同样恢复或删除它的两个文件：`layout/active_voice.lay` 与 `asset/dx11/image/active_voice.dds`。
5. 从备份恢复原来的 `xinput1_4.dll`。

只处理以上文件及本次实际安装的文件。
