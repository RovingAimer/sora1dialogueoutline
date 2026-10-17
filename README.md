# sora1dialogueoutline

《空之轨迹 the 1st》对白透明背景与自适应文字描边 Mod。正文、姓名和可选的带头像即时对白横条保留原文字色，并显示黑色或白色描边。

## 已发布下载

DialogueOutline v0.1.1 配合通用加载器 v0.2.0 使用。此版仅新增模块配置 `sora_mod.ini`；字幕 DLL、布局、贴图及描边效果保持不变。

| 下载 | 用途 |
| --- | --- |
| [配置更新包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.1/DialogueOutline-v0.1.1-config.zip) | 已安装字幕 Mod 的玩家使用：仅补充 `mods/DialogueOutline/sora_mod.ini`。 |
| [主包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.1/DialogueOutline-v0.1.1-main.zip) | 新安装使用：主对白正文、姓名描边及透明背景，包含模块配置。 |
| [即时对白横条包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.1/DialogueOutline-v0.1.1-active-voice.zip) | 可选：带头像的即时对白横条背景透明和文字描边；与主包一起使用。 |
| [完整源码包](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.1.1/DialogueOutline-v0.1.1-source.zip) | 源码、模块配置、构建方法和许可证，供查验或自行编译。 |
| [配套加载器 v0.2.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.2.0) | 提供 `xinput1_4.dll`，统一加载带配置的模块。 |

主包与配置更新包均提供字幕自己的模块配置。已存在相同配置时无需重复安装。配置内容和接入方法见 [integration/README.md](integration/README.md)，兼容来源及实机验收记录见 [loader.lock.json](loader.lock.json)。

## 安装与恢复

退出游戏并备份将被覆盖的文件。安装通用加载器 v0.2.0；再将主包解压合并到 `sora_1st.exe` 所在目录。需要带头像即时对白横条时再合并可选包，并保留 ZIP 内的目录结构。

已安装字幕 Mod 的玩家升级时，只需将配置更新包合并到游戏目录。通过加载器 v0.2.0 安装包补齐过同一配置的玩家可继续使用现有文件。

仅支持 [INSTALL.md](INSTALL.md) 列出的游戏 EXE 版本。资源补丁覆盖正文、姓名和可选横条；角色头顶随人物移动的气泡不属于当前资源范围。

卸载时退出游戏，按安装清单逐文件恢复备份；安装前不存在的文件可移除。保留其他 Mod 文件。

DLL 会在游戏进程内以当前玩家账户权限运行代码，可能影响游戏稳定性、文件和数据。只使用可信来源。完整源码、构建步骤和许可证见 [BUILDING.md](BUILDING.md)、[LICENSE](LICENSE) 与 [THIRD_PARTY.md](THIRD_PARTY.md)；可自行编译并将附件 SHA256 与发布清单核对。

## 当前实现

布局补丁为正文、姓名和可选即时对白横条的文字节点写入专属标记；DDS 提供对应透明背景。核心在字形副层构建时识别标记，将偏移阴影项转为原生描边几何，并在字形项创建时按有效文字色选择黑色或白色。核心保留填充文字 RGB 与 alpha，默认线宽系数为 `0.60`。算法、兼容入口、构建与资源来源见 [BUILDING.md](BUILDING.md)、[integration/README.md](integration/README.md) 和 [VISUAL_FIX.md](VISUAL_FIX.md)。
