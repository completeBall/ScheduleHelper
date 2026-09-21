#include "ActivityModel.h"

#include <QRegularExpression>
#include <QCryptographicHash>
#include <algorithm>

using namespace Campus;

namespace {

// "2026/09/21 10:30" -> "09-21 10:30"
QString shortStamp(const QString &stamp)
{
    if (stamp.isEmpty())
        return {};
    const QDateTime t = parseTimestamp(stamp);
    if (!t.isValid())
        return stamp;
    return t.toString(QStringLiteral("MM-dd HH:mm"));
}

} // namespace

ActivityModel::ActivityModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Statuses depend on the clock; re-evaluate once a minute like the old page did.
    m_tick.setInterval(60 * 1000);
    connect(&m_tick, &QTimer::timeout, this, [this] {
        QString sig;
        for (const Activity &a : std::as_const(m_all))
            sig += statusOf(a).at(0);
        if (sig != m_signature)
            rebuild();
    });
    m_tick.start();
}

int ActivityModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_shown.size());
}

QString ActivityModel::statusOf(const Activity &a) const
{
    return status(a);
}

QString ActivityModel::statusKind(const QString &s)
{
    if (s == Status::Open)
        return QStringLiteral("open");
    if (s == Status::Failed)
        return QStringLiteral("error");
    if (s == Status::Closed || s == Status::Full)
        return QStringLiteral("closed");
    if (s == Status::NotStarted)
        return QStringLiteral("soon");
    return QStringLiteral("neutral");
}

QVariant ActivityModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_shown.size())
        return {};
    const Activity &a = m_shown.at(index.row());
    switch (role) {
    case NameRole: return a.name;
    case OrganizerRole: return a.organizer;
    case TypeRole: return a.type;
    case StartRole: return shortStamp(registrationDates(a).start);
    case EndRole: return shortStamp(registrationDates(a).end);
    case PlaceRole: return a.place;
    case ActivityTimeRole: return a.activityTime;
    case CapacityRole: return a.capacity;
    case RemainingRole: return a.remaining;
    case RegisteredRole: return a.registered;
    case StatusRole: return statusOf(a);
    case StatusKindRole: return statusKind(statusOf(a));
    case UrlRole: return safeUrl(a.url);
    case ErrorRole: return a.error;
    case KeyRole: return activityKey(a);
    case ClaimedRole: return isClaimed(a);
    case AutoClaimedRole: return isAutoClaimed(a);
    }
    return {};
}

QHash<int, QByteArray> ActivityModel::roleNames() const
{
    return {
        {NameRole, "name"},           {OrganizerRole, "organizer"},   {TypeRole, "type"},
        {StartRole, "regStart"},      {EndRole, "regEnd"},            {PlaceRole, "place"},
        {ActivityTimeRole, "activityTime"}, {CapacityRole, "capacity"}, {RemainingRole, "remaining"},
        {RegisteredRole, "registered"}, {StatusRole, "status"},       {StatusKindRole, "statusKind"},
        {UrlRole, "url"},             {ErrorRole, "error"},
        {KeyRole, "activityKey"},    {ClaimedRole, "claimed"}, {AutoClaimedRole, "autoClaimed"},
    };
}

QString ActivityModel::activityKey(const Activity &a)
{
    const QString identity = !a.url.isEmpty() ? a.url :
        (!a.id.isEmpty() ? a.id : a.name + QLatin1Char('|') + a.activityTime);
    return QString::fromLatin1(QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex());
}

void ActivityModel::setClaimedKeys(const QStringList &keys)
{
    m_claimed = QSet<QString>(keys.cbegin(), keys.cend());
    rebuild();
}

void ActivityModel::setAutoClaimedKeys(const QStringList &keys)
{
    m_autoClaimed = QSet<QString>(keys.cbegin(), keys.cend());
    rebuild();
}

void ActivityModel::setClaimedSnapshots(const QList<Activity> &snapshots)
{
    m_claimedSnapshots.clear();
    for (const Activity &activity : snapshots) {
        const QString key = activityKey(activity);
        if (isClaimedKey(key)) m_claimedSnapshots.insert(key, activity);
    }
    emit summaryChanged();
}

QList<Activity> ActivityModel::reminderActivities() const
{
    QList<Activity> result = m_all;
    QSet<QString> current;
    for (const Activity &activity : m_all) current.insert(activityKey(activity));
    for (auto it = m_claimedSnapshots.cbegin(); it != m_claimedSnapshots.cend(); ++it)
        if (isClaimedKey(it.key()) && !current.contains(it.key())) result << it.value();
    return result;
}

