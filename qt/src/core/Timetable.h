#pragma once

#include "Activity.h"

#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace Campus {

struct Period {
    QString group;   // 上午 / 下午 / 晚上
    QString label;   // 第1、2节
    QString time;    // 08:30–09:55
};

const QList<Period> &periods();
const QStringList &dayNames();     // 星期一 … 星期日

struct Course {
    QString id;                    // only for manual courses
    QString name;
    QString raw;
    int day = 1;                   // 1 = Monday
    QList<int> sections;           // 1..12
    QList<int> weeks;
    bool hasWeeks = false;         // JSON null when false ("周次未识别")
    bool manual = false;

    static Course fromJson(const QJsonObject &o);
    QJsonObject toJson() const;
    bool inWeek(int week) const { return week <= 0 || !hasWeeks || weeks.contains(week); }
    bool inBlock(int block) const;  // block 0..5
};

// The document persisted to timetable.json.
struct Schedule {
    QList<Course> courses;
    QList<Course> manualCourses;
    QJsonObject memos;             // week/day/block -> {text,time}, legacy strings still load
    int activityReminderMode = 0;  // 0: registered event only; 1: preview all event times
    QString semester;
    QString source;
    int unknownRows = 0;
    QString weekOneMonday;         // yyyy-MM-dd, empty when not set
    QString selectedWeek;          // "" = all weeks
    QString importedAt;

    static Schedule fromJson(const QJsonObject &o);
    QJsonObject toJson() const;
    QList<Course> allCourses() const { return courses + manualCourses; }
};

// Week list from text such as "1-6(单周)", "2-8周(双)", "1-3,5,7-8(周)".
// Returns false when no week expression is found.
bool weekNumbers(const QString &raw, QList<int> *out);

// Class slots (1..12) from a row label: "第1、2节", "[03-04]节", "第一大节", "03-04".
QList<int> slotsFrom(const QString &text);

QString clean(const QString &s);

// Date algebra. Dates are yyyy-MM-dd strings.
QString weekOneMonday(int referenceWeek, int referenceDay, const QString &referenceDate);
QString dateFor(const QString &anchor, int week, int day);

struct Reminder {
    enum Kind { Registration, Event };
    Kind kind = Registration;
    const Activity *activity = nullptr;
    int week = 0;
    int day = 0;      // 1..7
    int block = 0;    // 0..5
    QString start;    // yyyy-MM-dd HH:mm
};

// Maps the first date found in `text` onto (week, day, block) relative to the anchor.
bool reminderFor(const QString &text, const QString &anchor, Reminder *out);
QList<Reminder> activityMoments(const Activity &a, const QString &anchor);

// Manual-course week input: "2", "2-6", "1-8单周". Returns false if unparsable.
bool parseManualWeeks(const QString &input, QList<int> *out);

} // namespace Campus
