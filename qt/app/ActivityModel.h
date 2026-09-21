#pragma once

#include "core/Activity.h"

#include <QAbstractListModel>
#include <QTimer>

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
    };

    explicit ActivityModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QList<Activity> rows);
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

private:
    void rebuild();
    QString statusOf(const Activity &a) const;
    static QString statusKind(const QString &status);

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
};