void ActivityModel::setClaimed(const QString &key, bool claimed)
{
    if (key.isEmpty() || m_claimed.contains(key) == claimed)
        return;
    if (claimed) {
        m_claimed.insert(key);
        for (const Activity &activity : m_all)
            if (activityKey(activity) == key) { m_claimedSnapshots.insert(key, activity); break; }
    } else {
        m_claimed.remove(key);
        if (!isClaimedKey(key)) m_claimedSnapshots.remove(key);
    }
    rebuild();
    emit claimedChanged();
}

void ActivityModel::setDetectedRegistration(const QString &key, bool registered)
{
    if (key.isEmpty() || m_autoClaimed.contains(key) == registered)
        return;
    if (registered) {
        m_autoClaimed.insert(key);
        for (const Activity &activity : m_all)
            if (activityKey(activity) == key) { m_claimedSnapshots.insert(key, activity); break; }
    } else {
        m_autoClaimed.remove(key);
        if (!isClaimedKey(key)) m_claimedSnapshots.remove(key);
    }
    rebuild();
    emit claimedChanged();
}

void ActivityModel::setRows(QList<Activity> rows)
{
    m_all = std::move(rows);
    for (const Activity &activity : m_all) {
        const QString key = activityKey(activity);
        if (isClaimedKey(key)) m_claimedSnapshots.insert(key, activity);
    }
    QStringList types;
    for (const Activity &a : std::as_const(m_all))
        if (!a.type.isEmpty() && !types.contains(a.type))
            types << a.type;
    types.sort();
    m_types = types;
    if (!m_type.isEmpty() && !m_types.contains(m_type)) {
        m_type.clear();
        emit filterChanged();
    }
    rebuild();
}

void ActivityModel::rebuild()
{
    const QDateTime now = nowInBeijing();
    m_notStarted = m_open = m_limited = 0;
    m_signature.clear();
    static const QRegularExpression digits(QStringLiteral("^\\d+$"));
    for (const Activity &a : std::as_const(m_all)) {
        const QString s = status(a, now);
        m_signature += s.at(0);
        if (s == Status::NotStarted)
            ++m_notStarted;
        else if (s == Status::Open)
            ++m_open;
        if (digits.match(a.capacity).hasMatch())
            ++m_limited;
    }

    QList<Activity> list = m_all;
    if (m_sort == 1)
        sortByRegistrationEnd(list);
    else
        sortByRegistrationStart(list);

    const QString q = m_search.trimmed().toLower();
    QList<Activity> shown;
    for (const Activity &a : std::as_const(list)) {
        if (!q.isEmpty() && !QStringList{a.name, a.place, a.organizer}.join(QLatin1Char(' ')).toLower().contains(q))
            continue;
        if (!m_status.isEmpty() && status(a, now) != m_status)
            continue;
        if (!m_type.isEmpty() && a.type != m_type)
            continue;
        shown << a;
    }

    beginResetModel();
    m_shown = std::move(shown);
    endResetModel();
    emit summaryChanged();
}

void ActivityModel::setSearchText(const QString &v)
{
    if (m_search == v)
        return;
    m_search = v;
    emit filterChanged();
    rebuild();
}

void ActivityModel::setStatusFilter(const QString &v)
{
    if (m_status == v)
        return;
    m_status = v;
    emit filterChanged();
    rebuild();
}

void ActivityModel::setTypeFilter(const QString &v)
{
    if (m_type == v)
        return;
    m_type = v;
    emit filterChanged();
    rebuild();
}

void ActivityModel::setSortMode(int v)
{
    if (m_sort == v)
        return;
    m_sort = v;
    emit filterChanged();
    rebuild();
}

void ActivityModel::clearFilters()
{
    m_search.clear();
    m_status.clear();
    m_type.clear();
    emit filterChanged();
    rebuild();
}

QString ActivityModel::collectedAt() const
{
    QString earliest;
    for (const Activity &a : m_all)
        if (!a.collectedAt.isEmpty() && (earliest.isEmpty() || a.collectedAt < earliest))
            earliest = a.collectedAt;
    if (earliest.isEmpty())
        return {};
    const QDateTime t = QDateTime::fromString(earliest, Qt::ISODateWithMs).toTimeZone(beijing());
    return t.isValid() ? t.toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QString();
}
