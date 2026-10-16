# 加载器接入约定

配套加载器在 [RovingAimer/sora1looseload](https://github.com/RovingAimer/sora1looseload) 维护，当前安装发布为 [v0.1.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.1.0)。源版本和模块路径记录于 [loader.lock.json](../loader.lock.json)。

加载器在后台资源初始化的 Detours 事务提交成功后，加载游戏程序目录下的 `mods/DialogueOutline/DialogueOutline.dll`，并在 loader lock 外显式调用 `DWORD WINAPI SoraDialogueInitialize()`。核心缺失时继续原有 loose 资源加载流程。

初始化返回值的 `0x008` 表示此核心已安装 hook，`0x020` 表示目标地址冲突，`0x800` 表示内存最终处理警告。核心用 InitOnce 缓存初始化结果。加载器将返回状态写入 `console.log`；正常初始化值为 `0x14F`。

核心也提供 `InitializeASI` 导出供支持该锁外入口的 ASI 加载器调用；构建方式为 `python build.py --asi`。当前发布安装包使用配套 xinput 加载器入口。构建 ASI 时选择单一入口进行初始化。
