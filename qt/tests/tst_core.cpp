// Golden tests ported from tests/core.test.mjs and tests/timetable.test.mjs.
#include "core/Activity.h"
#include "core/Storage.h"
#include "core/Timetable.h"
#include "core/XlsxExport.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

using namespace Campus;

namespace {

Activity base()
{
    Activity a;
    a.name = QStringLiteral("测试活动");
    a.registration = QStringLiteral("2026/09/21 10:00 —— 2026/09/21 12:00");
    a.remaining = QStringLiteral("2");
    return a;
}

QDateTime at(const char *text)
{
    return parseTimestamp(QString::fromUtf8(text));
}

QList<int> weeks(const char *text)
{
    QList<int> w;
    return weekNumbers(QString::fromUtf8(text), &w) ? w : QList<int>{-1};
}

} // namespace

class CoreTests : public QObject
{
    Q_OBJECT
private slots:
    // ---- core.test.mjs ------------------------------------------------------
    void timestampIsBeijingTime()
    {
        QCOMPARE(at("2026/09/21 10:00").toUTC(),
                 QDateTime(QDate(2026, 9, 21), QTime(2, 0), QTimeZone::utc()));
    }

    void statusBoundaries()
    {
        Activity a = base();
        QCOMPARE(status(a, at("2026/09/21 09:59")), Status::NotStarted);
        QCOMPARE(status(a, at("2026/09/21 10:00")), Status::Open);
        Activity full = a;
        full.remaining = QStringLiteral("0");
        QCOMPARE(status(full, at("2026/09/21 11:00")), Status::Full);
        QCOMPARE(status(a, at("2026/09/21 12:00")), Status::Closed);
        Activity unknown = a;
        unknown.registration = QStringLiteral("——");
        QCOMPARE(status(unknown), Status::Unknown);
        Activity failed = a;
        failed.error = QStringLiteral("读取超时");
        QCOMPARE(status(failed), Status::Failed);
    }

    void statusIsIndependentOfLocalZone()
    {
        // 02:00 UTC == 10:00 Beijing, so this instant is "open" in any local zone.
        const QDateTime utc(QDate(2026, 9, 21), QTime(2, 30), QTimeZone::utc());
        QCOMPARE(status(base(), utc), Status::Open);
    }

    void missingTimesSortLast()
    {
        QList<Activity> list;
        Activity b = base();
        b.id = QStringLiteral("b");
        b.registration = QStringLiteral("2026/09/22 10:00 —— 2026/09/22 12:00");
        Activity c = base();
        c.id = QStringLiteral("c");
        c.registration.clear();
        Activity a = base();
        a.id = QStringLiteral("a");
        list << b << c << a;
        sortByRegistrationStart(list);
        QCOMPARE(list[0].id, QStringLiteral("a"));
        QCOMPARE(list[1].id, QStringLiteral("b"));
        QCOMPARE(list[2].id, QStringLiteral("c"));
    }

    void safeUrlWhitelist()
    {
        const QString ok = QStringLiteral(
            "https://study.gdipu.edu.cn/CloudPortal/CloudActivityDetail?wid=1&dataType=HD");
        QCOMPARE(safeUrl(ok), ok);
        QVERIFY(safeUrl(QStringLiteral("javascript:alert(1)")).isEmpty());
        QVERIFY(safeUrl(QStringLiteral("http://study.gdipu.edu.cn/CloudPortal/CloudActivityDetail")).isEmpty());
        QVERIFY(safeUrl(QStringLiteral("https://evil.example/CloudPortal/CloudActivityDetail")).isEmpty());
        QVERIFY(safeUrl(QStringLiteral("https://study.gdipu.edu.cn/other")).isEmpty());
    }

    void ignoredActivities()
    {
        Activity a = base();
        QVERIFY(!isIgnored(a));
        a.name = QStringLiteral("三月团日活动");
        QVERIFY(isIgnored(a));
        a.name = QStringLiteral("主题班会");
        QVERIFY(isIgnored(a));
        a = base();
        a.type = QStringLiteral("班级活动");
        QVERIFY(isIgnored(a));
    }

