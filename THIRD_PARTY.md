# 来源与授权边界

DialogueOutline 核心及构建代码采用 [MIT](LICENSE)，Copyright (c) 2026 DialogueOutline contributors，协议与原加载器相同。

## 独立加载器 fork

[Hinkiii/sora1looseload](https://github.com/Hinkiii/sora1looseload) 为 MIT，Copyright (c) 2025 Hinkiii。加载器扩展独立维护于真正的 [RovingAimer/sora1looseload fork](https://github.com/RovingAimer/sora1looseload)，以 `04e898e369e3019d5aa2cb13a7209de39c643a4a` 为基线，保留历史与原协议。当前接入提交在 [loader.lock.json](loader.lock.json)。

本 Mod 仓库只维护核心，不再复制一套加载器源码。加载器所用的 Microsoft Detours 子模块固定在 `9764cebcb1a75940e68fa83d6730ffaf0f669401`，MIT，Microsoft 版权和许可证由 fork 的源码与加载器包保留。独立核心不使用 Detours。

## 可选 ASI

可选入口配合 [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) 在 DllMain 外调用 InitializeASI。本仓库不重新分发 UAL，也不默认安装额外 D3D/DXGI 入口。

## 参考 DDS 与游戏资源

两张参考 DDS 来自用户下载的 [3DM 论坛参考贴](https://bbs.3dmgame.com/forum.php?mod=viewthread&tid=6654303)。`window8.dds` SHA256 为 `c97b26a10149495d5a693e35b90e28344271c000705c05725cf6640f26c2181f`；`active_voice.dds` 为 `ffc1721d272e7b71015a419d1ee969cb811e4614018789151cc670994e596cf1`。

来源和校验值不代表再分发许可，也不表示贴图采用 MIT。公开源码不含参考 DDS、布局输入、派生游戏资源、EXE 或 PAC；本地候选包用于用户测试，资源权利与代码许可证分开处理。布局输入由用户从自己的游戏副本准备，资源补丁见 resource_patches.json。

黑白选色计算参考 [W3C sRGB 相对亮度与对比度定义](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html#dfn-relative-luminance)。
