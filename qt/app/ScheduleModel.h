#pragma once

#include "ActivityModel.h"
#include "core/Timetable.h"

#include <QObject>
#include <QVariantList>

// The timetable the QML grid renders: imported courses, manual courses and the
// activity bookmarks derived from the activity list, plus per-slot memos.
class ScheduleModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int week READ week WRITE setWeek NOTIFY weekChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(QString semester READ semester NOTIFY revisionChanged)
    Q_PROPERTY(QString importedAt READ importedAt NOTIFY revisionChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY revisionChanged)
    Q_PROPERTY(bool hasCourses READ hasCourses NOTIFY revisionChanged)
    Q_PROPERTY(bool hasAnchor READ hasAnchor NOTIFY revisionChanged)
    Q_PROPERTY(QString weekOneMonday READ weekOneMonday NOTIFY revisionChanged)
    Q_PROPERTY(QStringList dayNames READ dayNames CONSTANT)
    Q_PROPERTY(QVariantList periods READ periodList CONSTANT)

public:
    explicit ScheduleModel(ActivityModel *activities, QObject *parent = nullptr);

    int week() const { return m_week; }
    void setWeek(int week);
    int revision() const { return m_revision; }
    QString semester() const { return m_data.semester; }
    QString importedAt() const;
    QString summary() const;
    bool hasCourses() const { return !m_data.courses.isEmpty() || !m_data.manualCourses.isEmpty(); }
    bool hasAnchor() const { return !m_data.weekOneMonday.isEmpty(); }
    QString weekOneMonday() const { return m_data.weekOneMonday; }
    QStringList dayNames() const { return Campus::dayNames(); }
    QVariantList periodList() const;

    void setStoragePath(const QString &path) { m_path = path; }   // empty = do not persist
    bool load(QString *error = nullptr);
    const Campus::Schedule &data() const { return m_data; }

    // Replace the imported courses but keep the user's manual courses, anchor date and week.
    void applyImport(Campus::Schedule imported);

    Q_INVOKABLE QVariantList cards(int day, int block) const;
    Q_INVOKABLE QVariantList entries(int day, int block) const;
    Q_INVOKABLE QString memo(int week, int day, int block) const;
    Q_INVOKABLE QString setMemo(int week, int day, int block, const QString &text);
    Q_INVOKABLE QString dayDate(int day) const;               // "09/23" or ""
    Q_INVOKABLE QString dateOf(int week, int day) const;      // yyyy-MM-dd, "" without an anchor
    Q_INVOKABLE QString setAnchor(int week, int day, const QString &date);   // "" on success
    Q_INVOKABLE QString addManualCourse(const QString &name, const QString &teacher, const QString &place,
                                        int day, int block, const QString &weeksText);
    Q_INVOKABLE void removeManualCourse(const QString &id);

signals:
    void weekChanged();
    void revisionChanged();
    void persistFailed(const QString &error);

private:
    void changed();
    void save();
    static int colorIndex(const QString &name);

    ActivityModel *m_activities;
    Campus::Schedule m_data;
    QString m_path;
    int m_week = 0;
    int m_revision = 0;
};
