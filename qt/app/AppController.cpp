#include "AppController.h"

#include "core/Storage.h"
#include "core/XlsxExport.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QQuickWindow>
#include <QTextStream>

using namespace Campus;

namespace {

QString readResource(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(f.readAll());
}

QString htmlEscape(const QString &s)
{
    return s.toHtmlEscaped();
}

} // namespace

AppController::AppController(const AppOptions &options, QObject *parent)
    : QObject(parent)
    , m_options(options)
{
    m_dataDir = options.testing ? options.testDir : dataDirectory();
    QDir().mkpath(m_dataDir);

    m_activities = new ActivityModel(this);
    m_schedule = new ScheduleModel(m_activities, this);
    m_school = new WebBridge(QStringLiteral("school"), this);
    m_academic = new WebBridge(QStringLiteral("academic"), this);
    m_scraper = new Scraper(m_school, m_activities, this);
    m_importer = new ScheduleImporter(m_academic, m_schedule, this);
    m_updater = new UpdateManager(m_dataDir, !options.testing && options.demoActivities.isEmpty()
                                    && options.screenshotDir.isEmpty(), this);

    const QString prelude = readResource(QStringLiteral(":/gdipu/js/extract.js"));
    m_school->setPrelude(prelude);
    m_academic->setPrelude(prelude);

    if (!options.baseUrl.isEmpty())
        m_scraper->setBaseUrl(options.baseUrl);

    QSettings settings(m_dataDir + QStringLiteral("/settings.ini"), QSettings::IniFormat);
    m_theme = options.theme.isEmpty() ? settings.value(QStringLiteral("theme"), QStringLiteral("system")).toString()
                                      : options.theme;
    m_autoCollectHours = settings.value(QStringLiteral("autoCollectHours"), 0).toInt();
    if (!QList<int>{0, 3, 6, 12, 24}.contains(m_autoCollectHours)) m_autoCollectHours = 0;
    m_desktopReminders = settings.value(QStringLiteral("desktopReminders"), true).toBool();
    m_lastCollection = QDateTime::fromString(settings.value(QStringLiteral("lastCollection")).toString(), Qt::ISODateWithMs);
    m_activities->setClaimedKeys(settings.value(QStringLiteral("claimedActivities")).toStringList());
    m_activities->setAutoClaimedKeys(settings.value(QStringLiteral("autoClaimedActivities")).toStringList());
    QList<Activity> claimedSnapshots;
    for (const QJsonValue &value : QJsonDocument::fromJson(settings.value(QStringLiteral("claimedSnapshots")).toByteArray()).array())
        claimedSnapshots << Activity::fromJson(value.toObject());
    m_activities->setClaimedSnapshots(claimedSnapshots);

    m_notice = QStringLiteral("欢迎使用：首次请先在“学校登录”页登录，再点击“开始采集”。");

    // --self-test and --demo runs never read or write the user's real data
    const bool persist = !options.testing && options.demoActivities.isEmpty();
    if (persist) {
        m_scraper->setResultsPath(m_dataDir + QStringLiteral("/last-results.json"));
        m_schedule->setStoragePath(m_dataDir + QStringLiteral("/timetable.json"));

        QList<Activity> rows;
        QString err;
        if (loadActivities(m_dataDir + QStringLiteral("/last-results.json"), &rows, &err)) {
            if (!rows.isEmpty()) {
                m_activities->setRows(rows);
                m_notice = QStringLiteral("已恢复上次结果。点击“开始采集”获取最新活动。");
            }
        } else {
            m_notice = QStringLiteral("上次结果无法读取，请重新采集。");
        }
        if (!m_schedule->load(&err))
            m_notice = QStringLiteral("旧课表无法读取，请重新导入。");
    }

    connect(m_scraper, &Scraper::progressChanged, this, [this] { setNotice(m_scraper->message()); });
    connect(m_scraper, &Scraper::finished, this, [this](bool ok, const QString &message) {
        if (!m_options.testing) {
            QJsonArray saved;
            for (const Activity &activity : m_activities->claimedSnapshots()) saved.append(activity.toJson());
            QSettings settings(m_dataDir + QStringLiteral("/settings.ini"), QSettings::IniFormat);
            settings.setValue(QStringLiteral("claimedSnapshots"), QJsonDocument(saved).toJson(QJsonDocument::Compact));
        }
        if (ok && !m_options.testing) {
            m_lastCollection = QDateTime::currentDateTimeUtc();
            QSettings settings(m_dataDir + QStringLiteral("/settings.ini"), QSettings::IniFormat);
            settings.setValue(QStringLiteral("lastCollection"), m_lastCollection.toString(Qt::ISODateWithMs));
        }
        setNotice(message, ok ? QStringLiteral("info") : QStringLiteral("error"));
        emit toast(message, ok ? QStringLiteral("success") : QStringLiteral("error"));
    });
    connect(m_scraper, &Scraper::loginRequired, this, [this] {
        setBrowserTab(0);
        setPage(2);
        if (m_tray) {
            m_tray->showMessage(QStringLiteral("自动采集需要登录"),
                                QStringLiteral("请打开程序并重新登录学校活动系统。"),
                                QSystemTrayIcon::Warning, 12000);
        }
    });
    connect(m_importer, &ScheduleImporter::messageChanged, this, [this] { setNotice(m_importer->message()); });
    connect(m_importer, &ScheduleImporter::finished, this, [this](bool ok, const QString &message) {
        setNotice(message, ok ? QStringLiteral("info") : QStringLiteral("error"));
        emit toast(message, ok ? QStringLiteral("success") : QStringLiteral("error"));
    });
    connect(m_importer, &ScheduleImporter::loginRequired, this, [this] {
        setBrowserTab(1);
        setPage(2);
    });
    connect(m_schedule, &ScheduleModel::persistFailed, this, [this](const QString &err) {
        emit toast(QStringLiteral("课表保存失败：") + err, QStringLiteral("error"));
    });
    connect(m_activities, &ActivityModel::claimedChanged, this, [this] {
        if (!m_options.testing) {
            QSettings settings(m_dataDir + QStringLiteral("/settings.ini"), QSettings::IniFormat);
            settings.setValue(QStringLiteral("claimedActivities"), m_activities->claimedKeys());
            settings.setValue(QStringLiteral("autoClaimedActivities"), m_activities->autoClaimedKeys());
            QJsonArray saved;
            for (const Activity &activity : m_activities->claimedSnapshots()) saved.append(activity.toJson());
            settings.setValue(QStringLiteral("claimedSnapshots"), QJsonDocument(saved).toJson(QJsonDocument::Compact));
        }
    });
    if (!options.testing && options.demoActivities.isEmpty()) {
        if (QSystemTrayIcon::isSystemTrayAvailable()) {
            m_tray = new QSystemTrayIcon(qGuiApp->windowIcon(), this);
            m_tray->setToolTip(QStringLiteral("广轻活动汇总 · 课表提醒"));
            auto *menu = new QMenu;
            connect(menu->addAction(QStringLiteral("打开广轻活动汇总")), &QAction::triggered, this, [] {
                if (auto *window = qGuiApp->allWindows().value(0)) { window->show(); window->raise(); window->requestActivate(); }
            });
            connect(menu->addAction(QStringLiteral("退出")), &QAction::triggered, qGuiApp, &QGuiApplication::quit);
            m_tray->setContextMenu(menu);
            connect(m_tray, &QSystemTrayIcon::activated, this, [](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::DoubleClick)
                    if (auto *window = qGuiApp->allWindows().value(0)) { window->show(); window->raise(); window->requestActivate(); }
            });
            m_tray->show();
            emit trayAvailableChanged();
        }
        m_clock.setInterval(60 * 1000);
        connect(&m_clock, &QTimer::timeout, this, &AppController::checkTimers);
        m_clock.start();
        QTimer::singleShot(1000, this, &AppController::checkTimers);
    }
    connect(m_updater, &UpdateManager::changed, this, [this] {
        if (m_tray && m_updater->available() && m_updateNotifiedVersion != m_updater->latestVersion()) {
            m_updateNotifiedVersion = m_updater->latestVersion();
            m_tray->showMessage(QStringLiteral("发现新版本 v%1").arg(m_updateNotifiedVersion),
                                QStringLiteral("打开广轻活动汇总，点击更新按钮即可安装。"),
                                QSystemTrayIcon::Information, 10000);
        }
    });

    // Closing the window must unwind any nested wait loops before the app quits.
    connect(qGuiApp, &QGuiApplication::lastWindowClosed, this, [this] { cancelEverything(); });
    connect(qGuiApp, &QGuiApplication::aboutToQuit, this, [this] { cancelEverything(); });
}

