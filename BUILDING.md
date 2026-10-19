# 构建与核心实现

## 版本与构建环境

DialogueOutline v0.2.1 使用通用加载器 v0.2.0。核心源码和构建脚本采用 MIT；布局资源与 DDS 使用各自的来源和许可，见 [LICENSE](LICENSE) 与 [THIRD_PARTY.md](THIRD_PARTY.md)。

构建需要 Windows x64、Python 3.10+、Visual Studio 2022 C++ Build Tools（MSVC x64 与 MASM）及 Windows SDK。在源码根目录运行：

    python build.py

脚本编译 `src/core.cpp`、`src/bridge.asm`、`src/runtime_config.cpp` 与 `src/dialogue_layout.cpp`，输出 `build/DialogueOutline.dll`、`build/core_report.json`、编译日志和桥接反汇编。报告记录 DLL 与编译输入 SHA256、源码提交、编译器、导出符号及直接 DLL 导入。构建使用 C++17、静态 CRT `/MT`、`/O2` 与 `/W4`，直接依赖 `KERNEL32.dll` 和 `bcrypt.dll`。

当前 v0.2.0 核心 DLL SHA256：

    460b551adbd97452ece8c79bd8e52daa11500c69d77e9a389e8161e262138081

## 构建资源补丁

将授权的原始资源副本放在以下输入路径：

    inputs/layout/subtitle.lay
    inputs/layout/active_voice.lay
    inputs/reference/window8.dds
    inputs/reference/active_voice.dds

运行：

    python tools/build_resources.py

脚本按 `resource_patches.json` 校验输入 SHA256 和改动项，并输出到 `build/resources/`。主对白资源位于 `build/resources/main/`；可选带头像即时对白横条资源位于 `build/resources/optional_active_voice/`。

提交源码并完成核心与资源构建后，运行 `python tools/package_release.py` 生成主包、可选横条包与完整源码包。安装ZIP只含安装文件；`build/packages/release_manifest.json`与`native-build-report.json`在包外保存校验和构建记录。`--source-only`仅生成完整源码附件。

## 目标程序与描边 hook

核心验证 `sora_1st.exe` 的 SHA256 `9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac` 和 hook 位置的原始指令字节。主字形 hook 位于 RVA `0x46BDF9`。ABI 输入为 fill RDI、node RSI、outline RBX；原生字形尺度由 XMM8 经桥接作为第四参数 XMM3。

LAY 在目标对白节点的 `0x368` 写入专属标记 `0.28125f`（位模式 `0x3E900000`）。核心仅处理该标记下的 fill 与 outline 项：

- 原 outline mode `0x0001` 时，将 fill 的 `0x08..0x47` 字节复制到 outline，切换到 `0x0100`，并用节点标记除以原生字形尺度生成线宽。
- 原生 mode `0x0100` 保留几何；两种 mode 的线宽都在字形项构建时乘以 `0.60 × OutlineWidth / 100`。`OutlineWidth` 缺省为 100；为 0 时仅把描边副层的 alpha（item `+0xA4`）设为 0，正文填充不变。
- fill RGB 和 alpha 保持原值。`0x3E900000` 用作节点身份标记，线宽由独立 multiplier 控制。

有效填充色逐通道计算为 `itemRGB * nodeRGB` 并夹到 `0..1`，再转为线性 sRGB：`v <= 0.04045` 时除以 `12.92`，否则计算 `((v + 0.055) / 1.055)^2.4`。相对亮度为 `Y = 0.2126R + 0.7152G + 0.0722B`。黑色对比度为 `(Y + 0.05) / 0.05`，白色为 `1.05 / (Y + 0.05)`；选对比度较大者，相同时选黑。颜色在字形项创建时选择，字形项重建时重新选择。

自动选色位于 `SoraOutlineCallback` 的 `selected` 声明：

    const float selected = static_cast<float>(SoraChooseOutline(item[0] * modulation[0],
        item[1] * modulation[1], item[2] * modulation[2]));
    const float rgb[] = {selected, selected, selected};

固定黑色或白色时，将 `selected` 声明改为 `const float selected = 0.0f;` 或 `const float selected = 1.0f;`。自定义颜色时删除该声明，并把数组改为 0 到 1 的 RGB 分量，例如：

    const float rgb[] = {0.20f, 0.60f, 1.00f};

修改后运行 `python build.py` 重新构建核心 DLL。颜色设置不通过 INI 完成。

## 正文布局、姓名与热更新

`src/dialogue_layout.cpp` 通过专属 marker、原生 id `0xF0000006`、节点名 `text` 及同父节点结构识别主对白正文。RVA `0x46B1E0` 的原生 Prepare 前读取一份配置快照并应用三个布局设置。描边 hook 在每个新字形副层构建时读取同一个 64 位快照中的 `OutlineWidth`。

准备桥同时传入原始调用地址。主对白控制器在 `0x457F9F` 调用准备函数前重设原生字号，因此该路径每次直接采集新基值；字形重建等其他路径沿用已保存的未缩放基值。这样可随游戏字号档位变化重算，同时避免重复缩放。

`TextSize` 按正文原始字号 31、33 或 35 缩放正文与行距，并四舍五入。`NameSize` 分别按主姓名与前一姓名各自的原生 27 字号计算 `round(27 × NameSize / 100)`，结果至少为 1。字号改变时标记姓名节点 dirty，并在原 Prepare 前调用 RVA `0x46E760` 的原生测量函数，随后原生 Prepare 按新尺寸建立文字布局。

`BoxHeight` 按比例缩放对白区域高度并保持矩形底边；正文、姓名和前一姓名随高度差移动，`belt` 区域同步缩放和位移，`okuri` 图标保留原底边位置。RVA `0x458539` 在控制器坐标更新后协调位移，动画起点、当前位置和目标位置同步平移，动画计时与曲线保留。RVA `0x46ACE0` 处理控制器后续赋予的行距；RVA `0x469D80` 在正文销毁时清理基准缓存。脚本换行和分页规则由游戏继续处理。可选的带头像即时对白横条不受这三项设置影响。

`src/runtime_config.cpp` 将 `DialogueOutline.ini` 定位在核心 DLL 同目录，通过 `ReadDirectoryChangesW` 与 `CreateThreadpoolIo` 接收文件通知。文件变化后等待 250 ms 合并连续写入；读取失败后每 100 ms 重试，最多 3 次。合法配置以 64 位原子快照发布，三个布局参数在下一次正常主对白 Prepare 读取新值；描边粗细在新字形项构建时读取，不主动重建当前已缓存文字。

更多配置格式与生效时机见 [CONFIGURATION.md](CONFIGURATION.md)。加载器接口见 [integration/README.md](integration/README.md)。
