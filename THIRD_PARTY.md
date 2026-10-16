# 第三方来源与授权边界

## DialogueOutline 源码

`source/LICENSE` 为 DialogueOutline 源码采用的 MIT 许可证。该许可证不改变游戏文件、原 loader、PAC 内容或第三方贴图的权利归属，也不授予分发这些材料的权限。

## sora1looseload

上游项目：[Hinkiii/sora1looseload](https://github.com/Hinkiii/sora1looseload)，MIT 许可证，保留 Hinkiii 2025 版权声明。loader 接入改动以提交 [`04e898e369e3019d5aa2cb13a7209de39c643a4a`](https://github.com/Hinkiii/sora1looseload/commit/04e898e369e3019d5aa2cb13a7209de39c643a4a) 为基线。上游提供 loose-file loader；该版本没有本项目需要的插件 API。集成源码需要由维护者采纳或由用户自行构建，源码补丁不等于一个已构建、可直接替换的 loader DLL。

若分发包含从该项目复制或改编的代码，须随代码保留上游版权声明和 MIT 许可证文本。

## Microsoft Detours

集成 loader 使用的 Detours 源码已收录于 `source/third_party/sora1looseload/external/detours/`，固定在 [Microsoft Detours commit `9764cebcb1a75940e68fa83d6730ffaf0f669401`](https://github.com/microsoft/Detours/tree/9764cebcb1a75940e68fa83d6730ffaf0f669401)，采用 MIT 许可证。该目录保留了 Microsoft 版权与许可证文件；其许可证只涵盖 Detours 代码，不覆盖游戏资源。

## 参考 DDS 与游戏资源

工作副本中的两张 DDS 来自用户下载的 [3DM 论坛参考贴](https://bbs.3dmgame.com/forum.php?mod=viewthread&tid=6654303)：

| 文件 | SHA256 |
| --- | --- |
| `window8.dds` | `c97b26a10149495d5a693e35b90e28344271c000705c05725cf6640f26c2181f` |
| `active_voice.dds` | `ffc1721d272e7b71015a419d1ee969cb811e4614018789151cc670994e596cf1` |

列出来源和校验值不代表获得再分发许可，也不表示贴图采用 MIT。面向外部的源码分发不包含 `source/inputs/reference/` 下的 DDS，不得宣称可任意再分发。

布局输入来自用户本地游戏资源副本：`subtitle.lay` 与 `active_voice.lay`。构建资源工具按 `resource_patches.json` 中列出的偏移与源文件 SHA256 校验后生成派生布局；输入和派生游戏资源的权利与代码许可证分开处理。源码分发不包含这些输入，也不包含游戏 EXE、PAC 或原 loader DLL。不得从代码 MIT 许可证推导这些文件可以公开分发。

## 对比度计算参考

黑白描边的相对亮度与对比度计算参考 [W3C WCAG 2.2 对比度定义](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html#dfn-relative-luminance)。这里只引用公开计算定义；它不改变游戏资源授权，也不构成对所有后续画面调色效果均能自适应的保证。