void AppController::cancelEverything()
{
    m_scraper->stop();
    m_importer->stop();
}

void AppController::setNotice(const QString &text, const QString &kind)
{
    if (m_notice == text && m_noticeKind == kind)
        return;
    m_notice = text;
    m_noticeKind = kind;
    emit noticeChanged();
}

void AppController::setPage(int page)
{
    if (m_page == page)
        return;
    m_page = page;
    emit pageChanged();
}

void AppController::setBrowserTab(int tab)
{
    if (m_browserTab == tab)
        return;
    m_browserTab = tab;
    emit browserTabChanged();
}

void AppController::setThemeMode(const QString &mode)
{
    if (m_theme == mode)
        return;
    m_theme = mode;
    if (!m_options.testing) {
        QSettings settings(m_dataDir + QStringLiteral("/settings.ini"), QSettings::IniFormat);
        settings.setValue(QStringLiteral("theme"), mode);
    }
    emit themeModeChanged();
}

void AppController::setAutoCollectHours(int hours)
{
    if (!QList<int>{0, 3, 6, 12, 24}.contains(hours) || hours == m_autoCollectHours) return;
    m_autoCollectHours = hours;
    QSettings settings(m_dataDir + QStringLiteral("/settings.ini"), QSettings::IniFormat);
    settings.setValue(QStringLiteral("autoCollectHours"), hours);
    emit autoCollectHoursChanged();
    if (hours > 0) QTimer::singleShot(0, this, &AppController::checkTimers);
}

