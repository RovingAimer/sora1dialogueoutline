# 来源与授权

发布 ZIP 只包含安装文件。完整源码、构建步骤和许可证说明保存在两个公开 GitHub 仓库： [sora1dialogueoutline](https://github.com/RovingAimer/sora1dialogueoutline) 与 [sora1looseload](https://github.com/RovingAimer/sora1looseload)。

## 代码

描边核心和构建代码采用 MIT，Copyright (c) 2026 DialogueOutline contributors；完整文本见 [LICENSE](LICENSE)。

加载器 fork 基于 [Hinkiii/sora1looseload](https://github.com/Hinkiii/sora1looseload) 的提交 `04e898e369e3019d5aa2cb13a7209de39c643a4a`，保留上游历史、MIT 协议和 Hinkiii 2025 年版权声明。集成和构建改动也采用 MIT，Copyright (c) 2026 DialogueOutline contributors。

Microsoft Detours 是独立加载器的依赖，固定在提交 `9764cebcb1a75940e68fa83d6730ffaf0f669401`。其 Microsoft MIT 许可证和版权随加载器源码保留。

## DDS 与游戏资源

参考 `window8.dds` 与 `active_voice.dds` 来自用户下载的 [3DM 论坛参考贴](https://bbs.3dmgame.com/forum.php?mod=viewthread&tid=6654303)。对应 SHA256 为：

- `window8.dds`: `c97b26a10149495d5a693e35b90e28344271c000705c05725cf6640f26c2181f`
- `active_voice.dds`: `ffc1721d272e7b71015a419d1ee969cb811e4614018789151cc670994e596cf1`

以上校验值记录发布使用的文件。DDS版权归参考作者，游戏布局版权归原作权利人；相关资源遵循各自许可。MIT适用于本项目代码。

黑白描边选择参考 [W3C sRGB 相对亮度与对比度定义](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html#dfn-relative-luminance)。
