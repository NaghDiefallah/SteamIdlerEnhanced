#include "SteamArtFetcher.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QBuffer>

SteamArtFetcher::SteamArtFetcher(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_cleanupTimer(new QTimer(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &SteamArtFetcher::onReplyFinished);

    // Cleanup cache every 5 minutes
    m_cleanupTimer->setInterval(5 * 60 * 1000);
    connect(m_cleanupTimer, &QTimer::timeout, [this]() {
        QMutexLocker locker(&m_cacheMutex);
        m_artCache.clear();
        Logger::log()->info("Art cache cleared");
    });
    m_cleanupTimer->start();

    Logger::log()->info("Steam Art Fetcher initialized");
}

SteamArtFetcher::~SteamArtFetcher() {
    m_cleanupTimer->stop();
}

void SteamArtFetcher::fetchArt(int appId) {
    // Check cache first
    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_artCache.contains(appId)) {
            GameArt* cached = m_artCache.object(appId);
            if (cached && cached->loaded) {
                emit artLoaded(appId, *cached);
                return;
            }
        }
    }

    // Fetch header image
    QUrl headerUrl = getSteamArtUrl(appId, "header");
    QNetworkRequest headerRequest(headerUrl);
    headerRequest.setAttribute(QNetworkRequest::User, QVariant(appId));
    headerRequest.setRawHeader("User-Agent", "SteamIdlerEnhanced/1.0");

    m_networkManager->get(headerRequest);
    Logger::log()->info("Fetching art for AppID: {}", appId);
}

QString SteamArtFetcher::getSteamArtUrl(int appId, const QString& type) const {
    // Steam CDN URLs for game art
    if (type == "header") {
        return QString("https://cdn.cloudflare.steamstatic.com/steam/apps/%1/header.jpg").arg(appId);
    } else if (type == "capsule") {
        return QString("https://cdn.cloudflare.steamstatic.com/steam/apps/%1/capsule_231x87.jpg").arg(appId);
    } else if (type == "logo") {
        return QString("https://cdn.cloudflare.steamstatic.com/steam/apps/%1/logo.png").arg(appId);
    }
    return QString();
}

GameArt SteamArtFetcher::getArt(int appId) const {
    QMutexLocker locker(&m_cacheMutex);
    GameArt* art = m_artCache.object(appId);
    return art ? *art : GameArt();
}

bool SteamArtFetcher::hasArt(int appId) const {
    QMutexLocker locker(&m_cacheMutex);
    GameArt* art = m_artCache.object(appId);
    return art && art->loaded;
}

void SteamArtFetcher::onReplyFinished(QNetworkReply* reply) {
    int appId = reply->request().attribute(QNetworkRequest::User).toInt();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        Logger::log()->error("Failed to fetch art for AppID {}: {}", appId, reply->errorString().toStdString());
        emit artLoadFailed(appId, reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        emit artLoadFailed(appId, "Empty response");
        return;
    }

    // Determine art type from URL
    QString url = reply->url().toString();
    QString type;
    if (url.contains("header")) {
        type = "header";
    } else if (url.contains("capsule")) {
        type = "capsule";
    } else if (url.contains("logo")) {
        type = "logo";
    }

    processDownloadedImage(appId, data, type);
}

void SteamArtFetcher::processDownloadedImage(int appId, const QByteArray& data, const QString& type) {
    QPixmap pixmap;
    if (!pixmap.loadFromData(data)) {
        emit artLoadFailed(appId, "Failed to load image data");
        return;
    }

    // Update cache
    {
        QMutexLocker locker(&m_cacheMutex);
        GameArt* art = m_artCache.object(appId);
        if (!art) {
            art = new GameArt();
            m_artCache.insert(appId, art);
        }

        if (type == "header") {
            art->header = pixmap.scaled(460, 215, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else if (type == "capsule") {
            art->capsule = pixmap;
        } else if (type == "logo") {
            art->logo = pixmap.scaled(200, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // Check if all art is loaded
        if (!art->header.isNull() && !art->capsule.isNull() && !art->logo.isNull()) {
            art->loaded = true;
            emit artLoaded(appId, *art);
            Logger::log()->info("Art loaded for AppID: {}", appId);
        }
    }
}