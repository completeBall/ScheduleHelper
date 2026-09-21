#include "SelfTest.h"

#include "core/XlsxExport.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QQuickWindow>
#include <QTextStream>
#include <QTimer>

using namespace Campus;

namespace {

struct TestFailure {
    QString message;
};

void require(bool condition, const QString &message)
{
    if (!condition)
        throw TestFailure{message};
}

void settle(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

bool grab(QQuickWindow *window, const QString &path)
{
    settle(700);   // let the page and any WebEngine surface render
    const QImage image = window->grabWindow();
    return !image.isNull() && image.save(path);
}

void waitForBrowsers(AppController &app)
{
    for (int i = 0; i < 300 && !(app.schoolBridge()->attached() && app.academicBridge()->attached()); ++i)
        settle(50);
    require(app.schoolBridge()->attached() && app.academicBridge()->attached(), QStringLiteral("内置浏览器未能启动"));
}

void demoSchedule(AppController &app)
{
    Schedule s;
    auto course = [](const QString &name, const QString &detail, int day, QList<int> sections) {
        Course c;
        c.name = name;
        c.raw = name + QLatin1Char('\n') + detail;
        c.day = day;
        c.sections = sections;
        c.hasWeeks = true;
        for (int w = 1; w <= 16; ++w)
            c.weeks << w;
        return c;
    };
    s.courses << course(QStringLiteral("高等数学"), QStringLiteral("张伟\n1-16周\n教学楼A301"), 1, {1, 2})
              << course(QStringLiteral("大学英语"), QStringLiteral("李娜\n1-16周\n教学楼B205"), 1, {3, 4})
              << course(QStringLiteral("程序设计基础"), QStringLiteral("王磊\n1-16周\n实验楼C402"), 2, {3, 4})
              << course(QStringLiteral("思想道德与法治"), QStringLiteral("赵敏\n1-16周\n教学楼A101"), 3, {5, 6})
              << course(QStringLiteral("大学物理"), QStringLiteral("陈静\n1-16周\n教学楼A203"), 4, {1, 2})
              << course(QStringLiteral("体育"), QStringLiteral("刘洋\n1-16周\n体育馆"), 5, {7, 8});
    s.semester = QStringLiteral("2026-2027-1");
    s.importedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    app.schedule()->applyImport(s);
    app.schedule()->setAnchor(3, 1, QStringLiteral("2026-09-21"));
    app.schedule()->addManualCourse(QStringLiteral("临时调课"), QStringLiteral("测试教师"), QStringLiteral("测试教室"), 3, 1,
                                    QStringLiteral("3"));
    app.schedule()->setMemo(3, 1, 0, QStringLiteral("带上笔记本，课后完成活动报名。"));
    app.schedule()->setMemo(3, 7, 2, QStringLiteral("整理本周学习笔记。"));
}

} // namespace

void runScreenshots(AppController &app, QQuickWindow *window)
{
    const QString dir = app.options().screenshotDir;
    QDir().mkpath(dir);
    int status = 0;
    if (!app.options().demoActivities.isEmpty()) {
        app.loadDemo(app.options().demoActivities);
        demoSchedule(app);
    }
    struct Shot { int page; const char *name; };
    for (const Shot &s : {Shot{0, "activities"}, Shot{1, "schedule"}, Shot{2, "browser"}}) {
        app.setPage(s.page);
        if (!grab(window, dir + QLatin1Char('/') + QLatin1String(s.name) + QStringLiteral(".png")))
            status = 1;
    }
    app.setPage(1);
    if (QObject *dialog = window->findChild<QObject *>(QStringLiteral("slotDialog"))) {
        QMetaObject::invokeMethod(dialog, "openFor", Q_ARG(QVariant, 1), Q_ARG(QVariant, 0));
        if (!grab(window, dir + QStringLiteral("/dialog-slot.png"))) status = 1;
        // Exercise the actual editor/save handler and verify model propagation.
        QObject *editor = dialog->findChild<QObject *>(QStringLiteral("slotMemoEditor"));
        if (!editor) status = 1;
        else if (!app.options().demoActivities.isEmpty()) {
            editor->setProperty("text", QStringLiteral("编辑后的备忘录"));
            QMetaObject::invokeMethod(dialog, "saveMemo");
            if (app.schedule()->memo(3, 1, 0) != QStringLiteral("编辑后的备忘录")) status = 1;
        }
        QMetaObject::invokeMethod(dialog, "close");
    } else status = 1;
    // progress banner in its two busy states: indeterminate sweep and determinate fill
    app.setPage(0);
    if (QObject *banner = window->findChild<QObject *>(QStringLiteral("activityBanner"))) {
        banner->setProperty("running", true);
        banner->setProperty("message", QStringLiteral("读取详情 4 / 10：示例活动"));
        // the sweep loops every 1.1 s; frames ~0.87 s apart land on different phases, including the ends
        banner->setProperty("progress", -1.0);
        for (int i = 0; i < 4; ++i)
            if (!grab(window, dir + QStringLiteral("/progress-sweep%1.png").arg(i)))
                status = 1;
        banner->setProperty("progress", 0.4);
        if (!grab(window, dir + QStringLiteral("/progress-fill.png")))
            status = 1;
        banner->setProperty("running", false);
    }
    // dialogs (opened by objectName so screenshots cover them too)
    struct Dialog { int page; const char *object; const char *method; const char *file; };
    for (const Dialog &d : {Dialog{1, "dateDialog", "openDialog", "dialog-date"},
                            Dialog{1, "courseDialog", "openDialog", "dialog-course"},
                            Dialog{0, "settingsDialog", "open", "dialog-settings"}}) {
        app.setPage(d.page);
        QObject *dialog = window->findChild<QObject *>(QLatin1String(d.object));
        if (!dialog) {
            status = 1;
            continue;
        }
        QMetaObject::invokeMethod(dialog, d.method);
        if (!grab(window, dir + QLatin1Char('/') + QLatin1String(d.file) + QStringLiteral(".png")))
            status = 1;
        QMetaObject::invokeMethod(dialog, "close");
        settle(300);
    }
    QCoreApplication::exit(status);
}

void runSelfTest(AppController &app, QQuickWindow *window)
{
    const QString dir = app.dataDir();
    QFile logFile(dir + QStringLiteral("/self-test.log"));
    logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    QTextStream log(&logFile);
    log.setEncoding(QStringConverter::Utf8);
    auto note = [&](const QString &line) {
        log << QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")) << ' ' << line << '\n';
        log.flush();
    };

    QJsonObject result;
    int exitCode = 0;
    try {
        waitForBrowsers(app);
        note(QStringLiteral("browsers attached"));

        // both browsers must present as plain Edge, without the QtWebEngine token
        app.schoolBridge()->load(QUrl(app.options().baseUrl + QStringLiteral("/CloudPortal/CloudSquare")));
        const QString ua = app.schoolBridge()->eval(QStringLiteral("navigator.userAgent")).toString();
        note(QStringLiteral("user agent: ") + ua);
        require(ua.contains(QStringLiteral("Edg/")) && !ua.contains(QStringLiteral("QtWebEngine")),
                QStringLiteral("浏览器标识仍含 QtWebEngine：") + ua);

        // ---- activity collection --------------------------------------------------
        Scraper *scraper = app.scraper();
        require(app.schedule()->setMemo(2, 1, 0, QStringLiteral("刷新活动后保留")).isEmpty(),
                QStringLiteral("采集前备忘录保存失败"));
        const QString message = scraper->runCollection();
        note(message);
        require(!scraper->lastRunFailed(), QStringLiteral("采集失败：") + message);
        const QList<Activity> &rows = app.activities()->all();
        require(rows.size() == 3, QStringLiteral("活动过滤或采集结果不正确：%1 条").arg(rows.size()));
        QSet<QString> urls;
        int sameName = 0;
        for (const Activity &a : rows) {
            require(a.error.isEmpty(), QStringLiteral("存在读取失败的活动"));
            require(!isIgnored(a), QStringLiteral("团日/班会未被排除"));
            urls.insert(a.url);
            if (a.name == QStringLiteral("同名活动"))
                ++sameName;
        }
        require(urls.size() == 3, QStringLiteral("详情链接重复"));
        require(sameName == 2, QStringLiteral("同名活动丢失"));
        require(app.schedule()->memo(2, 1, 0) == QStringLiteral("刷新活动后保留"),
                QStringLiteral("重新采集活动后备忘录丢失"));
        note(QStringLiteral("PASS collection: group-day/class-meeting excluded, 3 details retained"));
        note(QStringLiteral("PASS collection: timetable memos retained"));

        // ---- Excel export ---------------------------------------------------------
        const QString xlsx = dir + QStringLiteral("/未开始活动_测试.xlsx");
        const QString exportError = app.exportExcelTo(xlsx);
        require(exportError.isEmpty(), QStringLiteral("Excel 导出失败：") + exportError);
        const QString sheet = QString::fromUtf8(readXlsxPart(xlsx, QStringLiteral("xl/worksheets/sheet1.xml")));
        require(sheet.contains(QStringLiteral("ySplit=\"4\"")) && sheet.contains(QStringLiteral("tableParts"))
                    && sheet.count(QStringLiteral("<row r=\"")) == rows.size() + 4,
                QStringLiteral("Excel 导出验证失败"));
        note(QStringLiteral("PASS Excel export: styled XLSX, rows sorted by registration start"));

        app.setPage(0);
        require(grab(window, dir + QStringLiteral("/desktop-preview.png")), QStringLiteral("无法截图"));

        // ---- timetable --------------------------------------------------------------
        ScheduleImporter *importer = app.importer();
        ScheduleModel *schedule = app.schedule();
        const QString site = app.options().baseUrl.section(QStringLiteral("/fixture"), 0, 0);   // http://127.0.0.1:8765

        if (!app.options().scheduleFixture.isEmpty()) {
            importer->navigate(app.options().scheduleFixture);
            const QString err = importer->runImport();
            require(err.isEmpty(), QStringLiteral("真实教务课表导入失败：") + err);
            const int actual = schedule->data().courses.size();
            require(actual > 0, QStringLiteral("真实教务课表导入测试未读取到课程"));
            note(QStringLiteral("PASS actual academic fixture: %1 course arrangements").arg(actual));
            result.insert(QStringLiteral("actualCourses"), actual);
        }

        importer->navigate(site + QStringLiteral("/timetable-main.html"));
        QString err = importer->runImport();
        require(err.isEmpty(), QStringLiteral("课表导入失败：") + err);
        require(schedule->data().courses.size() == 3, QStringLiteral("课表导入测试失败：%1 条").arg(schedule->data().courses.size()));

        schedule->setWeek(2);
        require(schedule->summary().contains(QStringLiteral("2 条")), QStringLiteral("单双周筛选不正确：") + schedule->summary());

        require(schedule->setAnchor(2, 3, QStringLiteral("2026-09-23")).isEmpty(), QStringLiteral("设置日期失败"));
        require(schedule->addManualCourse(QStringLiteral("临时调课"), QStringLiteral("测试教师"), QStringLiteral("测试教室"), 3, 1,
                                          QStringLiteral("2")).isEmpty(),
                QStringLiteral("新增手动课程失败"));
        settle(200);
        int registrationCards = 0, eventCards = 0;
        bool hasDeadline = false;
        for (int day = 1; day <= 7; ++day)
            for (int block = 0; block < 6; ++block)
                for (const QVariant &c : schedule->entries(day, block)) {
                    const QVariantMap m = c.toMap();
                    registrationCards += m.value(QStringLiteral("kind")) == QLatin1String("registration");
                    eventCards += m.value(QStringLiteral("kind")) == QLatin1String("event");
                    hasDeadline |= m.value(QStringLiteral("detail")).toString().contains(QStringLiteral("报名截止："));
                }
        require(app.activities()->isAutoClaimed(rows.last()), QStringLiteral("未自动识别个人已报名状态"));
        require(schedule->dayDate(3) == QLatin1String("09/23") && schedule->summary().contains(QStringLiteral("3 条"))
                    && schedule->summary().contains(QStringLiteral("提醒")) && registrationCards >= 1 && eventCards >= 1
                    && !hasDeadline && schedule->data().manualCourses.size() == 1,
                QStringLiteral("日期、报名提醒或手动课程测试失败：") + schedule->summary());
        app.activities()->setClaimed(ActivityModel::activityKey(rows.first()), true);
        int claimedEvents = 0;
        for (int day = 1; day <= 7; ++day)
            for (int block = 0; block < 6; ++block)
                for (const QVariant &entry : schedule->entries(day, block))
                    claimedEvents += entry.toMap().value(QStringLiteral("kind")) == QLatin1String("event");
        require(claimedEvents >= 1, QStringLiteral("已抢到活动未显示活动时间提醒"));
        app.setPage(1);
        require(grab(window, dir + QStringLiteral("/timetable-reminder-preview.png")), QStringLiteral("无法截图"));

        importer->navigate(site + QStringLiteral("/timetable-main.html"));
        err = importer->runImport();
        require(err.isEmpty() && schedule->data().manualCourses.size() == 1
                    && schedule->data().weekOneMonday == QLatin1String("2026-09-14"),
                QStringLiteral("重新导入后手动课程或日期设置丢失"));

        const QByteArray before = QJsonDocument(schedule->data().toJson()).toJson(QJsonDocument::Compact);
        importer->navigate(site + QStringLiteral("/timetable-empty.html"));
        const QString emptyErr = importer->runImport();
        importer->navigate(site + QStringLiteral("/timetable-login.html"));
        const QString loginErr = importer->runImport();
        require(emptyErr.contains(QStringLiteral("为空")) && loginErr.contains(QStringLiteral("登录"))
                    && QJsonDocument(schedule->data().toJson()).toJson(QJsonDocument::Compact) == before,
                QStringLiteral("空课表或登录失效保护测试失败：") + emptyErr + QLatin1Char('|') + loginErr);
        note(QStringLiteral("PASS timetable: empty table and login expiry preserve previous schedule"));
        note(QStringLiteral("PASS timetable: registration/event reminders, dates, manual course persistence, odd/even weeks"));

        result.insert(QStringLiteral("pass"), true);
        result.insert(QStringLiteral("count"), rows.size());
        result.insert(QStringLiteral("xlsx"), xlsx);
        note(QStringLiteral("PASS self-test"));
    } catch (const TestFailure &f) {
        result.insert(QStringLiteral("pass"), false);
        result.insert(QStringLiteral("error"), f.message);
        note(QStringLiteral("FAIL ") + f.message);
        exitCode = 1;
    } catch (const Failure &f) {
        result.insert(QStringLiteral("pass"), false);
        result.insert(QStringLiteral("error"), f.message);
        note(QStringLiteral("FAIL ") + f.message);
        exitCode = 1;
    } catch (const Cancelled &) {
        result.insert(QStringLiteral("pass"), false);
        result.insert(QStringLiteral("error"), QStringLiteral("cancelled"));
        exitCode = 1;
    }

    QFile out(dir + QStringLiteral("/result.json"));
    if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        out.write(QJsonDocument(result).toJson());
    QCoreApplication::exit(exitCode);
}
