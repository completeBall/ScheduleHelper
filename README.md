# ScheduleHelper（广轻活动汇总）

ScheduleHelper 是面向广东轻工职业技术大学智慧校园的 Windows 桌面程序，使用 C++17、Qt 6.8、Qt Quick 和 Qt WebEngine 开发。它可以采集未开始的校园活动、导入个人课表，并把报名、活动和备忘录集中显示在每周课表中。

## 功能

- 采集活动名称、报名时间、活动时间、地点、组织、名额和报名状态。
- 自动排除团日、班会和“班级活动”。
- 搜索、筛选、排序并导出带格式的 Excel `.xlsx` 文件。
- 从教务系统导入个人课表，支持单双周、手动课程和日期设置。
- 在课程旁显示黄色报名书签、红色活动书签和绿色备忘录书签。
- 点击课程或书签查看该时段的全部提醒，并编辑备忘录。
- 没有课程但存在提醒或备忘录时，自动创建空闲时段卡片。
- 从下拉框选择全部周次或第 1–30 周。
- 在课表页每天刷新活动；书签自动更新，备忘录不会被活动刷新或课表重新导入删除。
- 支持浅色、深色和跟随系统主题。

## 使用方法

1. 打开“学校 / 教务登录”，完成学校统一认证。
2. 在“活动汇总”点击“开始采集”，或在“我的课表”点击“刷新活动”。
3. 登录教务系统并点击“一键导入课表”。
4. 点击“设置日期”，指定任意周中的任意日期。
5. 从周次下拉框选择具体周次，即可查看提醒并添加备忘录。

程序只读取活动和课表信息，不会自动报名。缓存、课表、备忘录、设置和浏览器登录状态保存在：

```text
%LOCALAPPDATA%\GdipuActivityHelper
```

其中 `last-results.json` 保存活动缓存，`timetable.json` 保存课表和备忘录。每天重新采集只替换活动缓存，不会修改 `timetable.json` 中的备忘录。

## 源码结构

```text
qt/src/core/     活动、课表、存储和 XLSX 导出等 C++ 核心逻辑
qt/app/          Qt Quick 桌面应用、模型、采集器和教务导入器
qt/app/qml/      页面、对话框和通用界面组件
qt/app/js/       注入学校页面的 DOM 解析脚本
qt/tests/        Qt Test 自动测试
tests/           端到端自检所需的课表页面生成器
server.mjs       本地模拟学校网站
activities.json  演示和测试活动数据
```

## 编译环境

- Windows 10/11 x64
- Visual Studio 2022 或更高版本，安装“使用 C++ 的桌面开发”
- Qt 6.8.3 `msvc2022_64`
- Qt 模块：Declarative、WebEngine、WebChannel、Positioning、ShaderTools、SVG

构建脚本会通过 `vswhere` 自动查找 Visual Studio。Qt 默认从常见安装位置查找，也可以先设置：

```bat
set QT_ROOT=D:\Qt\6.8.3\msvc2022_64
```

编译 Release 版本：

```bat
qt\build.cmd
```

编译并运行测试：

```bat
qt\build.cmd test
```

仅编译核心库和测试：

```bat
qt\build.cmd noapp
```

开发运行：

```bat
qt\run.cmd
```

生成包含 Qt 和 WebEngine 运行文件的独立发布目录：

```bat
qt\deploy.cmd
```

输出目录为 `qt\dist\GdipuActivityHelper`。

## 测试

`qt/tests/tst_core.cpp` 验证活动处理、课表解析、存储和 XLSX 导出；`qt/tests/tst_schedule.cpp` 验证提醒书签、空闲卡片、备忘录持久化，以及活动刷新和课表重新导入不会删除备忘录。

端到端自检会运行真实 Qt 界面和内置浏览器。先在仓库根目录生成模拟课表页面并启动服务器：

```bat
node tests\build-timetable-fixture.mjs
node server.mjs
```

另开终端执行：

```bat
qt\run.cmd --self-test qt\build\self-test
```

自检涵盖活动采集与过滤、重复活动、Excel 导出、课表导入、单双周、手动课程、提醒映射、备忘录保留，以及登录失效或空课表时保护原数据。

## 数据与时间

所有活动时间固定按北京时间（UTC+8）判断。JSON 写入使用原子替换，避免写入中断损坏已有数据。活动详情链接只允许学校活动系统的 HTTPS 地址。