    void sampleDataIsCompleteAndParsable()
    {
        QFile f(QStringLiteral(REPO_ROOT "/activities.json"));
        QVERIFY2(f.open(QIODevice::ReadOnly), "activities.json missing");
        const QJsonArray rows = QJsonDocument::fromJson(f.readAll()).array();
        QCOMPARE(rows.size(), 25);
        QSet<QString> ids;
        int sameName = 0;
        for (const QJsonValue &v : rows) {
            const Activity a = Activity::fromJson(v.toObject());
            ids.insert(a.id);
            if (a.name == QStringLiteral("中华人民共和国保守国家秘密法"))
                ++sameName;
            const RegistrationDates d = registrationDates(a);
            QVERIFY2(!d.start.isEmpty() && !d.end.isEmpty(), qPrintable(a.name));
            QVERIFY(!a.activityTime.isEmpty() && !a.place.isEmpty() && !a.capacity.isEmpty());
            if (a.capacity != QStringLiteral("不限"))
                QCOMPARE(a.capacity.toInt() - a.registered.toInt(), a.remaining.toInt());
        }
        QCOMPARE(ids.size(), 25);
        QCOMPARE(sameName, 2);
    }

    void jsonRoundTripKeepsFields()
    {
        QFile f(QStringLiteral(REPO_ROOT "/activities.json"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QJsonObject first = QJsonDocument::fromJson(f.readAll()).array().first().toObject();
        const Activity a = Activity::fromJson(first);
        const QJsonObject back = a.toJson();
        for (auto it = first.begin(); it != first.end(); ++it)
            QCOMPARE(back.value(it.key()), it.value());
    }

    void storageRoundTripAndAtomicWrite()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("last-results.json"));
        QList<Activity> rows = {base()};
        rows[0].url = QStringLiteral("https://study.gdipu.edu.cn/CloudPortal/CloudActivityDetail?wid=1");
        Activity ignored = base();
        ignored.name = QStringLiteral("班会");
        rows << ignored;
        QVERIFY(saveActivities(path, rows));
        QList<Activity> loaded;
        QVERIFY(loadActivities(path, &loaded));
        QCOMPARE(loaded.size(), 1);              // ignored rows are filtered on load
        QCOMPARE(loaded[0].url, rows[0].url);
        QVERIFY(!QFile::exists(path + QStringLiteral(".tmp")));

        QFile bad(dir.filePath(QStringLiteral("bad.json")));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("{oops");
        bad.close();
        QString err;
        QVERIFY(!loadActivities(bad.fileName(), &loaded, &err));
        QVERIFY(!err.isEmpty());
    }

    // ---- timetable.test.mjs ---------------------------------------------------
    void periodsMatchTheTimetable()
    {
        QStringList times;
        for (const Period &p : periods())
            times << p.time;
        QCOMPARE(times, (QStringList{QStringLiteral("08:30–09:55"), QStringLiteral("10:15–11:40"),
                                     QStringLiteral("14:00–15:25"), QStringLiteral("15:45–17:10"),
                                     QStringLiteral("18:30–19:55"), QStringLiteral("20:00–21:25")}));
        QCOMPARE(dayNames().size(), 7);
    }

    void slotsAreSeparatedFromClockNumbers()
    {
        QCOMPARE(slotsFrom(QString::fromUtf8("上午 第1、2节 08:30-09:55")), (QList<int>{1, 2}));
        QCOMPARE(slotsFrom(QString::fromUtf8("第11、12节 20:00-21:25")), (QList<int>{11, 12}));
        QCOMPARE(slotsFrom(QString::fromUtf8("[03-04]节")), (QList<int>{3, 4}));
        QVERIFY(slotsFrom(QString::fromUtf8("08:30-09:55")).isEmpty());
    }

    void bigSectionsAndBounds()
    {
        QCOMPARE(slotsFrom(QString::fromUtf8("第一大节")), (QList<int>{1, 2}));
        QCOMPARE(slotsFrom(QString::fromUtf8("第六大节")), (QList<int>{11, 12}));
        QCOMPARE(slotsFrom(QString::fromUtf8("03-04")), (QList<int>{3, 4}));
        QVERIFY(slotsFrom(QString::fromUtf8("第13、14节")).isEmpty());
    }