void AppController::setDesktopReminders(bool enabled)
{
    if (enabled == m_desktopReminders) return;
    m_desktopReminders = enabled;
    QSettings settings(m_dataDir + QStringLiteral("/settings.ini"), QSettings::IniFormat);
    settings.setValue(QStringLiteral("desktopReminders"), enabled);
    emit desktopRemindersChanged();
}

void AppController::hideToTray()
{
    if (!m_tray) return;
    if (auto *window = qGuiApp->allWindows().value(0)) window->hide();
    m_tray->showMessage(QStringLiteral("广轻活动汇总仍在运行"),
                        QStringLiteral("自动采集和课表提醒会继续执行。双击托盘图标可重新打开。"),
                        QSystemTrayIcon::Information, 5000);
}

void AppController::checkTimers()
{
    if (m_autoCollectHours > 0 && !m_scraper->busy() && !m_importer->busy() && m_school->attached()
        && (!m_lastCollection.isValid() || m_lastCollection.secsTo(QDateTime::currentDateTimeUtc()) >= m_autoCollectHours * 3600)) {
        // A failed login/collection is retried on the next interval, not every minute.
        m_lastCollection = QDateTime::currentDateTimeUtc();
        QSettings settings(m_dataDir + QStringLiteral("/settings.ini"), QSettings::IniFormat);
        settings.setValue(QStringLiteral("lastCollection"), m_lastCollection.toString(Qt::ISODateWithMs));
        m_scraper->collect();
    }
    checkDesktopReminders();
}

