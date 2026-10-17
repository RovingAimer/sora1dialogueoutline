# 加载器接入

配套 [通用加载器 v0.2.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.2.0) 在资源 hook 尝试之后扫描 `mods` 的一级子目录；源码和验收记录见 [loader.lock.json](../loader.lock.json)。字幕 v0.1.1 主包和配置更新包均提供 `mods/DialogueOutline/sora_mod.ini`：

```ini
[SoraMod]
Api=1
DLL=DialogueOutline.dll
Initialize=SoraDialogueInitialize
```

核心入口为无参数 C 名称导出 `DWORD WINAPI SoraDialogueInitialize()`，在 loader lock 外调用。加载器原样记录返回值，不解释状态位。正常值为 `0x14F`；`0x008` 表示已安装 hook，`0x020` 表示目标地址冲突，`0x800` 表示内存最终处理警告。核心用 InitOnce 缓存初始化结果。用户已确认该组合的字幕描边正常。

核心保留 `InitializeASI` 导出；当前发布安装包使用通用 xinput 加载器入口。历史 v0.1.0 加载器按固定路径接入字幕核心，其来源保存在 loader lock 的历史字段中。