    void weekExpressions()
    {
        QCOMPARE(weeks("1-6(单周)[01-02节]"), (QList<int>{1, 3, 5}));
        QCOMPARE(weeks("2-8周(双)"), (QList<int>{2, 4, 6, 8}));
        QCOMPARE(weeks("1-3,5,7-8(周)"), (QList<int>{1, 2, 3, 5, 7, 8}));
        QList<int> none;
        QVERIFY(!weekNumbers(QString::fromUtf8("教室102 第1、2节"), &none));
    }

    void manualWeekInput()
    {
        QList<int> w;
        QVERIFY(parseManualWeeks(QStringLiteral("2"), &w));
        QCOMPARE(w, (QList<int>{2}));
        QVERIFY(parseManualWeeks(QStringLiteral("2-6"), &w));
        QCOMPARE(w, (QList<int>{2, 3, 4, 5, 6}));
        QVERIFY(parseManualWeeks(QString::fromUtf8("1-8单周"), &w));
        QCOMPARE(w, (QList<int>{1, 3, 5, 7}));
        QVERIFY(!parseManualWeeks(QStringLiteral("abc"), &w));
    }

    void anchorDatesAreFilledIn()
    {
        const QString anchor = weekOneMonday(2, 3, QStringLiteral("2026-09-23"));
        QCOMPARE(anchor, QStringLiteral("2026-09-14"));
        QCOMPARE(dateFor(anchor, 1, 1), QStringLiteral("2026-09-14"));
        QCOMPARE(dateFor(anchor, 2, 3), QStringLiteral("2026-09-23"));
        QCOMPARE(dateFor(anchor, 3, 7), QStringLiteral("2026-10-04"));
        QVERIFY(weekOneMonday(0, 1, QStringLiteral("2026-09-23")).isEmpty());
        QVERIFY(weekOneMonday(1, 1, QStringLiteral("2026-02-30")).isEmpty());
    }

    void registrationStartMapsToSlot()
    {
        const QString anchor = QStringLiteral("2026-09-14");
        Reminder r;
        QVERIFY(reminderFor(QString::fromUtf8("2026/09/21 09:31 —— 2026/09/21 20:00"), anchor, &r));
        QCOMPARE(r.week, 2);
        QCOMPARE(r.day, 1);
        QCOMPARE(r.block, 0);
        QCOMPARE(r.start, QStringLiteral("2026-09-21 09:31"));
        QVERIFY(reminderFor(QString::fromUtf8("2026/09/21 10:15 —— 2026/09/21 20:00"), anchor, &r));
        QCOMPARE(r.block, 1);
        QVERIFY(reminderFor(QString::fromUtf8("2026/09/23 20:00 —— 2026/09/23 21:00"), anchor, &r));
        QCOMPARE(r.block, 5);
        // before week one and after week forty are not shown
        QVERIFY(!reminderFor(QStringLiteral("2026/09/13 10:00 — x"), anchor, &r));
        QVERIFY(!reminderFor(QStringLiteral("2027/09/01 10:00 — x"), anchor, &r));
    }

    void registrationAndEventAreIndependent()
    {
        Activity a = base();
        a.registration = QString::fromUtf8("2026/09/21 10:30 —— 2026/09/21 20:00");
        a.activityTime = QStringLiteral("2026/09/23 14:00 - 15:00");
        const QList<Reminder> m = activityMoments(a, QStringLiteral("2026-09-14"));
        QCOMPARE(m.size(), 2);
        QCOMPARE(int(m[0].kind), int(Reminder::Registration));
        QCOMPARE(int(m[1].kind), int(Reminder::Event));
        QCOMPARE(QList<int>({m[0].week, m[0].day, m[0].block}), (QList<int>{2, 1, 1}));
        QCOMPARE(QList<int>({m[1].week, m[1].day, m[1].block}), (QList<int>{2, 3, 2}));
    }

