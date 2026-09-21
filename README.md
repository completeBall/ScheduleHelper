# ScheduleHelper（广轻活动汇总）

用于采集广东轻工职业技术大学智慧校园中的未开始活动，并与个人课表联动显示报名提醒和活动开始提醒。

Windows 64 位成品程序位于 [`release/广轻活动汇总-第一版.exe`](release/广轻活动汇总-第一版.exe)，具体操作请阅读 [`release/使用说明.txt`](release/使用说明.txt)。程序首次使用时需要分别登录活动系统和教务系统。

主要功能：

- 采集活动名称、报名时间、活动地点、活动时间和名额等信息。
- 自动排除团日、班会以及活动类型为“班级活动”的项目。
- 按报名开始时间排序并导出带格式的 Excel `.xlsx` 表格。
- 从教务系统一键导入课表，支持日期补全和手动添加临时调课。
- 在课表中分别显示橙色报名提醒卡和蓝色活动开始提醒卡。

## Qt 版（C++17 / Qt 6.8 · Qt Quick + Qt WebEngine）

`qt/` 是用 C++ 与 Qt 重写的新版本，功能与下面的 C# 版一致，界面重新设计（统一的侧栏外壳、浅色/深色主题、指标卡、可筛选的活动列表、自适应课表网格、表单对话框）。

```
qt/src/core/     纯 C++ 逻辑，无 UI：活动状态与排序、周次/节次解析、日期推算、提醒映射、JSON 存取、XLSX 导出
qt/app/          应用：QML 界面（qml/）、模型、采集状态机 Scraper、课表导入 ScheduleImporter、WebBridge
qt/app/js/       注入学校页面的 DOM 抽取脚本（从 extension/*.js 剥离，选择器与解析规则不变）
qt/tests/        Qt Test：移植自 tests/*.test.mjs 的用例
```

数据目录与 JSON 格式沿用 C# 版（`%LOCALAPPDATA%\GdipuActivityHelper` 下的 `last-results.json`、`timetable.json`），已有数据可直接读取。浏览器登录状态使用新的 `BrowserProfileQt`，首次需要重新登录一次。

环境：Visual Studio 2022+（MSVC 14.44）、Qt 6.8.3 `msvc2022_64`（含 QtDeclarative、QtWebEngine、QtWebChannel、QtPositioning、QtShaderTools、QtSvg）。路径写在 `qt/build.cmd`，按本机修改。

```
qt\build.cmd            配置并编译；build.cmd test 额外运行单元测试；build.cmd noapp 只编译核心库与测试
qt\run.cmd              开发时运行（临时把 Qt 加入 PATH）
qt\deploy.cmd           编译并打包到 qt\dist\GdipuActivityHelper（约 210 MB，zip 后约 95 MB，其中 Chromium 内核占大头）
```

端到端自检：先 `node server.mjs` 启动模拟学校网站，再运行

```
qt\run.cmd --self-test <输出目录>          可选 --schedule-fixture <真实教务页面URL>
```

自检会驱动真实界面与内置浏览器完成：采集（团日/班会/班级活动被排除、同名活动保留）、XLSX 导出、课表导入、单双周、手动课程、双类型提醒卡片、空课表/登录失效不覆盖旧课表，并写出 `result.json` 与 `self-test.log`，退出码表示是否通过。`--screenshot <目录> --demo activities.json [--theme light|dark]` 可对各页面和对话框截图（不读写真实数据）。

与 C# 版的差异：Excel 中读取失败的活动现在显示“读取失败”（原先误显示为“时间待确认”）；所有时间判断固定按北京时间（UTC+8），不再依赖本机时区。

## Windows 桌面版源码（C#，原版）

入口：desktop/Launcher.cs；窗口、登录和采集：desktop/App.cs；Excel 导出：desktop/ExcelExport.cs；表格和字段解析：extension/core.js。

课表：desktop/Schedule.cs 是延迟初始化的桌面入口与教务浏览器，extension/timetable.js 提供表格读取、节次与周次解析、日期推算、手动课程及七天六时段界面。日期锚点和手动课程随 timetable.json 保存，重新导入教务课表时会合并保留；用户缓存与活动缓存独立。

活动联动：活动名称含“团日”或“班会”，或活动类型为“班级活动”时排除。其他活动按课表日期锚点分别把报名开始时间映射为橙色“活动报名提醒”，把活动开始时间映射为蓝色“活动开始提醒”；两者始终使用独立卡片，不附着在课程卡上，也不显示报名截止时间。活动提醒直接使用过滤后的活动缓存，不写入 timetable.json。

Excel 导出：导出当前搜索和筛选结果，按报名开始时间升序排列。输出为带标题、筛选器、冻结标题行、交替行底色、日期数值格式和适配列宽的 `.xlsx` 文件。

课表测试：先运行 `node tests/build-timetable-fixture.mjs`，再启动 server.mjs 和 EXE --self-test。单元测试包括 tests/timetable.test.mjs。2026-09-20 已使用用户从真实教务系统保存的主页面和学期理论课表完成兼容验证，识别 2026-2027-1 学期 53 条课程安排；底部“备注”行会被排除。

## 编译

在源码根目录执行 `pwsh -File desktop/build.ps1`。脚本使用 Windows 自带 .NET Framework C# 编译器，并在缺少 SDK 时从 NuGet 官方源下载固定版本的 WebView2 SDK，校验 SHA-256。

输出为 `dist/广轻活动汇总.exe`。程序将托管程序集和 WebView2Loader 嵌入单一 EXE，首次运行时释放到用户本地应用数据目录。WebView2 Runtime 使用系统已安装版本。

## 测试

`node --test --test-isolation=none tests/core.test.mjs` 验证数据处理。

`node server.mjs` 启动仅监听 127.0.0.1:8765 的模拟学校网站，再运行 `dist/广轻活动汇总.exe --self-test <本地测试输出目录>`。不要从限制 Chromium 子进程的沙箱启动测试。

最终 Windows EXE 的测试日志为本备份中的 `tests/desktop-verification.log`。测试验证活动过滤、同名不同编号、XLSX 实际导出、双提醒卡片，以及桌面浏览器渲染。

真实学校页面列表与详情已通过浏览器逐项核对，最终 EXE 的用户登录需要在程序内完成。

`licenses` 包含 WebView2 SDK 的许可证及第三方通知。
