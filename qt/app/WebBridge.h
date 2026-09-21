#pragma once

#include <QHash>
#include <QJsonValue>
#include <QObject>
#include <QUrl>
#include <functional>

// Thrown by blocking helpers when the user pressed "stop".
struct Cancelled {};
// Thrown for any user-presentable failure.
struct Failure {
    QString message;
};

// One WebEngineView living in QML, driven from C++.
//
// QML forwards `runScript` to view.runJavaScript() and reports results and
// load events back. The blocking helpers below spin a nested event loop so the
// collection logic can be written as straight-line code (a direct port of the
// async/await flow in the C# build) while the UI stays responsive.
class WebBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentUrl READ currentUrl WRITE setCurrentUrl NOTIFY currentUrlChanged)
    Q_PROPERTY(bool attached READ attached WRITE setAttached NOTIFY attachedChanged)

public:
    explicit WebBridge(const QString &name, QObject *parent = nullptr);

    QString name() const { return m_name; }
    QString currentUrl() const { return m_currentUrl; }
    void setCurrentUrl(const QString &url);
    bool attached() const { return m_attached; }
    void setAttached(bool on);

    // Checked by every blocking helper; return true to abort with Cancelled.
    void setCancelCheck(std::function<bool()> check) { m_cancelCheck = std::move(check); }
    // JS source prepended to every evaluated expression.
    void setPrelude(const QString &js) { m_prelude = js; }

    // Spin the event loop for `ms` (throws Cancelled).
    void pause(int ms);
    // Wait until the QML view exists.
    void waitAttached(int timeoutMs = 15000);
    // Evaluate a JS *expression* in the page; result is JSON-decoded.
    // Returns null when the page is mid-navigation or the script fails.
    QJsonValue eval(const QString &expression, int timeoutMs = 8000);
    // Navigate and wait for the load to finish (throws Failure on error/timeout).
    void load(const QUrl &url, int timeoutMs = 30000);
    void stopLoading();
    // Fire-and-forget navigation (used by the UI, not by the scraper).
    Q_INVOKABLE void go(const QString &url) { emit loadUrl(QUrl(url)); }

    // Called from QML.
    Q_INVOKABLE void scriptResult(int id, const QVariant &result);
    Q_INVOKABLE void notifyLoad(int status, const QString &url, const QString &error);

signals:
    void runScript(int id, const QString &js);
    void loadUrl(const QUrl &url);
    void stopRequested();
    void currentUrlChanged();
    void attachedChanged();
    void loadFinished(bool ok, const QString &error);

private:
    void checkCancel() const;

    QString m_name;
    QString m_currentUrl;
    QString m_prelude;
    bool m_attached = false;
    int m_nextId = 0;
    struct Pending {
        bool done = false;
        QString json;
    };
    QHash<int, Pending> m_pending;
    std::function<bool()> m_cancelCheck;

    enum class LoadState { Idle, Loading, Succeeded, Failed };
    LoadState m_load = LoadState::Idle;
    QString m_loadError;
};
