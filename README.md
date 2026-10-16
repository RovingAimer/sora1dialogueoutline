# sora1dialogueoutline

《空之轨迹 the 1st》对话透明背景与黑白自适应描边工具。当前为初版开发候选，尚未实机验证。

本仓库维护独立 Mod 核心。加载器扩展在真正的上游 fork [RovingAimer/sora1looseload](https://github.com/RovingAimer/sora1looseload) 中维护，保留 Hinkiii 的原始历史和 MIT 协议。两者独立构建、独立发布；本仓库不再附带加载器开发源码或加载器构建脚本。来源与接入提交见 [loader.lock.json](loader.lock.json)。

## 效果与范围

仅处理明确标记的剧情对白正文和角色名，可选即时对话气泡。保留文字原色，在生成每个字的绘制内容时，依据文字 RGB 与节点 RGB 调制后的有效颜色选择对比度较高的黑/白描边。菜单、日志和其他未标记文字不参与。

颜色按 sRGB 解释，不读取屏幕背景；之后的调色动画若不重新生成文字，不保证重新选色。当前仅接受 `sora_1st.exe` SHA256 `9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac`。2nd、其他版本、实机画面与性能均未验证。

## 加载器与安装选择

主包按传统 loose 资源 Mod 的目录合并：

```text
游戏目录/
├─ mods/DialogueOutline/DialogueOutline.dll
├─ layout/subtitle.lay
└─ asset/dx11/image/window8.dds
```

独立核心不含 XInput/D3D 接口，不加载原资源加载器，不写日志；DllMain 不初始化。接入方须在 DllMain/loader lock 之外显式调用 `SoraDialogueInitialize()`。这是当前的描边模块接入约定，不是通用插件框架。

- **已有支持该接入点的加载器**：直接合并主包目录；即时对话气泡独立可选。
- **选择加载器 fork**：先备份原 `xinput1_4.dll`，明确选择换成 [加载器 fork](https://github.com/RovingAimer/sora1looseload) 的构建，再合并主包。原版加载器没有接入点，复制 Mod 文件不会使旧 DLL 自动获得能力。加载器源码、编译和下载由 fork 独立提供。
- **选择 ASI**：使用可选 ASI 包和兼容的 Ultimate ASI Loader。它应在锁外调用 `InitializeASI`，当前 UAL 的 `DontLoadFromDllMain=1` 符合此约定。可复用已有 UAL，也可自行权衡额外入口的安装与同名 DLL 冲突。不要同时使用加载器接入和 ASI。

详细备份、安装、恢复及实机检查见 [INSTALL.md](INSTALL.md)。主包不会默认替换任何现有 DLL，ASI 和加载器升级都需要玩家明确选择。

## 构建与交付

Windows x64、Python 3.10+、Visual Studio C++ Build Tools 与 Windows SDK：

```powershell
python build.py
python build.py --asi
python tools/build_resources.py
python tools/package_release.py
```

核心构建不读取或运行游戏 EXE、PAC、原加载器或游戏资源。`--asi` 仅额外生成同字节的 ASI 文件。资源生成工具从工作副本 `inputs/layout/{subtitle,active_voice}.lay` 以及 `inputs/reference/{window8,active_voice}.dds` 读取用户准备的输入，核对完整 SHA256 后应用固定补丁，输出到 `build/resources/`。工具不会从游戏目录自动提取，也不会安装。

打包输出：`DialogueOutline_初版_MIT源码.zip`、`1st_初版_主接入包.zip`、`1st_初版_可选即时对话.zip`；存在 ASI 构建时再输出 `1st_初版_可选ASI入口.zip`。加载器候选及完整源码包由 loader fork 单独发布，本脚本不再生成加载器包。

已附公开构建工作流，构建结果与源码 commit 关联；实际运行状态以 GitHub Actions 为准。本地离线构建与选色检查不代表实机兼容性验证。保留 `.gitattributes`，防止 Windows Git 自动换行改变源码字节。

## 许可证

本项目代码采用与原加载器相同的 [MIT 协议](LICENSE)。原加载器、Detours、参考贴图及游戏资源的来源和授权边界见 [THIRD_PARTY.md](THIRD_PARTY.md)。公开源码不包含 DLL、游戏 EXE、PAC、布局输入或参考 DDS；本地 `inputs/` 和 `build/` 不进入仓库。
