#include "ScheduleModel.h"

#include "core/Storage.h"

#include <QDate>
#include <QDateTime>
#include <QUuid>
#include <QUrl>
#include <QVariantMap>

using namespace Campus;

ScheduleModel::ScheduleModel(ActivityModel *activities, QObject *parent)
    : QObject(parent)
    , m_activities(activities)
{
    // Reminder cards follow the activity list.
    connect(m_activities, &ActivityModel::summaryChanged, this, [this] { changed(); });
}

QVariantList ScheduleModel::periodList() const
{
    QVariantList out;
    for (const Period &p : periods())
        out << QVariantMap{{QStringLiteral("group"), p.group},
                           {QStringLiteral("label"), p.label},
                           {QStringLiteral("time"), p.time}};
    return out;
}

void ScheduleModel::changed()
{
    ++m_revision;
    emit revisionChanged();
}

void ScheduleModel::setWeek(int week)
{
    week = qBound(0, week, 30);
    if (m_week == week)
        return;
    m_week = week;
    m_data.selectedWeek = week > 0 ? QString::number(week) : QString();
    emit weekChanged();
    changed();
    save();
}

QString ScheduleModel::importedAt() const
{
    if (m_data.importedAt.isEmpty())
        return {};
    const QDateTime t = QDateTime::fromString(m_data.importedAt, Qt::ISODateWithMs).toTimeZone(beijing());
    return t.isValid() ? t.toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QString();
}

QString ScheduleModel::summary() const
{
    int courses = 0;
    for (const Course &c : m_data.allCourses())
        if (c.inWeek(m_week))
            ++courses;
    int reminders = 0;
    if (m_week > 0 && hasAnchor())
        for (const Activity &a : m_activities->reminderActivities())
            for (const Reminder &r : activityMoments(a, m_data.weekOneMonday))
                if (r.week == m_week && (r.kind == Reminder::Event) == m_activities->isClaimed(a))
                    ++reminders;
    QString text = QStringLiteral("%1 条课程安排").arg(courses);
    if (reminders > 0)
        text += QStringLiteral(" · %1 个活动提醒").arg(reminders);
    return text;
}

bool ScheduleModel::load(QString *error)
{
    Schedule s;
    bool exists = false;
    if (!loadSchedule(m_path, &s, &exists, error))
        return false;
    if (exists) {
        m_data = s;
        m_week = m_data.selectedWeek.toInt();
        emit weekChanged();
        changed();
    }
    return true;
}

void ScheduleModel::save()
{
    if (m_path.isEmpty())
        return;
    QString err;
    if (!saveSchedule(m_path, m_data, &err))
        emit persistFailed(err);
}

void ScheduleModel::applyImport(Schedule imported)
{
    imported.manualCourses = m_data.manualCourses;
    imported.memos = m_data.memos;
    imported.weekOneMonday = m_data.weekOneMonday;
    imported.selectedWeek = m_data.selectedWeek;
    imported.importedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    const QUrl source(imported.source);
    if (source.isValid() && !source.scheme().isEmpty())
        imported.source = source.adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment).toString();
    m_data = imported;
    save();
    changed();
}

int ScheduleModel::colorIndex(const QString &name)
{
    quint32 hash = 0;
    for (const QChar c : name)
        hash = hash * 31u + c.unicode();
    return int(hash % 5u);
}

QString ScheduleModel::dayDate(int day) const
{
    if (m_week <= 0 || !hasAnchor())
        return {};
    const QString full = dateFor(m_data.weekOneMonday, m_week, day);
    return full.mid(5).replace(QLatin1Char('-'), QLatin1Char('/'));
}

QVariantList ScheduleModel::entries(int day, int block) const
{
    QVariantList out;
    for (const Course &c : m_data.allCourses()) {
        if (c.day != day || !c.inBlock(block) || !c.inWeek(m_week))
            continue;
        QString detail = c.raw.startsWith(c.name) ? c.raw.mid(c.name.size()).trimmed() : c.raw;
        QVariantMap m;
        m[QStringLiteral("kind")] = QStringLiteral("course");
        m[QStringLiteral("title")] = c.name;
        m[QStringLiteral("detail")] = detail;
        m[QStringLiteral("full")] = c.raw;
        m[QStringLiteral("color")] = colorIndex(c.name);
        m[QStringLiteral("manual")] = c.manual;
        m[QStringLiteral("id")] = c.id;
        m[QStringLiteral("weeksUnknown")] = !c.manual && !c.hasWeeks;
        out << m;
    }
    if (m_week > 0 && hasAnchor()) {
        for (const Activity &a : m_activities->reminderActivities()) {
            for (const Reminder &r : activityMoments(a, m_data.weekOneMonday)) {
                if (r.week != m_week || r.day != day || r.block != block)
                    continue;
                if ((r.kind == Reminder::Event) != m_activities->isClaimed(a))
                    continue;
                const bool registration = r.kind == Reminder::Registration;
                const QString what = registration ? QStringLiteral("报名时间：") : QStringLiteral("活动时间：");
                QStringList full{a.name, what + r.start};
                if (!a.place.isEmpty())
                    full << QStringLiteral("地点：") + a.place;
                QVariantMap m;
                m[QStringLiteral("kind")] = registration ? QStringLiteral("registration") : QStringLiteral("event");
                // card face: "报名 10:30" over the activity name; everything else lives in the detail dialog
                m[QStringLiteral("label")] = (registration ? QStringLiteral("报名 ") : QStringLiteral("开始 ")) + r.start.mid(11);
                m[QStringLiteral("title")] = a.name;
                m[QStringLiteral("detail")] = QString();
                m[QStringLiteral("full")] = full.join(QLatin1Char('\n'));
                m[QStringLiteral("dialogTitle")] = registration ? QStringLiteral("活动报名提醒") : QStringLiteral("活动开始提醒");
                m[QStringLiteral("url")] = safeUrl(a.url);
                out << m;
            }
        }
    }
    return out;
}

