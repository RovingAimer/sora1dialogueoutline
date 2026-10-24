# DialogueOutline

DialogueOutline 为《空之轨迹 the 1st》的主对白添加自动黑白描边，保留正文与姓名原色，移除原偏移阴影，并让对应背景透明。当前正式版本为 v0.2.2；通用加载器继续使用 v0.2.0。

v0.2.2 移除额外 EXE 白名单并改为特征定位，本次按用户授权静态验证后发布。

## 实现思路

字幕布局为目标节点写入专属标记。核心在原生字形副层构建处检查标记，只调整主对白及可选的带头像即时对白横条。该路径限定到目标对白节点，并复用原生 CPU 字形处理流程。

填充 RGB 和 alpha 保持原值；描边颜色按填充 RGB 与节点 RGB 调制后的有效色自动选择纯黑或纯白。主对白另可设置正文字号与行距、姓名字号和区域高度，描边粗细可通过配置热更新，见 [CONFIGURATION.md](CONFIGURATION.md)。

v0.2.1 新增 `OutlineWidth=0–200%`：100% 保持原粗细，50% 减半，0% 关闭描边；保存后在新生成的文字上生效。同时修复特殊 CG 后主对白描边和字号设置被剧情样式覆盖的问题。

## v0.2.2 下载

| 下载 | 用途 |
| --- | --- |
| [DialogueOutline-v0.2.2-main.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.2/DialogueOutline-v0.2.2-main.zip) | 新安装主包：核心 DLL、加载器模块声明、布局配置、主对白 LAY 与 DDS。 |
| [DialogueOutline-v0.2.2-active-voice.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.2/DialogueOutline-v0.2.2-active-voice.zip) | 可选资源包：带头像即时对白横条布局与背景 DDS。 |
| [DialogueOutline-v0.2.2-source.zip](https://github.com/RovingAimer/sora1dialogueoutline/releases/download/v0.2.2/DialogueOutline-v0.2.2-source.zip) | 完整源码、构建脚本、文档和许可证。 |

游戏目录还需要[通用加载器 v0.2.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.2.0)。安装和升级步骤见 [INSTALL.md](INSTALL.md)。

## 支持范围

不再按 EXE 文件名、文件大小或整文件 SHA256 设置白名单。采用与 Hinkiii 原版加载器相同的掩码特征扫描方式定位实际代码；找不到对应特征时跳过相应接入。其他版本是否可用取决于目标代码及对象布局，不要求整份 EXE 与某一版本完全相同。可选资源包覆盖带头像的即时对白横条；人物头顶随角色移动的气泡不属于当前资源补丁范围。

核心代码和构建脚本采用 MIT，见 [LICENSE](LICENSE)。游戏布局与 DDS 的来源和许可独立于代码，见 [THIRD_PARTY.md](THIRD_PARTY.md)。

核心 DLL 在游戏进程内运行代码，可能影响游戏稳定性或存档数据。请核对目标程序版本与下载文件哈希，也可阅读源码并自行构建。完整构建步骤见 [BUILDING.md](BUILDING.md)，核心 DLL SHA256 见 [INSTALL.md](INSTALL.md)。
