#pragma once

#include "ActivityModel.h"
#include "ScheduleImporter.h"
#include "ScheduleModel.h"
#include "Scraper.h"
#include "WebBridge.h"

#include <QObject>
#include <QUrl>

struct AppOptions {
    bool testing = false;            // --self-test <dir>: fixture server, nothing persisted
    QString testDir;
    QString baseUrl;                 // --base-url
    QString demoActivities;          // --demo <activities.json>: populate the UI with sample data
    QString screenshotDir;           // --screenshot <dir>: save every page as PNG, then quit
    QString scheduleFixture;         // --schedule-fixture <url>
    QString theme;                   // --theme light|dark (overrides the saved choice for this run)
};

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ActivityModel *activities READ activities CONSTANT)
    Q_PROPERTY(ScheduleModel *schedule READ schedule CONSTANT)
    Q_PROPERTY(Scraper *scraper READ scraper CONSTANT)
    Q_PROPERTY(ScheduleImporter *importer READ importer CONSTANT)
    Q_PROPERTY(WebBridge *schoolBridge READ schoolBridge CONSTANT)
    Q_PROPERTY(WebBridge *academicBridge READ academicBridge CONSTANT)
    Q_PROPERTY(int page READ page WRITE setPage NOTIFY pageChanged)
    Q_PROPERTY(int browserTab READ browserTab WRITE setBrowserTab NOTIFY browserTabChanged)
    Q_PROPERTY(QString notice READ notice NOTIFY noticeChanged)
    Q_PROPERTY(QString noticeKind READ noticeKind NOTIFY noticeChanged)   // info | error
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(QString dataDir READ dataDir CONSTANT)
    Q_PROPERTY(QString profilePath READ profilePath CONSTANT)
    Q_PROPERTY(bool testing READ testing CONSTANT)
    Q_PROPERTY(QString schoolHome READ schoolHome CONSTANT)
    Q_PROPERTY(QString defaultExportName READ defaultExportName CONSTANT)

public:
    explicit AppController(const AppOptions &options, QObject *parent = nullptr);

    ActivityModel *activities() const { return m_activities; }
    ScheduleModel *schedule() const { return m_schedule; }
    Scraper *scraper() const { return m_scraper; }
    ScheduleImporter *importer() const { return m_importer; }
    WebBridge *schoolBridge() const { return m_school; }
    WebBridge *academicBridge() const { return m_academic; }
    const AppOptions &options() const { return m_options; }

    int page() const { return m_page; }
    void setPage(int page);
    int browserTab() const { return m_browserTab; }
    void setBrowserTab(int tab);
    QString notice() const { return m_notice; }
    QString noticeKind() const { return m_noticeKind; }
    QString themeMode() const { return m_theme; }
    void setThemeMode(const QString &mode);
    QString dataDir() const { return m_dataDir; }
    QString profilePath() const { return m_dataDir + QStringLiteral("/BrowserProfileQt"); }
    bool testing() const { return m_options.testing; }
    QString schoolHome() const;
    QString defaultExportName() const;

    void loadDemo(const QString &activitiesJson);
    void cancelEverything();

    // Blocking export used by the self-test; returns "" on success.
    QString exportExcelTo(const QString &path);

    Q_INVOKABLE void openSchool();
    Q_INVOKABLE void openAcademic();
    Q_INVOKABLE void openActivity(const QString &url);
    Q_INVOKABLE void startCollect();
    Q_INVOKABLE void exportExcel(const QUrl &file);
    Q_INVOKABLE void exportHtml(const QUrl &file);
    Q_INVOKABLE void openDataFolder();
    Q_INVOKABLE QString dateToday() const;

signals:
    void pageChanged();
    void browserTabChanged();
    void noticeChanged();
    void themeModeChanged();
    void toast(const QString &message, const QString &kind);   // kind: info | success | error

private:
    void setNotice(const QString &text, const QString &kind = QStringLiteral("info"));

    AppOptions m_options;
    QString m_dataDir;
    ActivityModel *m_activities;
    ScheduleModel *m_schedule;
    WebBridge *m_school;
    WebBridge *m_academic;
    Scraper *m_scraper;
    ScheduleImporter *m_importer;
    int m_page = 0;
    int m_browserTab = 0;
    QString m_notice;
    QString m_noticeKind = QStringLiteral("info");
    QString m_theme = QStringLiteral("system");
};
