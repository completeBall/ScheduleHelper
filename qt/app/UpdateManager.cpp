#include "UpdateManager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QVersionNumber>

namespace {
const QUrl latestRelease(QStringLiteral("https://api.github.com/repos/completeBall/ScheduleHelper/releases/latest"));
const QUrl releaseManifest(QStringLiteral("https://raw.githubusercontent.com/completeBall/ScheduleHelper/main/update.json"));
constexpr qint64 maxArchiveBytes = 600LL * 1024 * 1024;
}

UpdateManager::UpdateManager(const QString &dataDir, bool enabled, QObject *parent)
    : QObject(parent), m_dataDir(dataDir)
{
    if (!enabled) return; // Self-tests, demos and screenshots do not access GitHub.
    QFile status(dataDir + QStringLiteral("/updates/update-status.txt"));
    if (status.open(QIODevice::ReadOnly)) {
        const QString text = QString::fromUtf8(status.readAll()).trimmed();
        if (text.startsWith(QLatin1String("failed:"))) {
            m_message = QStringLiteral("上次更新失败，旧版本已保留。请重试更新或检查安装目录权限。");
            m_showResult = true;
        }
        status.close();
        status.remove();
    }
    m_periodic.setInterval(12 * 60 * 60 * 1000);
    connect(&m_periodic, &QTimer::timeout, this, &UpdateManager::check);
    m_periodic.start();
    QTimer::singleShot(m_showResult ? 15000 : 3000, this, &UpdateManager::check);
}

QString UpdateManager::currentVersion() const
{
    return QCoreApplication::applicationVersion();
}

void UpdateManager::fail(const QString &message)
{
    m_busy = false;
    m_progress = 0;
    m_message = message;
    emit changed();
}

void UpdateManager::check()
{
    if (m_busy || m_reply) return;
    requestRelease(latestRelease, false);
}

void UpdateManager::requestRelease(const QUrl &url, bool fallback)
{
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "ScheduleHelper-Updater");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setTransferTimeout(20000);
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply.data(), fallback] {
        const QByteArray bytes = reply->readAll();
        const QString error = reply->errorString();
        const auto replyError = reply->error();
        reply->deleteLater();
        m_reply = nullptr;
        if (replyError != QNetworkReply::NoError) {
            if (!fallback) { requestRelease(releaseManifest, true); return; }
            fail(QStringLiteral("检查更新失败：") + error); return;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            fail(QStringLiteral("更新信息格式有误。")); return;
        }
        const QJsonObject release = document.object();
        const QString tag = release.value(QStringLiteral("tag_name")).toString();
        static const QRegularExpression releaseTag(QStringLiteral(R"(^v\d+\.\d+\.\d+$)"));
        if (!releaseTag.match(tag).hasMatch()) { fail(QStringLiteral("更新版本号无效。")); return; }
        const QString version = tag.mid(1);
        const QVersionNumber latest = QVersionNumber::fromString(version);
        const QVersionNumber current = QVersionNumber::fromString(currentVersion());
        if (latest.isNull() || latest.segmentCount() != 3 || current.isNull()) {
            fail(QStringLiteral("更新版本号无效。")); return;
        }
        m_latestVersion = version;
        m_available = false;
        m_assetUrl = {};
        if (QVersionNumber::compare(latest, current) > 0) {
            const QString expected = QStringLiteral("ScheduleHelper-v%1-windows-x64.zip").arg(version);
            for (const QJsonValue &value : release.value(QStringLiteral("assets")).toArray()) {
                const QJsonObject asset = value.toObject();
                const QUrl url(asset.value(QStringLiteral("browser_download_url")).toString());
                const qint64 size = asset.value(QStringLiteral("size")).toInteger();
                if (asset.value(QStringLiteral("name")).toString() != expected
                    || asset.value(QStringLiteral("state")).toString() != QLatin1String("uploaded")
                    || url.scheme() != QLatin1String("https") || url.host() != QLatin1String("github.com")
                    || !url.path().startsWith(QStringLiteral("/completeBall/ScheduleHelper/releases/download/"))
                    || size <= 0 || size > maxArchiveBytes) continue;
                m_assetUrl = url;
                m_assetSize = size;
                m_digest = asset.value(QStringLiteral("digest")).toString();
                m_available = true;
                break;
            }
            m_message = m_available ? QStringLiteral("发现新版本 v%1，点击即可下载并更新。" ).arg(version)
                                    : QStringLiteral("发现新版本 v%1，但没有可用的 Windows 运行包。" ).arg(version);
        } else {
            m_message = QStringLiteral("当前已是最新版本 v%1。").arg(currentVersion());
        }
        emit changed();
    });
}

