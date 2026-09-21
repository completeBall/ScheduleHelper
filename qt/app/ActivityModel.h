#pragma once

#include "core/Activity.h"

#include <QAbstractListModel>
#include <QTimer>
#include <QSet>
#include <QHash>

using Campus::Activity;

// All collected activities plus the search / filter / sort state of the table.
class ActivityModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filterChanged)
    Q_PROPERTY(QString statusFilter READ statusFilter WRITE setStatusFilter NOTIFY filterChanged)
    Q_PROPERTY(QString typeFilter READ typeFilter WRITE setTypeFilter NOTIFY filterChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY filterChanged)
    Q_PROPERTY(int total READ total NOTIFY summaryChanged)
    Q_PROPERTY(int notStarted READ notStarted NOTIFY summaryChanged)
    Q_PROPERTY(int open READ open NOTIFY summaryChanged)
    Q_PROPERTY(int limited READ limited NOTIFY summaryChanged)
    Q_PROPERTY(int shown READ shown NOTIFY summaryChanged)
    Q_PROPERTY(QStringList types READ types NOTIFY summaryChanged)
    Q_PROPERTY(QString collectedAt READ collectedAt NOTIFY summaryChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        OrganizerRole,
        TypeRole,
        StartRole,
        EndRole,
        PlaceRole,
        ActivityTimeRole,
        CapacityRole,
        RemainingRole,
        RegisteredRole,
        StatusRole,
        StatusKindRole,
        UrlRole,
        ErrorRole,
        KeyRole,
        ClaimedRole,
        AutoClaimedRole,
    };

    explicit ActivityModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QList<Activity> rows);
    static QString activityKey(const Activity &activity);
    bool isClaimed(const Activity &activity) const { return isClaimedKey(activityKey(activity)); }
    bool isAutoClaimed(const Activity &activity) const { return m_autoClaimed.contains(activityKey(activity)); }
    void setClaimedKeys(const QStringList &keys);
    QStringList claimedKeys() const { return m_claimed.values(); }
    void setAutoClaimedKeys(const QStringList &keys);
    QStringList autoClaimedKeys() const { return m_autoClaimed.values(); }
    void setClaimedSnapshots(const QList<Activity> &snapshots);
    QList<Activity> claimedSnapshots() const { return m_claimedSnapshots.values(); }
    QList<Activity> reminderActivities() const;
    Q_INVOKABLE void setClaimed(const QString &key, bool claimed);
    void setDetectedRegistration(const QString &key, bool registered);
    const QList<Activity> &all() const { return m_all; }
    QList<Activity> displayed() const { return m_shown; }

    QString searchText() const { return m_search; }
    QString statusFilter() const { return m_status; }
    QString typeFilter() const { return m_type; }
    int sortMode() const { return m_sort; }
    void setSearchText(const QString &v);
    void setStatusFilter(const QString &v);
    void setTypeFilter(const QString &v);
    void setSortMode(int v);

    int total() const { return m_all.size(); }
    int notStarted() const { return m_notStarted; }
    int open() const { return m_open; }
    int limited() const { return m_limited; }
    int shown() const { return m_shown.size(); }
    QStringList types() const { return m_types; }
    QString collectedAt() const;

    Q_INVOKABLE void clearFilters();

signals:
    void filterChanged();
    void summaryChanged();
    void claimedChanged();

private:
    void rebuild();
    QString statusOf(const Activity &a) const;
    static QString statusKind(const QString &status);
    bool isClaimedKey(const QString &key) const { return m_claimed.contains(key) || m_autoClaimed.contains(key); }

    QList<Activity> m_all;
    QList<Activity> m_shown;
    QStringList m_types;
    QString m_search;
    QString m_status;
    QString m_type;
    int m_sort = 0;
    int m_notStarted = 0;
    int m_open = 0;
    int m_limited = 0;
    QString m_signature;
    QTimer m_tick;
    QSet<QString> m_claimed;
    QSet<QString> m_autoClaimed;
    QHash<QString, Activity> m_claimedSnapshots;
};
