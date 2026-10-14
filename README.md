# TESMIOPOPMOD

[GitHub](https://github.com/HONGxuanzhao/TESMIOPOPMOD) | [Steam Workshop](https://steamcommunity.com/sharedfiles/filedetails/?id=3794910879)

## Download and installation / 下载与安装

The ready-to-use plugin files are in `plugins/`. Download this repository using **Code → Download ZIP**, extract it, and follow the instructions below.

可直接使用的插件文件位于 `plugins/`。点击 **Code → Download ZIP** 下载并解压，按下文安装。

## Package instructions / 使用说明

TESMIOPOPMOD — TESMIO / Republic Mod Loader 安装说明
====================================================

运行包结构
README.txt、previewimage.png 放在根目录；插件 DLL、INI 和 RML 元数据放在 plugins 目录。
本包面向 WRSR 1.1.1.9（64 位）。DLL 使用 TESMIO API 4；RML 使用其 TESMIO 兼容宿主加载。

TESMIO 安装
1. 完全退出游戏，从本包 plugins 目录复制 TESMIOPOPMOD.dll 和 TESMIOPOPMOD.ini 到：
   <游戏目录>\tesmioloader\build\plugins\（tesmioloader.dll 旁的 plugins 目录）。
2. 启用 TESMIO 插件扫描；若 [plugins] 显式关闭了 TESMIOPOPMOD，将其设为 1。
3. 使用 tesmiolauncher.exe 启动。RML 的 .rml.json 元数据无需复制给 TESMIO。

Republic Mod Loader（RML）安装
1. 订阅并下载对应创意工坊项目后，使用支持 TESMIO API 4 的 RML 启动器。
2. 刷新列表，在 Plugins 中启用 TESMIOPOPMOD；工坊文件应位于 <项目编号>\plugins\。
3. 手动安装时，将本包 plugins 内的 DLL、INI、.rml.json 复制到 RML 的 rml\plugins\。
4. INI 与实际加载的 DLL 放在同一目录。若已有手动安装副本，先备份个人设置再统一安装来源。

配置与发布
编辑实际加载目录内的 INI；保存为 UTF-8 无 BOM，并完整重启游戏。
同一个游戏进程选择一个 Loader 启动。可选的 Reputation 联动不构成强制依赖。
上传目录应包含 plugins、README.txt、previewimage.png 及该项目原有的 workshopconfig.ini。
保留各项目的 ITEM_ID、OWNER_ID 和可见性，使用 SCRIPT 项目类型；不要上传传输 ZIP。
本次调整验证了打包结构和接口；调整参数后的游戏效果及存档兼容性仍需实际运行验证。

English installation
====================
Target: WRSR 1.1.1.9 x64; TESMIO API 4 or RML with the TESMIO API 4 compatibility host.
TESMIO: copy TESMIOPOPMOD.dll and TESMIOPOPMOD.ini from this package's plugins folder into
<game>\tesmioloader\build\plugins, enable the plugin, and launch with tesmiolauncher.exe.
RML Workshop: subscribe/download the item, refresh the inventory and enable TESMIOPOPMOD.
RML manual: copy DLL, INI and .rml.json from plugins into RML's rml\plugins folder.
Keep the INI beside the DLL that is actually loaded. Edit settings, then fully restart.
Use one loader per game process. Reputation integration is optional, not a required dependency.
Publish the plugins folder with root README/preview and the item's original workshopconfig.ini;
preserve item/owner IDs and visibility, use SCRIPT type, and do not upload the transfer ZIP.
Package/interface checks do not establish gameplay behavior or save compatibility.

功能与参数 / Features and settings
====================================================
TESMIOPOPMOD 0.4.0-reputation2.1
中文 / English

中文
====
功能
- 调整居民出生速度，以及增加/降低出生率研究的效果。
- 调整部分居民死亡候选判断的基准值。
- 可与已核验的 Reputation 2.1 联动，应用其事件和阵营的出生倍率。
参数不是全国出生率或死亡率的百分比；实际人口变化仍取决于游戏条件和移民等因素。

要求与安装
需要 WRSR 1.1.1.9（64 位）和支持 TESMIO API 4 的 TESMIO 或 RML。可在中文或英文游戏中使用。
1. 将 TESMIOPOPMOD.dll 和 TESMIOPOPMOD.ini 复制到：
   <游戏目录>\tesmioloader\build\plugins\
2. 在 build\tesmioloader.ini 的 [plugins] 段加入 TESMIOPOPMOD=1。
3. 确认 TESMIOPOPMOD.ini 中 enabled=1，通过 tesmiolauncher.exe 启动。
使用 TESMIO 时，仍需手动安装 DLL 和 INI；使用 RML 时按开头的订阅安装步骤操作。

常用设置（编辑 TESMIOPOPMOD.ini，保存后完全退出并重启游戏）
- birth_base_factor：出生基础乘数；越小，出生累积越慢。原版约 0.6666667。
- birth_research_increase_factor：增加出生率研究启用时的乘数。
- birth_research_decrease_factor：降低出生率研究启用时的乘数。
- death_candidate_base_factor：部分死亡候选判断的基准值；原版 0.85。
  增大更容易成为死亡候选，减小则更难；它不是直接的死亡概率。
- reputation_compat_enabled：Reputation 2.1 联动开关；没有 Reputation 也能使用基础功能。
出生参数范围为 0..4，死亡候选参数为 0..2。完整说明见 INI 注释。
详细技术说明见 COMPATIBILITY_UPDATE.md。

English
=======
Features
- Adjust resident birth accumulation and the effects of birth-rate research.
- Adjust the baseline used in one resident death-candidate check.
- Optional integration with the verified Reputation 2.1 build applies its event
  and faction birth multipliers.
These settings are not nationwide birth/death percentages. Actual population
changes still depend on game conditions, immigration and other factors.

Requirements and installation
Requires WRSR 1.1.1.9 (64-bit) and TESMIO or RML with TESMIO API 4 support. Works with Chinese or English game text.
1. Copy TESMIOPOPMOD.dll and TESMIOPOPMOD.ini to:
   <game folder>\tesmioloader\build\plugins\
2. Add TESMIOPOPMOD=1 under [plugins] in build\tesmioloader.ini.
3. Set enabled=1 in TESMIOPOPMOD.ini and launch through tesmiolauncher.exe.
For TESMIO, manually install the DLL and INI; for RML, follow the subscription instructions above.

Main settings
Edit TESMIOPOPMOD.ini, save it, then fully exit and restart the game.
- birth_base_factor: base birth multiplier. Lower values slow accumulation;
  the vanilla value is approximately 0.6666667.
- birth_research_increase_factor: multiplier when birth-increasing research is active.
- birth_research_decrease_factor: multiplier when birth-decreasing research is active.
- death_candidate_base_factor: baseline for one death-candidate check (vanilla: 0.85).
  Higher values make that check more likely to pass; this is not a direct death probability.
- reputation_compat_enabled: enable Reputation 2.1 integration; base features also
  work without Reputation.
Birth settings accept 0..4; the death-candidate setting accepts 0..2.
See the INI comments for details. Technical details: COMPATIBILITY_UPDATE.md (Chinese).

GitHub: https://github.com/HONGxuanzhao/TESMIOPOPMOD
