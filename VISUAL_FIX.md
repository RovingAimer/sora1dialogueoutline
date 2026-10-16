# 当前显示范围

| 界面 | 布局与背景资源 | 安装包 |
| --- | --- | --- |
| 主对白正文与姓名 | `subtitle.lay`、`window8.dds` | 主包 |
| 带头像的即时对白横条 | `active_voice.lay`、`active_voice.dds` | 可选 active-voice 包 |

`active_voice.lay` 的横条背景引用 `active_voice` 贴图，头像使用独立贴图，翻页图标使用 `window3`。可选包修改横条背景与文字节点，保留头像和翻页图标。人物头顶跟随角色移动的气泡暂未提供资源补丁。

带标记的文字保留原位置、RGB 和 alpha，偏移阴影副层转换为原生描边几何。描边在每个字形项创建时按填充 RGB 与节点 RGB 的乘积选择对比度较高的纯黑或纯白；默认线宽系数为 `0.60`。完整公式和粗细、颜色调整见 [BUILDING.md](BUILDING.md)。

支持的 EXE 版本与安装文件清单见 [INSTALL.md](INSTALL.md)。
