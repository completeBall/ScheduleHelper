#pragma once

#include "ScheduleModel.h"
#include "WebBridge.h"

#include <QObject>

// Imports the personal timetable from the academic system through the embedded
// browser. Direct port of MainForm.ImportSchedule / ReadAcademic (Schedule.cs),
// including the rules that never overwrite a good timetable with a bad one.
class ScheduleImporter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

public:
    ScheduleImporter(WebBridge *academic, ScheduleModel *model, QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    QString message() const { return m_message; }

    static QString academicMainUrl();
    void setMainUrl(const QString &url) { m_mainUrl = url; }

    Q_INVOKABLE void import();       // returns immediately
    Q_INVOKABLE void stop() { m_cancel = true; }
    Q_INVOKABLE void openLogin();    // navigates the academic view to the portal

    // Blocking variant used by the self-test. Returns an empty string on success,
    // otherwise the user-visible failure message.
    QString runImport();
    void navigate(const QString &url);

signals:
    void busyChanged();
    void messageChanged();
    void loginRequired();
    void finished(bool ok, const QString &message);

private:
    void say(const QString &text);

    WebBridge *m_bridge;
    ScheduleModel *m_model;
    QString m_mainUrl;
    QString m_message;
    bool m_busy = false;
    bool m_cancel = false;
};