void AppController::checkDesktopReminders()
{
    if (!m_desktopReminders || !m_tray || !m_schedule->hasAnchor()) return;
    const QDateTime now = nowInBeijing();
    const QDate date = now.date();
    const QDate monday = QDate::fromString(m_schedule->weekOneMonday(), Qt::ISODate);
    if (!monday.isValid() || date < monday) return;
    const int week = int(monday.daysTo(date)) / 7 + 1;
    const int day = date.dayOfWeek();
    if (week < 1 || week > 40) return;
    QStringList tasks;
    const auto due = [&](const QString &key, const QDateTime &when) {
        if (!when.isValid() || when > now || when.secsTo(now) > 90 || m_notified.contains(key)) return false;
        m_notified.insert(key);
        return true;
    };
    const auto &blocks = periods();
    for (int block = 0; block < blocks.size(); ++block) {
        const QTime start = QTime::fromString(blocks.at(block).time.left(5), QStringLiteral("HH:mm"));
        const QDateTime when(date, start, beijing());
        if (!start.isValid() || !due(date.toString(Qt::ISODate) + QStringLiteral("/slot/") + QString::number(block), when)) continue;
        for (const Course &course : m_schedule->data().allCourses())
            if (course.day == day && course.inWeek(week) && course.inBlock(block)) tasks << QStringLiteral("课程：") + course.name;
        const QString note = m_schedule->memo(week, day, block);
        if (!note.isEmpty()) tasks << QStringLiteral("备忘录：") + note;
    }
    for (const Activity &activity : m_activities->reminderActivities()) {
        const bool claimed = m_activities->isClaimed(activity);
        const QString timeText = claimed ? activity.activityTime : activity.registration;
        const QDateTime when = parseTimestamp(timeText);
        const QString key = date.toString(Qt::ISODate) + QLatin1Char('/') + ActivityModel::activityKey(activity);
        if (due(key, when)) tasks << (claimed ? QStringLiteral("活动：") : QStringLiteral("报名：")) + activity.name;
    }
    if (!tasks.isEmpty()) m_tray->showMessage(QStringLiteral("当前时间段任务"), tasks.join(QLatin1Char('\n')), QSystemTrayIcon::Information, 12000);
    if (m_notified.size() > 500) m_notified.clear();
}

QString AppController::schoolHome() const
{
    const QString base = m_options.baseUrl.isEmpty() ? QStringLiteral("https://study.gdipu.edu.cn") : m_options.baseUrl;
    return base + QStringLiteral("/CloudPortal/CloudSquare");
}

QString AppController::defaultExportName() const
{
    return QStringLiteral("未开始活动_%1.xlsx").arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
}

QString AppController::dateToday() const
{
    return QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
}

void AppController::openSchool()
{
    setBrowserTab(0);
    setPage(2);
    if (!m_scraper->busy())
        m_school->go(schoolHome());
}

void AppController::openAcademic()
{
    setBrowserTab(1);
    setPage(2);
    m_importer->openLogin();
}

void AppController::openActivity(const QString &url)
{
    if (url.isEmpty())
        return;
    if (m_scraper->busy()) {
        emit toast(QStringLiteral("采集进行中，完成后再查看活动详情。"), QStringLiteral("info"));
        return;
    }
    setBrowserTab(0);
    setPage(2);
    m_school->go(url);
}

void AppController::startCollect()
{
    if (m_scraper->busy())
        return;
    if (!m_school->attached()) {
        emit toast(QStringLiteral("内置浏览器正在启动，请稍后再试。"), QStringLiteral("info"));
        return;
    }
    m_scraper->collect();
}

void AppController::openDataFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_dataDir));
}

QString AppController::exportExcelTo(const QString &path)
{
    QString err;
    if (!exportXlsx(path, m_activities->displayed(), nowInBeijing(), &err))
        return err.isEmpty() ? QStringLiteral("导出失败") : err;
    return {};
}

