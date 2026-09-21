#include "WebBridge.h"

#include <QDeadlineTimer>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

WebBridge::WebBridge(const QString &name, QObject *parent)
    : QObject(parent)
    , m_name(name)
{
}

void WebBridge::setCurrentUrl(const QString &url)
{
    if (m_currentUrl == url)
        return;
    m_currentUrl = url;
    emit currentUrlChanged();
}

void WebBridge::setAttached(bool on)
{
    if (m_attached == on)
        return;
    m_attached = on;
    emit attachedChanged();
}

void WebBridge::checkCancel() const
{
    if (m_cancelCheck && m_cancelCheck())
        throw Cancelled{};
}

void WebBridge::pause(int ms)
{
    QDeadlineTimer deadline(ms);
    while (!deadline.hasExpired()) {
        checkCancel();
        QEventLoop loop;
        QTimer::singleShot(qMin<qint64>(40, qMax<qint64>(1, deadline.remainingTime())), &loop,
                           &QEventLoop::quit);
        loop.exec();
    }
    checkCancel();
}

void WebBridge::waitAttached(int timeoutMs)
{
    QDeadlineTimer deadline(timeoutMs);
    while (!m_attached) {
        if (deadline.hasExpired())
            throw Failure{QStringLiteral("内置浏览器尚未就绪，请稍后重试。")};
        pause(50);
    }
}

QJsonValue WebBridge::eval(const QString &expression, int timeoutMs)
{
    waitAttached();
    const int id = ++m_nextId;
    m_pending.insert(id, {});
    const QString script = QStringLiteral(
        "(()=>{%1\n;try{return JSON.stringify((%2)??null);}"
        "catch(e){return JSON.stringify({__error:String(e&&e.message||e)});}})()")
                               .arg(m_prelude, expression);
    emit runScript(id, script);

    QDeadlineTimer deadline(timeoutMs);
    try {
        while (!m_pending.value(id).done) {
            if (deadline.hasExpired()) {
                m_pending.remove(id);
                return QJsonValue(QJsonValue::Null);
            }
            pause(20);
        }
    } catch (...) {
        m_pending.remove(id);
        throw;
    }
    const QString json = m_pending.take(id).json;
    if (json.isEmpty())
        return QJsonValue(QJsonValue::Null);
    // JSON.stringify output is a complete JSON value; wrap it so scalars parse too.
    const QJsonDocument doc = QJsonDocument::fromJson(QStringLiteral("[%1]").arg(json).toUtf8());
    const QJsonValue v = doc.array().first();
    if (v.isObject() && v.toObject().contains(QLatin1String("__error")))
        return QJsonValue(QJsonValue::Null);
    return v;
}

void WebBridge::load(const QUrl &url, int timeoutMs)
{
    waitAttached();
    m_load = LoadState::Loading;
    m_loadError.clear();
    emit loadUrl(url);
    QDeadlineTimer deadline(timeoutMs);
    while (m_load == LoadState::Loading) {
        if (deadline.hasExpired()) {
            emit stopRequested();
            throw Failure{QStringLiteral("学校网页加载超时")};
        }
        pause(30);
    }
    if (m_load == LoadState::Failed)
        throw Failure{QStringLiteral("页面加载失败：%1").arg(m_loadError)};
}

void WebBridge::stopLoading()
{
    emit stopRequested();
}

void WebBridge::scriptResult(int id, const QVariant &result)
{
    auto it = m_pending.find(id);
    if (it == m_pending.end())
        return;
    it->done = true;
    it->json = result.isValid() && !result.isNull() ? result.toString() : QString();
}

void WebBridge::notifyLoad(int status, const QString &url, const QString &error)
{
    setCurrentUrl(url);
    // WebEngineView.LoadStatus: 0 started, 1 stopped, 2 succeeded, 3 failed
    if (m_load != LoadState::Loading)
        return;
    if (status == 2) {
        m_load = LoadState::Succeeded;
        emit loadFinished(true, QString());
    } else if (status == 3) {
        m_load = LoadState::Failed;
        m_loadError = error;
        emit loadFinished(false, error);
    }
}
