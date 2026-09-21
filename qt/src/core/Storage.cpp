#include "Storage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QSaveFile>
#include <QStandardPaths>

namespace Campus {

QString dataDirectory()
{
    const QByteArray override = qgetenv("GDIPU_DATA_DIR");
    if (!override.isEmpty())
        return QDir::fromNativeSeparators(QString::fromLocal8Bit(override));
    const QString local = QString::fromLocal8Bit(qgetenv("LOCALAPPDATA"));
    if (!local.isEmpty())
        return QDir::fromNativeSeparators(local) + QStringLiteral("/GdipuActivityHelper");
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

bool writeJsonAtomically(const QString &path, const QJsonDocument &doc, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Compact));
    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

static bool readJson(const QString &path, QJsonDocument *doc, bool *exists, QString *error)
{
    QFile file(path);
    if (exists)
        *exists = file.exists();
    if (!file.exists())
        return true;
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    QJsonParseError parse;
    *doc = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError) {
        if (error)
            *error = parse.errorString();
        return false;
    }
    return true;
}

bool loadActivities(const QString &path, QList<Activity> *out, QString *error)
{
    out->clear();
    QJsonDocument doc;
    bool exists = false;
    if (!readJson(path, &doc, &exists, error))
        return false;
    if (!exists)
        return true;
    if (!doc.isArray()) {
        if (error)
            *error = QStringLiteral("not a JSON array");
        return false;
    }
    for (const QJsonValue &v : doc.array()) {
        const Activity a = Activity::fromJson(v.toObject());
        if (!isIgnored(a))
            out->append(a);
    }
    return true;
}

bool saveActivities(const QString &path, const QList<Activity> &rows, QString *error)
{
    QJsonArray array;
    for (const Activity &a : rows)
        array.append(a.toJson());
    return writeJsonAtomically(path, QJsonDocument(array), error);
}

bool loadSchedule(const QString &path, Schedule *out, bool *exists, QString *error)
{
    QJsonDocument doc;
    if (!readJson(path, &doc, exists, error))
        return false;
    if (exists && !*exists)
        return true;
    if (!doc.isObject()) {
        if (error)
            *error = QStringLiteral("not a JSON object");
        return false;
    }
    *out = Schedule::fromJson(doc.object());
    return true;
}

bool saveSchedule(const QString &path, const Schedule &schedule, QString *error)
{
    return writeJsonAtomically(path, QJsonDocument(schedule.toJson()), error);
}

} // namespace Campus
