#include "Activity.h"

#include <QCollator>
#include <QLocale>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>
#include <limits>

namespace Campus {

namespace {

QString str(const QJsonObject &o, const char *key)
{
    const QJsonValue v = o.value(QLatin1String(key));
    if (v.isString())
        return v.toString();
    if (v.isDouble())
        return QString::number(v.toInt());
    return {};
}

void put(QJsonObject &o, const char *key, const QString &value)
{
    if (!value.isEmpty())
        o.insert(QLatin1String(key), value);
}

const QRegularExpression &dateRe()
{
    static const QRegularExpression re(
        QStringLiteral(R"((\d{4})[/-](\d{1,2})[/-](\d{1,2})\s+(\d{1,2}):(\d{2}))"));
    return re;
}

qint64 startKey(const Activity &a)
{
    const QDateTime t = parseTimestamp(registrationDates(a).start);
    return t.isValid() ? t.toMSecsSinceEpoch() : std::numeric_limits<qint64>::max();
}

qint64 endKey(const Activity &a)
{
    const QDateTime t = parseTimestamp(registrationDates(a).end);
    return t.isValid() ? t.toMSecsSinceEpoch() : std::numeric_limits<qint64>::max();
}

template <typename Key>
void sortBy(QList<Activity> &rows, Key key)
{
    QCollator collator(QLocale(QLocale::Chinese, QLocale::China));
    std::stable_sort(rows.begin(), rows.end(), [&](const Activity &a, const Activity &b) {
        const qint64 ka = key(a), kb = key(b);
        if (ka != kb)
            return ka < kb;
        return collator.compare(a.name, b.name) < 0;
    });
}

} // namespace

Activity Activity::fromJson(const QJsonObject &o)
{
    Activity a;
    a.id = str(o, "id");
    a.name = str(o, "name");
    a.activityTime = str(o, "activityTime");
    a.registration = str(o, "registration");
    a.place = str(o, "place");
    a.teacher = str(o, "teacher");
    a.capacity = str(o, "capacity");
    a.remaining = str(o, "remaining");
    a.registered = str(o, "registered");
    a.organizer = str(o, "organizer");
    a.type = str(o, "type");
    a.url = str(o, "url");
    a.collectedAt = str(o, "collectedAt");
    a.error = str(o, "error");
    a.page = o.contains(QLatin1String("page")) ? o.value(QLatin1String("page")).toInt(-1) : -1;
    a.index = o.contains(QLatin1String("index")) ? o.value(QLatin1String("index")).toInt(-1) : -1;
    return a;
}

QJsonObject Activity::toJson() const
{
    QJsonObject o;
    put(o, "id", id);
    put(o, "name", name);
    put(o, "activityTime", activityTime);
    put(o, "registration", registration);
    put(o, "place", place);
    put(o, "teacher", teacher);
    put(o, "capacity", capacity);
    put(o, "remaining", remaining);
    put(o, "registered", registered);
    put(o, "organizer", organizer);
    put(o, "type", type);
    put(o, "url", url);
    put(o, "collectedAt", collectedAt);
    put(o, "error", error);
    if (page >= 0)
        o.insert(QStringLiteral("page"), page);
    if (index >= 0)
        o.insert(QStringLiteral("index"), index);
    return o;
}

QTimeZone beijing()
{
    return QTimeZone(8 * 3600);
}

QString normalize(const QString &s)
{
    return s.simplified();
}

QDateTime parseTimestamp(const QString &text)
{
    const QRegularExpressionMatch m = dateRe().match(text);
    if (!m.hasMatch())
        return {};
    const QDate d(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
    const QTime t(m.captured(4).toInt(), m.captured(5).toInt());
    if (!d.isValid() || !t.isValid())
        return {};
    return QDateTime(d, t, beijing());
}

QDateTime nowInBeijing()
{
    return QDateTime::currentDateTimeUtc().toTimeZone(beijing());
}

QStringList dateMatches(const QString &text)
{
    QStringList result;
    const QString flat = normalize(text);
    auto it = dateRe().globalMatch(flat);
    while (it.hasNext())
        result << it.next().captured(0);
    return result;
}

RegistrationDates registrationDates(const Activity &a)
{
    const QStringList d = dateMatches(a.registration);
    return {d.value(0), d.value(1)};
}

QString status(const Activity &a, const QDateTime &now)
{
    if (!a.error.isEmpty())
        return Status::Failed;
    const RegistrationDates d = registrationDates(a);
    if (d.start.isEmpty() || d.end.isEmpty())
        return Status::Unknown;
    if (now < parseTimestamp(d.start))
        return Status::NotStarted;
    if (now >= parseTimestamp(d.end))
        return Status::Closed;
    if (a.remaining == QLatin1String("0"))
        return Status::Full;
    return Status::Open;
}

QString status(const Activity &a)
{
    return status(a, nowInBeijing());
}

bool isIgnored(const Activity &a)
{
    return a.name.contains(QStringLiteral("团日"), Qt::CaseInsensitive)
        || a.name.contains(QStringLiteral("班会"), Qt::CaseInsensitive)
        || a.type == QStringLiteral("班级活动");
}

QString safeUrl(const QString &value)
{
    const QUrl u(value, QUrl::StrictMode);
    if (!u.isValid() || u.scheme() != QLatin1String("https")
        || u.host() != QLatin1String("study.gdipu.edu.cn") || (u.port() != -1 && u.port() != 443)
        || u.path() != QLatin1String("/CloudPortal/CloudActivityDetail"))
        return {};
    return u.toString();
}

void sortByRegistrationStart(QList<Activity> &rows)
{
    sortBy(rows, startKey);
}

void sortByRegistrationEnd(QList<Activity> &rows)
{
    sortBy(rows, endKey);
}

} // namespace Campus
