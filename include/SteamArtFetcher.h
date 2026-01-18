#ifndef STEAMARTFETCHER_H
#define STEAMARTFETCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QCache>
#include <QMutex>
#include <QTimer>
#include "Logger.h"

struct GameArt {
    QPixmap header;
    QPixmap logo;
    QPixmap capsule;
    bool loaded = false;
};

class SteamArtFetcher : public QObject {
    Q_OBJECT

public:
    static SteamArtFetcher& instance() {
        static SteamArtFetcher inst;
        return inst;
    }

    void fetchArt(int appId);
    GameArt getArt(int appId) const;
    bool hasArt(int appId) const;

signals:
    void artLoaded(int appId, const GameArt& art);
    void artLoadFailed(int appId, const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    SteamArtFetcher(QObject* parent = nullptr);
    ~SteamArtFetcher();

    QNetworkAccessManager* m_networkManager;
    QCache<int, GameArt> m_artCache;
    mutable QMutex m_cacheMutex;
    QTimer* m_cleanupTimer;

    QString getSteamArtUrl(int appId, const QString& type) const;
    void processDownloadedImage(int appId, const QByteArray& data, const QString& type);
};

#endif