QVariantList ScheduleModel::cards(int day, int block) const
{
    QVariantList out;
    int registration = 0, event = 0;
    for (const QVariant &entry : entries(day, block)) {
        const QVariantMap item = entry.toMap();
        const QString kind = item.value(QStringLiteral("kind")).toString();
        if (kind == QLatin1String("course")) out << item;
        else if (kind == QLatin1String("registration")) ++registration;
        else if (kind == QLatin1String("event")) ++event;
    }
    const bool hasMemo = !memo(m_week, day, block).isEmpty();
    if (out.isEmpty() && (registration || event || hasMemo))
        out << QVariantMap{{QStringLiteral("kind"), QStringLiteral("empty")},
                           {QStringLiteral("title"), QStringLiteral("空闲时段")},
                           {QStringLiteral("detail"), QStringLiteral("点击查看提醒或备忘录")}};
    // One set of bookmarks per time slot, even when multiple courses overlap.
    if (!out.isEmpty()) {
        QVariantMap first = out.first().toMap();
        first[QStringLiteral("registrationCount")] = registration;
        first[QStringLiteral("eventCount")] = event;
        first[QStringLiteral("hasMemo")] = hasMemo;
        out[0] = first;
    }
    return out;
}

QString ScheduleModel::memo(int week, int day, int block) const
{
    return m_data.memos.value(QStringLiteral("%1/%2/%3").arg(week).arg(day).arg(block)).toString();
}

QString ScheduleModel::setMemo(int week, int day, int block, const QString &text)
{
    if (week < 1 || week > 30 || day < 1 || day > 7 || block < 0 || block > 5)
        return QStringLiteral("请先选择具体周次和有效时段。");
    Schedule updated = m_data;
    const QString key = QStringLiteral("%1/%2/%3").arg(week).arg(day).arg(block);
    if (text.trimmed().isEmpty()) updated.memos.remove(key);
    else updated.memos.insert(key, text.trimmed());
    QString error;
    if (!m_path.isEmpty() && !saveSchedule(m_path, updated, &error))
        return QStringLiteral("备忘录保存失败：") + error;
    m_data = updated;
    changed();
    return {};
}

QString ScheduleModel::dateOf(int week, int day) const
{
    return dateFor(m_data.weekOneMonday, week, day);
}

QString ScheduleModel::setAnchor(int week, int day, const QString &date)
{
    const QString anchor = Campus::weekOneMonday(week, day, date);
    if (anchor.isEmpty())
        return QStringLiteral("请填写有效的周次和日期。");
    m_data.weekOneMonday = anchor;
    m_week = week;
    m_data.selectedWeek = QString::number(week);
    emit weekChanged();
    save();
    changed();
    return {};
}

QString ScheduleModel::addManualCourse(const QString &nameIn, const QString &teacherIn, const QString &placeIn,
                                       int day, int block, const QString &weeksText)
{
    const QString name = clean(nameIn);
    if (name.isEmpty())
        return QStringLiteral("请填写课程名称。");
    QList<int> weeks;
    if (!parseManualWeeks(weeksText, &weeks))
        return QStringLiteral("周次格式无法识别，请填写如 2、2-6 或 1-8单周。");
    const QString teacher = clean(teacherIn), place = clean(placeIn), weekInput = clean(weeksText);
    QStringList details;
    for (const QString &d : {teacher, weekInput + QStringLiteral("周"), place})
        if (!d.isEmpty())
            details << d;
    Course c;
    c.manual = true;
    c.id = QStringLiteral("manual-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    c.name = name;
    c.raw = (QStringList{name} + details).join(QLatin1Char('\n'));
    c.day = day;
    c.sections = {block * 2 + 1, block * 2 + 2};
    c.hasWeeks = true;
    c.weeks = weeks;
    m_data.manualCourses << c;
    save();
    changed();
    return {};
}

void ScheduleModel::removeManualCourse(const QString &id)
{
    for (int i = 0; i < m_data.manualCourses.size(); ++i) {
        if (m_data.manualCourses[i].id == id) {
            m_data.manualCourses.removeAt(i);
            save();
            changed();
            return;
        }
    }
}
