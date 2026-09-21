#include "ScheduleImporter.h"

#include <QDeadlineTimer>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QTimer>

using namespace Campus;

ScheduleImporter::ScheduleImporter(WebBridge *academic, ScheduleModel *model, QObject *parent)
    : QObject(parent)
    , m_bridge(academic)
    , m_model(model)
    , m_mainUrl(academicMainUrl())
{
}

QString ScheduleImporter::academicMainUrl()
{
    return QStringLiteral("https://jw.gdipu.edu.cn/jsxsd/framework/xsMain.jsp");
}

void ScheduleImporter::say(const QString &text)
{
    m_message = text;
    emit messageChanged();
}

void ScheduleImporter::navigate(const QString &url)
{
    m_bridge->load(QUrl(url));
}

void ScheduleImporter::openLogin()
{
    if (m_busy)
        return;
    QTimer::singleShot(0, this, [this] {
        try {
            m_bridge->setCancelCheck({});
            if (m_bridge->currentUrl().isEmpty() || m_bridge->currentUrl() == QLatin1String("about:blank"))
                navigate(m_mainUrl);
        } catch (const Failure &e) {
            say(e.message);
        } catch (const Cancelled &) {
        }
    });
}

void ScheduleImporter::import()
{
    if (m_busy)
        return;
    QTimer::singleShot(0, this, [this] { runImport(); });
}

QString ScheduleImporter::runImport()
{
    if (m_busy)
        return {};
    m_busy = true;
    m_cancel = false;
    m_bridge->setCancelCheck([this] { return m_cancel; });
    emit busyChanged();

    QString failure;
    try {
        if (m_bridge->currentUrl().isEmpty() || m_bridge->currentUrl() == QLatin1String("about:blank"))
            navigate(m_mainUrl);
        say(QStringLiteral("正在查找教务系统的个人课表…"));

        const QString probe = QStringLiteral(
            "(()=>{const data=GdipuTimetable.extract(document);const link=GdipuTimetable.nextLink(document);"
            "return {data,link:link?link.label:'',login:!!document.querySelector('input[type=password]'),url:location.href};})()");
        const QString click = QStringLiteral(
            "(()=>{const link=GdipuTimetable.nextLink(document);if(link)link.element.click();return true;})()");

        QDeadlineTimer deadline(45 * 1000);
        QElapsedTimer stableSince;
        stableSince.start();
        QByteArray previous;
        QSet<QString> clicked;
        QJsonObject imported;
        bool found = false;

        while (!deadline.hasExpired()) {
            const QJsonValue v = m_bridge->eval(probe);
            if (!v.isObject()) {
                m_bridge->pause(300);
                continue;
            }
            const QJsonObject state = v.toObject();
            const QJsonValue data = state.value(QLatin1String("data"));
            if (data.isObject()) {
                const QByteArray key = QJsonDocument(data.toObject()).toJson(QJsonDocument::Compact);
                if (key != previous) {
                    previous = key;
                    stableSince.restart();
                } else if (stableSince.elapsed() > 1000) {
                    imported = data.toObject();
                    found = true;
                    break;
                }
            } else if (state.value(QLatin1String("login")).toBool()) {
                emit loginRequired();
                throw Failure{QStringLiteral("请先在“教务系统登录”页完成登录，再点击课表页的“一键导入课表”。")};
            } else if (const QString link = state.value(QLatin1String("link")).toString(); !link.isEmpty()
                       && !clicked.contains(state.value(QLatin1String("url")).toString() + QLatin1Char('|') + link)) {
                clicked.insert(state.value(QLatin1String("url")).toString() + QLatin1Char('|') + link);
                m_bridge->eval(click);
                previous.clear();
            }
            m_bridge->pause(300);
        }
        if (!found) {
            emit loginRequired();
            throw Failure{QStringLiteral("未找到可识别的个人课表。请在教务系统中打开“学生个人课表”，选择学期并查询，再点击“一键导入课表”。原课表未覆盖。")};
        }
        const int count = imported.value(QLatin1String("courses")).toArray().size();
        if (count == 0)
            throw Failure{QStringLiteral("当前教务课表为空，未覆盖已有课表。请确认学期与查询范围后重试。")};
        if (imported.value(QLatin1String("unknownRows")).toInt() > 0)
            throw Failure{QStringLiteral("部分课程行的节次未能识别，未覆盖已有课表。需要根据实际课表调整导入规则。")};

        m_model->applyImport(Schedule::fromJson(imported));
        say(QStringLiteral("已导入 %1 条课程安排，可按周次查看；关闭程序后也会保留。").arg(count));
    } catch (const Cancelled &) {
        failure = QStringLiteral("已取消导入，原课表未改动。");
    } catch (const Failure &e) {
        failure = e.message;
    }

    m_bridge->setCancelCheck({});
    m_busy = false;
    if (!failure.isEmpty())
        say(failure);
    emit busyChanged();
    emit finished(failure.isEmpty(), failure.isEmpty() ? m_message : failure);
    return failure;
}
