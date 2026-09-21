#pragma once

#include "ActivityModel.h"
#include "WebBridge.h"

#include <QJsonObject>
#include <QObject>
#include <functional>

struct TimeoutFailure : Failure {};

// Collects every "未开始" activity from the school portal through the embedded
// browser.
class Scraper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int done READ done NOTIFY progressChanged)
    Q_PROPERTY(int expected READ expected NOTIFY progressChanged)
    Q_PROPERTY(QString message READ message NOTIFY progressChanged)

public:
    Scraper(WebBridge *school, ActivityModel *model, QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    int done() const { return m_done; }
    int expected() const { return m_expected; }
    QString message() const { return m_message; }

    void setBaseUrl(const QString &url) { m_baseUrl = url; }
    void setResultsPath(const QString &path) { m_resultsPath = path; }   // empty = do not persist

    Q_INVOKABLE void collect();   // returns immediately; runs on the event loop
    Q_INVOKABLE void stop();

    // Blocking variant (used by the self-test). Returns the final message.
    QString runCollection();
    bool lastRunFailed() const { return m_lastFailed; }

signals:
    void busyChanged();
    void progressChanged();
    void loginRequired();
    void finished(bool ok, const QString &message);

private:
    void progress(const QString &text);
    void checkLogin();
    QJsonObject until(const QString &expression, const std::function<bool(const QJsonObject &)> &ready,
                      const QString &description, int stableMs = 700, int timeoutMs = 25000);
    QJsonObject openList();
    QJsonObject nextPage(const QJsonObject &current);
    void save();

    WebBridge *m_bridge;
    ActivityModel *m_model;
    QString m_baseUrl = QStringLiteral("https://study.gdipu.edu.cn");
    QString m_resultsPath;
    QList<Activity> m_rows;
    bool m_busy = false;
    bool m_cancel = false;
    bool m_lastFailed = false;
    int m_done = 0;
    int m_expected = 0;
    QString m_message;
};
