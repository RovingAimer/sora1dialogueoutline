# 加载器接入约定

加载器开发已迁移到真正的上游 fork：[RovingAimer/sora1looseload](https://github.com/RovingAimer/sora1looseload)。本仓库只维护独立 Mod 核心，不再附带另一份加载器开发源码或构建脚本。完整来源和当前接入提交记录在 [loader.lock.json](../loader.lock.json)。

当前约定是固定加载游戏程序目录下的 `mods/DialogueOutline/DialogueOutline.dll`，在 DllMain/loader lock 之外显式调用 `DWORD WINAPI SoraDialogueInitialize()`，通过返回状态的 `0x008` 判断本核心是否安装 hook；`0x800` 需作为内存维护警告处理。缺少模块时保留原资源加载行为。

这是针对描边模块的接入点，不是通用插件框架。现用原版加载器没有此接入点：选择该路线需要明确更换为支持它的加载器版本；fork 的源码、编译和发布独立维护。ASI 是另一条可选路线，与该接入点择一使用。
