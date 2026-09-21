#include "ScheduleModel.h"
#include <QTemporaryDir>
#include <QtTest>

class ScheduleTests : public QObject {
    Q_OBJECT
private slots:
    void bookmarksAndMemos() {
        QTemporaryDir dir;
        ActivityModel activities;
        ScheduleModel model(&activities);
        model.setStoragePath(dir.filePath("timetable.json"));
        QVERIFY(model.setAnchor(3, 1, "2026-09-21").isEmpty());
        Campus::Activity a;
        a.name = QStringLiteral("示例活动");
        a.registration = "2026/09/21 08:45 - 2026/09/21 09:00";
        a.activityTime = "2026/09/21 09:00 - 2026/09/21 09:30";
        activities.setRows({a, a});
        auto cards = model.cards(1, 0);
        QCOMPARE(cards.size(), 1);
        QCOMPARE(cards[0].toMap()["kind"].toString(), QString("empty"));
        QCOMPARE(cards[0].toMap()["registrationCount"].toInt(), 2);
        QCOMPARE(cards[0].toMap()["eventCount"].toInt(), 0);
        model.setActivityReminderMode(1);
        QCOMPARE(model.cards(1, 0)[0].toMap()["registrationCount"].toInt(), 2);
        QCOMPARE(model.cards(1, 0)[0].toMap()["eventCount"].toInt(), 2);
        model.setActivityReminderMode(0);
        QVERIFY(model.addManualCourse("Math", "", "", 1, 0, "3").isEmpty());
        cards = model.cards(1, 0);
        QCOMPARE(cards.size(), 1); // no standalone reminders below the course
        QCOMPARE(cards[0].toMap()["kind"].toString(), QString("course"));
        QCOMPARE(model.entries(1, 0).size(), 3);
        activities.setClaimed(ActivityModel::activityKey(a), true);
        QCOMPARE(model.cards(1, 0)[0].toMap()["registrationCount"].toInt(), 0);
        QCOMPARE(model.cards(1, 0)[0].toMap()["eventCount"].toInt(), 2);
        activities.setClaimed(ActivityModel::activityKey(a), false);
        QCOMPARE(model.cards(1, 0)[0].toMap()["registrationCount"].toInt(), 2);
        activities.setClaimed(ActivityModel::activityKey(a), true);
        QVERIFY(model.setMemo(3, 1, 0, QStringLiteral("带好资料\n提前到场")).isEmpty());
        QVERIFY(model.setMemoAt(3, 1, 0, QStringLiteral("带好资料\n提前到场"), QStringLiteral("09:12:34")).isEmpty());
        QCOMPARE(model.memoTime(3, 1, 0), QStringLiteral("09:12:34"));
        QVERIFY(!model.setMemoAt(3, 1, 0, QStringLiteral("越界"), QStringLiteral("13:00:00")).isEmpty());
        QVERIFY(model.cards(1, 0)[0].toMap()["hasMemo"].toBool());
        activities.setRows({}); // a daily activity refresh must not touch user memos
        QCOMPARE(model.memo(3, 1, 0), QStringLiteral("带好资料\n提前到场"));
        QCOMPARE(model.cards(1, 0)[0].toMap()["eventCount"].toInt(), 1);
        activities.setRows({a, a});
        QVERIFY(activities.isClaimed(a));
        QCOMPARE(model.cards(1, 0)[0].toMap()["eventCount"].toInt(), 2);
        ActivityModel restoredClaims;
        restoredClaims.setClaimedKeys(activities.claimedKeys());
        QVERIFY(restoredClaims.isClaimed(a));
        restoredClaims.setClaimedSnapshots(activities.claimedSnapshots());
        QCOMPARE(restoredClaims.reminderActivities().size(), 1);
        QVERIFY(model.setMemo(3, 7, 5, "memo only").isEmpty());
        QCOMPARE(model.cards(7, 5).size(), 1);
        model.setWeek(4);
        QVERIFY(model.cards(7, 5).isEmpty());
        QVERIFY(model.memo(4, 1, 0).isEmpty());
        model.setWeek(3);
        model.applyImport(Campus::Schedule{});
        QCOMPARE(model.memo(3, 7, 5), QString("memo only"));
        ScheduleModel reloaded(&activities);
        reloaded.setStoragePath(dir.filePath("timetable.json"));
        QVERIFY(reloaded.load());
        QCOMPARE(reloaded.memo(3, 7, 5), QString("memo only"));
        QCOMPARE(reloaded.memoTime(3, 1, 0), QStringLiteral("09:12:34"));
        QVERIFY(reloaded.setMemo(3, 7, 5, " ").isEmpty());
        QVERIFY(reloaded.cards(7, 5).isEmpty());
        QVERIFY(!reloaded.setMemo(0, 1, 0, "invalid").isEmpty());
        model.setWeek(0);
        QCOMPARE(model.cards(1, 0)[0].toMap()["registrationCount"].toInt(), 0);
    }
    void failedSaveDoesNotDiscardMemo() {
        QTemporaryDir dir;
        ActivityModel activities;
        ScheduleModel model(&activities);
        QVERIFY(model.setMemo(1, 1, 0, "original").isEmpty());
        model.setStoragePath(dir.path()); // directory cannot be replaced by a JSON file
        QVERIFY(!model.setMemo(1, 1, 0, "replacement").isEmpty());
        QCOMPARE(model.memo(1, 1, 0), QString("original"));
    }
    void detectedRegistrationChangesBookmarks() {
        ActivityModel activities;
        ScheduleModel model(&activities);
        QVERIFY(model.setAnchor(3, 1, "2026-09-21").isEmpty());
        Campus::Activity a;
        a.name = QStringLiteral("报名活动");
        a.registration = "2026/09/21 08:45 - 2026/09/21 09:00";
        a.activityTime = "2026/09/21 09:00 - 2026/09/21 09:30";
        activities.setRows({a});
        const QString key = ActivityModel::activityKey(a);
        QCOMPARE(model.cards(1, 0)[0].toMap()["registrationCount"].toInt(), 1);
        activities.setDetectedRegistration(key, true);
        QCOMPARE(model.cards(1, 0)[0].toMap()["registrationCount"].toInt(), 0);
        QCOMPARE(model.cards(1, 0)[0].toMap()["eventCount"].toInt(), 1);
        ActivityModel restored;
        restored.setAutoClaimedKeys(activities.autoClaimedKeys());
        restored.setClaimedSnapshots(activities.claimedSnapshots());
        QVERIFY(restored.isClaimed(a));
        QCOMPARE(restored.reminderActivities().size(), 1);
        activities.setDetectedRegistration(key, false);
        QCOMPARE(model.cards(1, 0)[0].toMap()["registrationCount"].toInt(), 1);
        QCOMPARE(model.cards(1, 0)[0].toMap()["eventCount"].toInt(), 0);
        activities.setClaimed(key, true);
        activities.setDetectedRegistration(key, true);
        activities.setDetectedRegistration(key, false);
        QVERIFY(activities.isClaimed(a)); // a failed or cancelled auto check must preserve a manual mark
    }
    void previewModeAndExactMemoTimeSurviveImport() {
        QTemporaryDir dir;
        ActivityModel activities;
        ScheduleModel model(&activities);
        model.setStoragePath(dir.filePath("timetable.json"));
        QVERIFY(model.setAnchor(3, 1, "2026-09-21").isEmpty());
        Campus::Activity activity;
        activity.name = QStringLiteral("公开活动");
        activity.registration = "2026/09/21 08:45 - 2026/09/21 09:00";
        activity.activityTime = "2026/09/21 09:15 - 2026/09/21 09:40";
        activities.setRows({activity});
        QCOMPARE(model.tasksForDate("2026-09-21").size(), 1);
        model.setActivityReminderMode(1);
        QCOMPARE(model.cards(1, 0)[0].toMap()["registrationCount"].toInt(), 1);
        QCOMPARE(model.cards(1, 0)[0].toMap()["eventCount"].toInt(), 1);
        QVERIFY(model.setMemoAt(3, 1, 0, QStringLiteral("准备材料"), QStringLiteral("09:12:34")).isEmpty());
        const auto tasks = model.tasksForDate("2026-09-21");
        QCOMPARE(tasks.size(), 3);
        bool foundMemo = false;
        for (const QVariant &task : tasks) {
            const QVariantMap item = task.toMap();
            if (item.value("kind") == QLatin1String("memo")) {
                foundMemo = true;
                QCOMPARE(item.value("time").toString(), QStringLiteral("09:12:34"));
                QVERIFY(item.value("target").toString().endsWith(QStringLiteral("+08:00")));
            }
        }
        QVERIFY(foundMemo);
        model.applyImport(Campus::Schedule{});
        QCOMPARE(model.activityReminderMode(), 1);
        QCOMPARE(model.memoTime(3, 1, 0), QStringLiteral("09:12:34"));
        ScheduleModel restored(&activities);
        restored.setStoragePath(dir.filePath("timetable.json"));
        QVERIFY(restored.load());
        QCOMPARE(restored.activityReminderMode(), 1);
        QCOMPARE(restored.memo(3, 1, 0), QStringLiteral("准备材料"));
    }
};
QTEST_GUILESS_MAIN(ScheduleTests)
#include "tst_schedule.moc"
