#include "UpdateChecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QCoreApplication>
#include <QDateTime>
#include "AppConfig.h"
// Added missing includes for file and path operations
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QIODevice>

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_autoCheckTimer(new QTimer(this))
    , m_currentVersion(QVersionNumber::fromString(getCurrentVersion()))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onReplyFinished);

    // Auto-check every 24 hours
    m_autoCheckTimer->setInterval(24 * 60 * 60 * 1000);
    connect(m_autoCheckTimer, &QTimer::timeout, this, &UpdateChecker::onAutoCheckTimer);

    if (m_autoCheck) {
        m_autoCheckTimer->start();
    }

    Logger::log()->info("Update Checker initialized - Current version: {}", m_currentVersion.toString().toStdString());
}

UpdateChecker::~UpdateChecker() {
    m_autoCheckTimer->stop();
}

void UpdateChecker::checkForUpdates() {
    QUrl updateUrl("https://api.github.com/repos/yourusername/SteamIdlerEnhanced/releases/latest");
    QNetworkRequest request(updateUrl);
    request.setRawHeader("User-Agent", "SteamIdlerEnhanced/1.0");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    m_networkManager->get(request);
    Logger::log()->info("Checking for updates...");
}

void UpdateChecker::setAutoCheck(bool enabled) {
    m_autoCheck = enabled;
    if (enabled) {
        m_autoCheckTimer->start();
    } else {
        m_autoCheckTimer->stop();
    }
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        Logger::log()->error("Update check failed: {}", reply->errorString().toStdString());
        emit updateCheckFailed(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    parseUpdateResponse(data);
}

void UpdateChecker::onAutoCheckTimer() {
    checkForUpdates();
}

void UpdateChecker::downloadUpdate(const QUrl& url) {
    if (!url.isValid()) {
        emit updateCheckFailed("Invalid download URL");
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "SteamIdlerEnhanced/1.0");

    QNetworkReply* reply = m_networkManager->get(request);
    QString outDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/updates";
    QDir().mkpath(outDir);
    QString outFile = QDir(outDir).absoluteFilePath(url.fileName());

    QFile* file = new QFile(outFile);
    if (!file->open(QIODevice::WriteOnly)) {
        emit updateCheckFailed("Failed to open output file for download");
        delete file;
        return;
    }

    connect(reply, &QNetworkReply::readyRead, [reply, file]() {
        file->write(reply->readAll());
    });

    connect(reply, &QNetworkReply::finished, [this, reply, file, outFile]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit updateCheckFailed(reply->errorString());
        } else {
            file->flush();
            file->close();
            emit updateDownloaded(outFile);
            Logger::log()->info("Update downloaded to {}", outFile.toStdString());
        }
        reply->deleteLater();
        file->deleteLater();
    });
}

void UpdateChecker::parseUpdateResponse(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit updateCheckFailed("Invalid JSON response");
        return;
    }

    QJsonObject release = doc.object();
    QString latestVersion = release.value("tag_name").toString();
    QString changelog = release.value("body").toString();

    QVersionNumber remoteVersion = QVersionNumber::fromString(latestVersion);

    if (remoteVersion > m_currentVersion) {
        // Get download URL from assets
        QJsonArray assets = release.value("assets").toArray();
        QUrl downloadUrl;
        for (const QJsonValue& asset : assets) {
            QJsonObject assetObj = asset.toObject();
            QString name = assetObj.value("name").toString();
            if (name.contains("SteamIdlerEnhanced") && name.endsWith(".exe")) {
                downloadUrl = QUrl(assetObj.value("browser_download_url").toString());
                break;
            }
        }

        Logger::log()->info("Update available: {} -> {}", m_currentVersion.toString().toStdString(), latestVersion.toStdString());
        emit updateAvailable(latestVersion, changelog, downloadUrl);
    } else {
        Logger::log()->info("No updates available");
        emit noUpdateAvailable();
    }
}

QString UpdateChecker::getCurrentVersion() const {
    // In a real application, this would read from version info
    return "1.1.0";
}