    void scheduleJsonKeepsUnknownWeeksAsNull()
    {
        Schedule s;
        Course c;
        c.name = QStringLiteral("高等数学");
        c.raw = QStringLiteral("高等数学\n教室 A101");
        c.day = 3;
        c.sections = {1, 2};
        c.hasWeeks = false;
        s.courses << c;
        Course m;
        m.manual = true;
        m.id = QStringLiteral("manual-1");
        m.name = QStringLiteral("临时调课");
        m.day = 3;
        m.sections = {3, 4};
        m.hasWeeks = true;
        m.weeks = {2};
        s.manualCourses << m;
        s.weekOneMonday = QStringLiteral("2026-09-14");
        s.selectedWeek = QStringLiteral("2");

        const Schedule back = Schedule::fromJson(s.toJson());
        QCOMPARE(back.courses.size(), 1);
        QVERIFY(!back.courses[0].hasWeeks);
        QVERIFY(back.courses[0].inWeek(5));          // unknown weeks show in every week
        QCOMPARE(back.manualCourses.size(), 1);
        QVERIFY(back.manualCourses[0].manual);
        QVERIFY(!back.manualCourses[0].inWeek(3));
        QCOMPARE(back.weekOneMonday, s.weekOneMonday);
        QVERIFY(back.courses[0].inBlock(0));
        QVERIFY(!back.courses[0].inBlock(1));
    }

    // ---- xlsx -----------------------------------------------------------------
    void xlsxExportIsStyledAndSorted()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("out.xlsx"));

        QList<Activity> rows;
        Activity late = base();
        late.name = QStringLiteral("后开始");
        late.registration = QStringLiteral("2026/09/25 10:00 —— 2026/09/25 12:00");
        late.capacity = QStringLiteral("30");
        Activity early = base();
        early.name = QStringLiteral("先开始 & <特殊>");
        early.capacity = QStringLiteral("不限");
        Activity ignored = base();
        ignored.name = QStringLiteral("团日活动");
        Activity failed = base();
        failed.name = QStringLiteral("失败项");
        failed.error = QStringLiteral("超时");
        rows << late << ignored << early << failed;

        QString err;
        QVERIFY2(exportXlsx(path, rows, at("2026/09/21 11:00"), &err), qPrintable(err));
        QVERIFY(!QFile::exists(path + QStringLiteral(".tmp")));

        const QString sheet = QString::fromUtf8(readXlsxPart(path, QStringLiteral("xl/worksheets/sheet1.xml")));
        QVERIFY(sheet.contains(QStringLiteral("ySplit=\"4\"")));
        QVERIFY(sheet.contains(QStringLiteral("tableParts")));
        QCOMPARE(sheet.count(QStringLiteral("<row r=\"")), 3 + 4 + 1 - 1);   // title rows + header + 3 data rows
        QVERIFY(!sheet.contains(QStringLiteral("团日活动")));
        QVERIFY(sheet.contains(QStringLiteral("先开始 &amp; &lt;特殊&gt;")));
        QVERIFY(sheet.indexOf(QStringLiteral("先开始")) < sheet.indexOf(QStringLiteral("后开始")));
        // regression fix: failed rows show "读取失败" like the on-screen table
        QVERIFY(sheet.contains(Status::Failed));
        // 2026-09-21 10:00 as an Excel serial = 46286 + 10/24
        QVERIFY(sheet.contains(QStringLiteral("<v>46286.416666666664</v>"))
                || sheet.contains(QStringLiteral("<v>46286.41666666667</v>")));
        QVERIFY(!readXlsxPart(path, QStringLiteral("xl/tables/table1.xml")).isEmpty());
        QVERIFY(!readXlsxPart(path, QStringLiteral("xl/styles.xml")).isEmpty());
    }

    void xlsxExportEmptyIsValid()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("empty.xlsx"));
        QVERIFY(exportXlsx(path, {}, nowInBeijing()));
        const QString sheet = QString::fromUtf8(readXlsxPart(path, QStringLiteral("xl/worksheets/sheet1.xml")));
        QVERIFY(sheet.contains(QStringLiteral("暂无符合条件的活动")));
    }
};

QTEST_APPLESS_MAIN(CoreTests)
#include "tst_core.moc"
