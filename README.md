# ScheduleHelper（广轻活动汇总）

用于采集广东轻工职业技术大学智慧校园中的未开始活动，并与个人课表联动显示报名提醒和活动开始提醒。

Windows 桌面程序使用 C++17 / Qt 6.8，编译与运行方法见下文。首次使用时需要分别登录活动系统和教务系统。

主要功能：

- 采集活动名称、报名时间、活动地点、活动时间和名额等信息。
- 自动排除团日、班会以及活动类型为“班级活动”的项目。
- 按报名开始时间排序并导出带格式的 Excel `.xlsx` 表格。
- 从教务系统一键导入课表，支持日期补全和手动添加临时调课。
- 在课表中分别显示橙色报名提醒卡和蓝色活动开始提醒卡。

## Qt 版（C++17 / Qt 6.8 · Qt Quick + Qt WebEngine）

`qt/` 包含 C++ 与 Qt 桌面程序源码，界面支持统一的侧栏外壳、浅色/深色主题、指标卡、可筛选的活动列表、自适应课表网格、表单对话框。

```
qt/src/core/     纯 C++ 逻辑，无 UI：活动状态与排序、周次/节次解析、日期推算、提醒映射、JSON 存取、XLSX 导出
qt/app/          应用：QML 界面（qml/）、模型、采集状态机 Scraper、课表导入 ScheduleImporter、WebBridge
qt/app/js/       注入学校页面的 DOM 抽取脚本（从 extension/*.js 剥离，选择器与解析规则不变）
qt/tests/        Qt Test：移植自 tests/*.test.mjs 的用例
```

数据保存在 `%LOCALAPPDATA%\GdipuActivityHelper` 下的 `last-results.json`、`timetable.json`，已有数据可直接读取。浏览器登录状态使用新的 `BrowserProfileQt`，首次需要重新登录一次。

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

Excel 中读取失败的活动显示“读取失败”；所有时间判断固定按北京时间（UTC+8），不再依赖本机时区。

## JavaScript 解析测试

保留 `extension/` 中的解析脚本及其测试，可在仓库根目录执行：

```bat
node --test --test-isolation=none tests/core.test.mjs tests/timetable.test.mjs
```

模拟课表页面可通过 `node tests/build-timetable-fixture.mjs` 生成，供 Qt 端到端自检使用。
