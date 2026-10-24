# 加载器接入

DialogueOutline v0.2.1 配合[通用加载器 v0.2.0](https://github.com/RovingAimer/sora1looseload/releases/tag/v0.2.0) 使用。核心安装于 `mods/DialogueOutline/DialogueOutline.dll`，由同目录声明文件加载：

```ini
[SoraMod]
Api=1
DLL=DialogueOutline.dll
Initialize=SoraDialogueInitialize
```

主安装包包含 `sora_mod.ini` 与 `DialogueOutline.ini`。前者声明模块入口，后者设置正文、姓名、对白区域比例和描边粗细。格式与生效方式见 [CONFIGURATION.md](../CONFIGURATION.md)。

核心入口为无参数 C 名称导出 `DWORD WINAPI SoraDialogueInitialize()`，在 loader lock 外调用；加载器原样记录其返回值。核心、描边与布局 hook 全部启用时状态值为 `0x114D`。`0x008` 表示描边 hook 启用，`0x1000` 表示布局 hook 启用，`0x2000` 表示布局接入失败，`0x4000` 表示配置目录通知不可用。核心使用 InitOnce 缓存初始化结果。

核心保留 `InitializeASI` 兼容导出。目标程序文件名与 SHA256 见 [INSTALL.md](../INSTALL.md)。加载器版本及来源记录见 [loader.lock.json](../loader.lock.json)。
