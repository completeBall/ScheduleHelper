#pragma once

#include "Activity.h"
#include "Timetable.h"

#include <QJsonDocument>
#include <QString>

namespace Campus {

// %LOCALAPPDATA%\GdipuActivityHelper — same folder as the C# build so cached
// results and the timetable carry over. Override with GDIPU_DATA_DIR.
QString dataDirectory();

bool writeJsonAtomically(const QString &path, const QJsonDocument &doc, QString *error = nullptr);

// Missing file -> true with empty result. Corrupt file -> false with *error set.
bool loadActivities(const QString &path, QList<Activity> *out, QString *error = nullptr);
bool saveActivities(const QString &path, const QList<Activity> &rows, QString *error = nullptr);

bool loadSchedule(const QString &path, Schedule *out, bool *exists, QString *error = nullptr);
bool saveSchedule(const QString &path, const Schedule &schedule, QString *error = nullptr);

} // namespace Campus