void UpdateManager::install()
{
    if (!m_available || m_busy || m_reply) return;
    const QString installDir = QCoreApplication::applicationDirPath();
    if (!QFileInfo::exists(installDir + QStringLiteral("/Qt6Core.dll"))) {
        fail(QStringLiteral("请从已解压的 Windows 发布包运行程序后再更新。")); return;
    }
    const QString updates = m_dataDir + QStringLiteral("/updates");
    if (!QDir().mkpath(updates)) { fail(QStringLiteral("无法创建更新目录。")); return; }
    m_archive.setFileName(updates + QStringLiteral("/ScheduleHelper-v%1-windows-x64.zip").arg(m_latestVersion));
    if (!m_archive.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("无法保存更新包：") + m_archive.errorString()); return;
    }
    m_busy = true;
    m_progress = 0;
    m_message = QStringLiteral("正在下载 v%1…").arg(m_latestVersion);
    emit changed();
    QNetworkRequest request(m_assetUrl);
    request.setRawHeader("User-Agent", "ScheduleHelper-Updater");
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::readyRead, this, [this] {
        if (!m_reply || !m_archive.isOpen()) return;
        const QByteArray chunk = m_reply->readAll();
        if (m_archive.size() + chunk.size() > maxArchiveBytes || m_archive.write(chunk) != chunk.size())
            m_reply->abort();
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        const qint64 expected = total > 0 ? total : m_assetSize;
        const int progress = expected > 0 ? int(qBound<qint64>(0LL, received * 100 / expected, 99LL)) : 0;
        if (progress != m_progress) { m_progress = progress; emit changed(); }
    });
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply.data()] {
        finishDownload(reply);
        reply->deleteLater();
        m_reply = nullptr;
    });
}

void UpdateManager::finishDownload(QNetworkReply *reply)
{
    if (m_archive.isOpen()) {
        const QByteArray rest = reply->readAll();
        if (!rest.isEmpty() && m_archive.write(rest) != rest.size()) reply->abort();
        m_archive.close();
    }
    if (reply->error() != QNetworkReply::NoError || QFileInfo(m_archive.fileName()).size() != m_assetSize) {
        QFile::remove(m_archive.fileName());
        fail(QStringLiteral("下载失败或文件不完整：") + reply->errorString());
        return;
    }
    if (m_digest.startsWith(QLatin1String("sha256:"), Qt::CaseInsensitive)) {
        QFile file(m_archive.fileName());
        if (!file.open(QIODevice::ReadOnly)) { fail(QStringLiteral("无法校验更新包。")); return; }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!hash.addData(&file) || QString::fromLatin1(hash.result().toHex()).compare(m_digest.mid(7), Qt::CaseInsensitive) != 0) {
            file.close();
            QFile::remove(m_archive.fileName());
            fail(QStringLiteral("更新包校验失败，已删除下载文件。"));
            return;
        }
    }
    m_progress = 100;
    m_message = QStringLiteral("下载完成，正在安装并重启…");
    emit changed();
    if (!launchInstaller()) { fail(QStringLiteral("无法启动更新程序，请检查安装目录权限。")); return; }
    QCoreApplication::quit();
}

bool UpdateManager::launchInstaller()
{
    QFile source(QStringLiteral(":/gdipu/js/install-update.ps1"));
    if (!source.open(QIODevice::ReadOnly)) return false;
    const QString script = m_dataDir + QStringLiteral("/updates/install-update.ps1");
    QSaveFile out(script);
    if (!out.open(QIODevice::WriteOnly) || out.write(source.readAll()) < 0 || !out.commit()) return false;
    const QString installDir = QCoreApplication::applicationDirPath();
    const QStringList arguments{QStringLiteral("-NoProfile"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                                QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"), QStringLiteral("-File"), script,
                                QStringLiteral("-Archive"), m_archive.fileName(), QStringLiteral("-InstallDir"), installDir,
                                QStringLiteral("-ProcessId"), QString::number(QCoreApplication::applicationPid())};
    return QProcess::startDetached(QStringLiteral("powershell.exe"), arguments, QFileInfo(installDir).absolutePath());
}
