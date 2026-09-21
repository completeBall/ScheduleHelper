#include "Scraper.h"

#include "core/Storage.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QTimer>

using namespace Campus;

namespace {

const char *kListExpression = R"JS(({url:location.href,ready:document.readyState,cards:Campus.extractCards(document),total:Number(document.querySelector('.el-pagination__total')?.textContent.match(/\d+/)?.[0]??-1),page:Number(document.querySelector('.el-pager .active')?.textContent||1),pending:[...document.querySelectorAll('.tag-radio-item.active')].some(e=>e.textContent.trim()==='未开始')}))JS";

int num(const QJsonObject &o, const char *key)
{
    const QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toInt(-1) : -1;
}

QList<Activity> cardsOf(const QJsonObject &o)
{
    QList<Activity> out;
    for (const QJsonValue &v : o.value(QLatin1String("cards")).toArray())
        out << Activity::fromJson(v.toObject());
    return out;
}

QByteArray cardsKey(const QJsonObject &o)
{
    return QJsonDocument(o.value(QLatin1String("cards")).toArray()).toJson(QJsonDocument::Compact);
}

QString rowKey(const Activity &a)
{
    return a.name + QLatin1Char('|') + a.teacher + QLatin1Char('|') + a.activityTime + QLatin1Char('|') + a.place;
}

QString nowIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

} // namespace

Scraper::Scraper(WebBridge *school, ActivityModel *model, QObject *parent)
    : QObject(parent)
    , m_bridge(school)
    , m_model(model)
{
}

void Scraper::progress(const QString &text)
{
    m_message = text;
    emit progressChanged();
}

void Scraper::stop()
{
    m_cancel = true;
}

void Scraper::save()
{
    if (m_resultsPath.isEmpty())
        return;
    QString err;
    saveActivities(m_resultsPath, m_rows, &err);
}

void Scraper::checkLogin()
{
    const QString url = m_bridge->currentUrl();
    if (url.contains(QLatin1String("/authserver/")) || url.contains(QLatin1String("/login"))) {
        emit loginRequired();
        throw Failure{QStringLiteral("需要登录：请在“学校登录”页完成认证，然后再次点击“开始采集”。")};
    }
}

QJsonObject Scraper::until(const QString &expression, const std::function<bool(const QJsonObject &)> &ready,
                           const QString &description, int stableMs, int timeoutMs)
{
    QElapsedTimer total, since;
    total.start();
    since.start();
    QByteArray last;
    while (total.elapsed() < timeoutMs) {
        checkLogin();
        const QJsonValue v = m_bridge->eval(expression);
        if (v.isObject() && ready(v.toObject())) {
            const QByteArray key = QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact);
            if (key != last) {
                last = key;
                since.restart();
            } else if (since.elapsed() >= stableMs) {
                return v.toObject();
            }
        } else {
            last.clear();
            since.restart();
        }
        m_bridge->pause(200);
    }
    throw TimeoutFailure{{description + QStringLiteral("超时。请确认学校页面能正常打开后重试。")}};
}

QJsonObject Scraper::openList()
{
    // A fresh top-level navigation avoids iframe / CSP restrictions and stale SPA state.
    m_bridge->load(QUrl(m_baseUrl + QStringLiteral("/CloudPortal/CloudSquare")));
    until(QString::fromUtf8(kListExpression),
          [](const QJsonObject &r) { return num(r, "total") >= 0 && (num(r, "total") == 0 || !cardsOf(r).isEmpty()); },
          QStringLiteral("活动列表"), 1200);
    const QJsonValue clicked = m_bridge->eval(QStringLiteral(
        "(()=>{const e=[...document.querySelectorAll('.tag-radio-item')].find(e=>e.textContent.trim()==='未开始');if(!e)return false;e.click();return true;})()"));
    if (!clicked.toBool())
        throw Failure{QStringLiteral("未找到“未开始”筛选，请确认当前为学校活动广场。")};
    return until(QString::fromUtf8(kListExpression),
                 [](const QJsonObject &r) {
                     return r.value(QLatin1String("pending")).toBool() && num(r, "total") >= 0
                         && (num(r, "total") == 0 || !cardsOf(r).isEmpty());
                 },
                 QStringLiteral("未开始活动"), 1200);
}

QJsonObject Scraper::nextPage(const QJsonObject &current)
{
    const int page = num(current, "page");
    const QByteArray old = cardsKey(current);
    const QJsonValue clicked = m_bridge->eval(QStringLiteral(
        "(()=>{const b=document.querySelector('.el-pagination .btn-next');if(!b||b.disabled)return false;b.click();return true;})()"));
    if (!clicked.toBool())
        throw Failure{QStringLiteral("分页提前结束，请重新采集。")};
    return until(QString::fromUtf8(kListExpression),
                 [&](const QJsonObject &r) {
                     return num(r, "page") == page + 1 && !cardsOf(r).isEmpty() && cardsKey(r) != old;
                 },
                 QStringLiteral("下一页"));
}

