#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTimeZone>

namespace Campus {

// One scraped activity. Field names match the persisted JSON format;
// existing last-results.json files load unchanged.
struct Activity {
    QString id;
    QString name;
    QString activityTime;   // "2026/09/22 09:30 ~ 2026/09/23 21:30"
    QString registration;   // "2026/09/21 10:30 —— 2026/09/21 21:30"
    QString place;
    QString teacher;        // list-page only; used to detect list reordering
    QString capacity;       // "不限" or digits
    QString remaining;      // "不限" or digits
    QString registered;
    QString organizer;
    QString type;
    QString url;
    QString collectedAt;
    QString error;
    int page = -1;          // list position; only meaningful while collecting
    int index = -1;

    static Activity fromJson(const QJsonObject &o);
    QJsonObject toJson() const;
};

struct RegistrationDates {
    QString start;
    QString end;
};

namespace Status {
inline const QString Failed = QStringLiteral("读取失败");
inline const QString Unknown = QStringLiteral("时间待确认");
inline const QString NotStarted = QStringLiteral("报名未开始");
inline const QString Closed = QStringLiteral("报名已截止");
inline const QString Full = QStringLiteral("名额已满");
inline const QString Open = QStringLiteral("报名时间内");
}

// School times are Beijing time (UTC+8, no DST) regardless of the local zone.
QTimeZone beijing();
QDateTime parseTimestamp(const QString &text);          // invalid QDateTime on failure
QDateTime nowInBeijing();

QStringList dateMatches(const QString &text);
RegistrationDates registrationDates(const Activity &a);
QString status(const Activity &a, const QDateTime &now);
QString status(const Activity &a);

// 团日 / 班会 in the name, or type == 班级活动.
bool isIgnored(const Activity &a);

// Only https://study.gdipu.edu.cn/CloudPortal/CloudActivityDetail links are kept.
QString safeUrl(const QString &value);

// Ascending registration start (missing last), then Chinese collation on name.
void sortByRegistrationStart(QList<Activity> &rows);
void sortByRegistrationEnd(QList<Activity> &rows);

QString normalize(const QString &s);

} // namespace Campus