void AppController::exportExcel(const QUrl &file)
{
    if (file.isEmpty())
        return;
    QString path = file.isLocalFile() ? file.toLocalFile() : file.toString();
    if (!path.endsWith(QLatin1String(".xlsx"), Qt::CaseInsensitive))
        path += QStringLiteral(".xlsx");
    const QString err = exportExcelTo(path);
    if (err.isEmpty())
        emit toast(QStringLiteral("已导出 Excel：") + QDir::toNativeSeparators(path), QStringLiteral("success"));
    else
        emit toast(QStringLiteral("导出失败：") + err, QStringLiteral("error"));
}

void AppController::exportHtml(const QUrl &file)
{
    if (file.isEmpty())
        return;
    QString path = file.isLocalFile() ? file.toLocalFile() : file.toString();
    if (!path.endsWith(QLatin1String(".html"), Qt::CaseInsensitive))
        path += QStringLiteral(".html");

    QList<Activity> rows = m_activities->all();
    sortByRegistrationStart(rows);
    const QDateTime now = nowInBeijing();
    QString html = QStringLiteral(
        "<!doctype html><html lang=\"zh-CN\"><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>广轻未开始活动汇总</title><style>body{margin:0;background:#f4f7f9;color:#172b3a;font:14px/1.6 'Microsoft YaHei',system-ui,sans-serif}"
        "main{max-width:1200px;margin:0 auto;padding:28px}h1{margin:0 0 4px}p{color:#667986;margin:0 0 18px}"
        "table{width:100%;border-collapse:collapse;background:#fff;border:1px solid #dbe5ea}th,td{padding:10px 12px;border-bottom:1px solid #e8eef1;text-align:left;vertical-align:top}"
        "th{background:#edf3f6;font-size:12px;color:#546b79}</style><main><h1>未开始活动汇总</h1><p>数据采集自学校活动系统 · 共 %1 项 · 按报名开始时间排列（北京时间）</p>"
        "<table><thead><tr><th>活动名称</th><th>报名开始</th><th>报名截止</th><th>活动地点</th><th>活动时间</th><th>名额</th><th>状态</th></tr></thead><tbody>")
                       .arg(rows.size());
    for (const Activity &a : rows) {
        const RegistrationDates d = registrationDates(a);
        const QString link = safeUrl(a.url);
        const QString name = link.isEmpty()
            ? htmlEscape(a.name)
            : QStringLiteral("<a href=\"%1\" target=\"_blank\" rel=\"noopener noreferrer\">%2</a>").arg(htmlEscape(link), htmlEscape(a.name));
        html += QStringLiteral("<tr><td>%1<br><small>%2 · %3</small></td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8 / 剩余 %9</td><td>%10</td></tr>")
                    .arg(name, htmlEscape(a.type), htmlEscape(a.organizer), htmlEscape(d.start), htmlEscape(d.end),
                         htmlEscape(a.place), htmlEscape(a.activityTime), htmlEscape(a.capacity), htmlEscape(a.remaining),
                         htmlEscape(status(a, now)));
    }
    html += QStringLiteral("</tbody></table></main></html>");

    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit toast(QStringLiteral("导出失败：") + out.errorString(), QStringLiteral("error"));
        return;
    }
    out.write(html.toUtf8());
    emit toast(QStringLiteral("已保存网页：") + QDir::toNativeSeparators(path), QStringLiteral("success"));
}

void AppController::loadDemo(const QString &activitiesJson)
{
    QFile f(activitiesJson);
    if (!f.open(QIODevice::ReadOnly))
        return;
    QList<Activity> rows;
    const QString collected = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    for (const QJsonValue &v : QJsonDocument::fromJson(f.readAll()).array()) {
        Activity a = Activity::fromJson(v.toObject());
        a.collectedAt = collected;
        if (!isIgnored(a))
            rows << a;
    }
    m_activities->setRows(rows);
    setNotice(QStringLiteral("采集完成：%1项，已排除若干团日/班会/班级活动。可搜索、筛选并导出。").arg(rows.size()));
}
