# 安装、恢复与冲突处理

当前初版是开发候选，尚未实机验证。本文提供手动安装步骤，不是自动安装脚本；项目不会自动向游戏目录写入文件。不要根据本文推断已经生成可发布的 loader 二进制。

## 安装前确认

- 确认目标是《空之轨迹 the 1st》，并核对 `sora_1st.exe` SHA256：`9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac`。不匹配时不要初始化核心。
- 退出游戏。先备份每个将被覆盖的同路径文件，记录此前不存在的文件。不要覆盖或删除整个 `layout`、`asset` 或 `mods` 目录。
- 游戏读取资源与加载核心是两件事：复制 `DialogueOutline.dll` 到 `mods/DialogueOutline/` 不会让原版 stockloader 自动加载它。
- Hinkiii 的 [sora1looseload](https://github.com/Hinkiii/sora1looseload) 原版没有本项目所需的插件接入 API。接入改动以其 [`04e898e369e3019d5aa2cb13a7209de39c643a4a`](https://github.com/Hinkiii/sora1looseload/commit/04e898e369e3019d5aa2cb13a7209de39c643a4a) 为基线。首次让该 loader 支持核心，需要维护者采纳源码改动或用户自行构建兼容版本；这是明确的 loader 升级/构建，不是安装资源时顺带完成的步骤。
- 工作区保留的 `backups/before_v0.2_20261016_132542/` 是 v0.2 之前的本地快照，清单记录当时的原 loader 和图像文件状态。它不表示 Mod 已安装，也不能替代本次安装前对当前文件的备份。工作区未自动安装过本方案。
- 旧 v0.1/v0.2 都是离线候选，不是当前初版。旧文件已归档到 `research/archive/legacy_candidates_20261016/dist/`。不要把旧 v0.2 的代理 DLL 改名或复制成 `SoraLooseLoader.dll` 来安装当前核心。

## 只选择一种核心接入方式

三种方式共用同一 hook；一次游戏进程只允许一种方式初始化核心。A 与 C 是 loader 接入路线，B 是替代的 ASI 路线。A 主包提供核心 DLL 和主资源，既可配合已有兼容 loader，也可配合玩家选择的 C 集成 loader。不要再用 B ASI 同时加载核心；可选即时对话包可以与任一接入路线搭配。

### A. 已支持核心接入的 loader（主方案）

使用已支持以下约定的 loader：从游戏目录相对路径加载 `mods/DialogueOutline/DialogueOutline.dll`，并在 DllMain 和 loader lock 之外同步调用导出函数 `SoraDialogueInitialize()`。如果已有符合约定的 loader，无需替换它。主包 `1st_初版_主接入包.zip` 包含核心 DLL 和主资源，不含 XInput loader。

1. 按[构建说明](BUILDING.md)生成 `DialogueOutline.dll`，并按需运行资源生成工具。
2. 将核心放到游戏目录 `mods/DialogueOutline/DialogueOutline.dll`。
3. 将 `source/build/resources/main/layout/subtitle.lay` 复制到游戏目录 `layout/subtitle.lay`。
4. 将 `source/build/resources/main/asset/dx11/image/window8.dds` 复制到游戏目录 `asset/dx11/image/window8.dds`。
5. 如需即时对话气泡描边，再分别将 `source/build/resources/optional_active_voice/layout/active_voice.lay` 与 `source/build/resources/optional_active_voice/asset/dx11/image/active_voice.dds` 复制到游戏目录对应相对路径。

若使用 Hinkiii 原版 stockloader，上述复制不会加载核心。先由 loader 维护者采纳集成改动，或自行基于指定上游提交构建支持接入的 loader，再执行资源复制步骤。可选方案 C 是单独替换 XInput loader；它仍须与 A 主包共同使用。不要把“主资源包已复制”写成“原版 loader 已兼容”。

### B. UAL ASI（可选）

可选 ASI 包 `1st_初版_可选ASI入口.zip` 包含 `plugins/DialogueOutline.asi`、主 `layout/subtitle.lay` 与 `asset/dx11/image/window8.dds`，不含 `mods/DialogueOutline/DialogueOutline.dll`，不需要安装 A 主包。运行 `python build.py --asi` 可生成 ASI；将其放到兼容 UAL 支持的 `plugins/` 加载位置。当前核心导出 `InitializeASI`，UAL 默认 `DontLoadFromDllMain=1`，符合锁外初始化约定。

可复用已安装的兼容 UAL。若没有，玩家可自行选择安装[官方 UAL](https://github.com/ThirteenAG/Ultimate-ASI-Loader)；UAL 不随本 Mod 提供，这也不是默认安装步骤。先检查游戏目录中同名入口 DLL 和其他 Mod 的冲突。目标 EXE 静态导入 `d3d11.dll` 与 `dxgi.dll`，这些入口的实机行为待验证；不要随意改名、覆盖已有代理，也不要据此假定 UAL 一定可用。若需要即时对话气泡，另外安装可选即时对话资源包。

### C. 集成 XInput loader（可选）

该方式是可选的 XInput loader 替换。主包不含 `xinput1_4.dll`；可选接入版 loader 单独生成并需玩家明确选择。它必须与 A 主包一起安装，A 提供核心 DLL 与主资源，C loader 负责加载核心。`python build.py` 和 `python build.py --asi` 都不会生成这个 loader。安装前备份当前 `xinput1_4.dll`，确认构建报告、上游基线和校验值，再决定是否替换。不要把旧 v0.2 代理或任意 `proxy.dll` 直接改名成 `xinput1_4.dll` 或 `SoraLooseLoader.dll`。

在 `source` 目录运行 `python integration/build_loader.py` 可按锁定的上游源码和补丁构建该可选 loader，输出为 `build/optional_loader/xinput1_4.dll` 与构建报告；可选包名为 `1st_初版_可选接入版加载器.zip`。构建过程不安装或运行 DLL，报告标记未实机验证。替换 loader 属于显式升级，可能影响其他依赖它的 Mod。若无法确认构建报告与核心接入调用，不要安装此方式。A 与 C 可以共同使用；不要再同时使用 B ASI。

## 检查与已知限制

初始化只适用于指定 EXE SHA256 和预期 hook 原始字节。游戏更新、其他可执行版本或同一位置的其他 hook 可能导致核心拒绝初始化。`SoraGetStatus()` 的 `HookActive` 位（`0x008`）才表示此核心成功启用了 hook；冲突位 `0x020` 不证明另一个实例正常。

描边仅响应带标记的对白正文/姓名节点；安装可选资源后再包含即时对话气泡。它在生成文字内容时依据当时文字颜色挑选黑/白描边，后续调色、亮度动画未重新生成文字时不保证重新选择。尚未验证实机画面、性能或其他 Mod 的 hook 兼容性；《空之轨迹 the 2nd》未支持。核心不写日志。

## 建议的实机核验顺序

兼容性核验尚未完成，不保证与所有 Mod 或 loader 组合都不冲突。安装前后使用同一存档、同一设置和同一场景比较：

1. 记录未安装时的画面、存档位置与设置，作为对照基准。
2. 先确认原有发型、饰品资源能加载，键盘/手柄输入正常。
3. 检查对白正文和姓名是否保留原色，并观察黑白描边是否与当前文字颜色形成预期对比。
4. 检查菜单与日志区域，确认没有新增描边。
5. 若安装气泡资源，再检查气泡、淡入淡出和连续切换场景的稳定性。

核心本身不写日志；原版 loader 仍可能输出其自己的 `console.log`。使用集成接入版 loader 时，可查看 `[DialogueOutline] status 0x14F`。其中状态位 `0x008` 只表示 hook 已安装，不证明画面效果正确；最终仍需按上述步骤检查游戏画面。

## 恢复与卸载

1. 退出游戏。
2. 用本次安装前的备份逐个恢复被覆盖的 `layout`、`asset` 和其他文件。对安装前不存在的新增文件，仅在确认属于本 Mod 后逐个删除。
3. 仅删除本次新增的 `mods/DialogueOutline/DialogueOutline.dll`；不要删除整个 `mods` 或其父目录。
4. 若选择 ASI，只删除本次新增的 `DialogueOutline.asi`。若选择集成 XInput loader，恢复安装前备份的 `xinput1_4.dll`。
5. 如果曾安装旧 v0.1 全局字体候选，依据你自己的安装前备份逐个恢复六个 shader：`ui_basic_p.fxo`、`ui_basic_vcolor_p.fxo`、`ui_basic_mono_p.fxo`、`ui_bold_p.fxo`、`ui_depth_alpha_p.fxo`、`ui_bold_depth_alpha_p.fxo`。某文件只在确认 v0.1 安装前不存在后，才可逐个删除。不要用旧候选覆盖当前备份。

旧候选随附的历史说明和交付记录保留在 `research/archive/legacy_candidates_20261016/dist/`，其中的绝对路径可能已陈旧；本文件是当前安装入口。