void Scraper::collect()
{
    if (m_busy)
        return;
    QTimer::singleShot(0, this, [this] { runCollection(); });
}

QString Scraper::runCollection()
{
    if (m_busy)
        return {};
    m_busy = true;
    m_cancel = false;
    m_lastFailed = false;
    m_done = 0;
    m_expected = 0;
    m_bridge->setCancelCheck([this] { return m_cancel; });
    emit busyChanged();

    QString finalMessage;
    bool ok = true;
    try {
        progress(QStringLiteral("正在读取未开始活动列表…"));
        QJsonObject current = openList();
        m_expected = num(current, "total");
        if (m_expected > 1000)
            throw Failure{QStringLiteral("活动超过1000项，请确认筛选条件。")};

        QList<Activity> queue;
        while (queue.size() < m_expected) {
            for (Activity card : cardsOf(current)) {
                card.page = num(current, "page");
                queue << card;
            }
            progress(QStringLiteral("读取列表 %1 / %2").arg(queue.size()).arg(m_expected));
            if (queue.size() < m_expected)
                current = nextPage(current);
        }
        if (queue.size() != m_expected)
            throw Failure{QStringLiteral("列表数量发生变化，请重新采集。")};

        const int listTotal = m_expected;     // what the portal reports; must stay constant
        int excluded = 0;
        QList<Activity> wanted;
        for (const Activity &a : std::as_const(queue)) {
            if (isIgnored(a))
                ++excluded;
            else
                wanted << a;
        }
        queue = wanted;
        m_expected = queue.size();            // progress is measured against what we keep

        m_rows.clear();
        m_model->setRows({});
        QSet<QString> ids;
        int excludedAtDetail = 0;
        for (int i = 0; i < queue.size(); ++i) {
            Activity row = queue[i];
            m_done = i;
            progress(QStringLiteral("读取详情 %1 / %2：%3").arg(i + 1).arg(queue.size()).arg(row.name));
            current = openList();
            if (num(current, "total") != listTotal)
                throw Failure{QStringLiteral("活动数量改变，请重新采集。")};
            for (int p = 1; p < row.page; ++p)
                current = nextPage(current);
            const int index = row.index;
            const QList<Activity> visible = cardsOf(current);
            if (index < 0 || index >= visible.size() || rowKey(visible[index]) != rowKey(row))
                throw Failure{QStringLiteral("活动排序改变，请重新采集。")};
            m_bridge->eval(QStringLiteral("(()=>{document.querySelectorAll('.table-container .card-list-item')[%1].click();return true;})()").arg(index));
            try {
                const QString rowJson = QString::fromUtf8(QJsonDocument(row.toJson()).toJson(QJsonDocument::Compact));
                const QString expr = QStringLiteral(
                    "(()=>{if(!location.href.includes('/CloudPortal/CloudActivityDetail?'))return null;"
                    "try{const r=Campus.extractDetail(document,%1,location.href);delete r.collectedAt;return r;}catch{return null;}})()")
                                         .arg(rowJson);
                QJsonObject detail = until(
                    expr, [](const QJsonObject &r) { return !r.value(QLatin1String("registration")).toString().isEmpty(); },
                    QStringLiteral("活动详情"), 900);
                const QString id = detail.value(QLatin1String("url")).toString();
                if (ids.contains(id))
                    throw Failure{QStringLiteral("详情重复，请重新采集。")};
                ids.insert(id);
                detail.insert(QStringLiteral("collectedAt"), nowIso());
                const Activity a = Activity::fromJson(detail);
                if (isIgnored(a))
                    ++excludedAtDetail;
                else
                    m_rows << a;
            } catch (const TimeoutFailure &e) {
                row.error = e.message;
                row.collectedAt = nowIso();
                m_rows << row;
            }
            m_done = i + 1;
            m_model->setRows(m_rows);
            save();
            emit progressChanged();
            m_bridge->pause(350);
        }
        save();
        int failed = 0;
        for (const Activity &a : std::as_const(m_rows))
            if (!a.error.isEmpty())
                ++failed;
        const int excludedTotal = excluded + excludedAtDetail;
        const QString ignored = excludedTotal > 0
            ? QStringLiteral("，已排除%1项团日/班会/班级活动").arg(excludedTotal)
            : QString();
        if (failed == 0) {
            finalMessage = QStringLiteral("采集完成：%1项%2。可搜索、筛选并导出。").arg(m_rows.size()).arg(ignored);
        } else {
            ok = false;
            finalMessage = QStringLiteral("已处理%1项，其中%2项读取失败%3。").arg(m_rows.size()).arg(failed).arg(ignored);
        }
    } catch (const Cancelled &) {
        m_bridge->stopLoading();
        ok = false;
        finalMessage = QStringLiteral("已停止。已读取的结果可以导出。");
    } catch (const Failure &e) {
        ok = false;
        finalMessage = e.message;
    }

    m_bridge->setCancelCheck({});
    m_busy = false;
    m_lastFailed = !ok;
    progress(finalMessage);
    emit busyChanged();
    emit finished(ok, finalMessage);
    return finalMessage;
}
