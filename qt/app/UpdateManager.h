#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>

class QNetworkReply;

// Checks the official GitHub release and replaces a deployed Windows ZIP after exit.
class UpdateManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(bool showResult READ showResult NOTIFY changed)

public:
    explicit UpdateManager(const QString &dataDir, bool enabled, QObject *parent = nullptr);

    QString currentVersion() const;
    QString latestVersion() const { return m_latestVersion; }
    bool available() const { return m_available; }
    bool busy() const { return m_busy; }
    int progress() const { return m_progress; }
    QString message() const { return m_message; }
    bool showResult() const { return m_showResult; }

    Q_INVOKABLE void check();
    Q_INVOKABLE void checkNow() { m_showResult = true; emit changed(); check(); }
    Q_INVOKABLE void install();

signals:
    void changed();

private:
    void finishDownload(QNetworkReply *reply);
    void fail(const QString &message);
    bool launchInstaller();
    void requestRelease(const QUrl &url, bool fallback);

    QString m_dataDir;
    QString m_latestVersion;
    QString m_digest;
    QString m_message;
    QUrl m_assetUrl;
    qint64 m_assetSize = 0;
    bool m_available = false;
    bool m_busy = false;
    bool m_showResult = false;
    int m_progress = 0;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QFile m_archive;
    QTimer m_periodic;
};
