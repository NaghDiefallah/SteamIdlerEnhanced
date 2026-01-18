#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QVersionNumber>
#include "Logger.h"

class UpdateChecker : public QObject {
    Q_OBJECT

public:
    static UpdateChecker& instance() {
        static UpdateChecker inst;
        return inst;
    }

    void checkForUpdates();
    void setAutoCheck(bool enabled);
    bool isAutoCheckEnabled() const { return m_autoCheck; }
    void downloadUpdate(const QUrl& url);

signals:
    void updateAvailable(const QString& version, const QString& changelog, const QUrl& downloadUrl);
    void noUpdateAvailable();
    void updateCheckFailed(const QString& error);
    void updateDownloaded(const QString& filePath);

private slots:
    void onReplyFinished(QNetworkReply* reply);
    void onAutoCheckTimer();

private:
    UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker();

    QNetworkAccessManager* m_networkManager;
    QTimer* m_autoCheckTimer;
    bool m_autoCheck = true;
    QVersionNumber m_currentVersion;

    void parseUpdateResponse(const QByteArray& data);
    QString getCurrentVersion() const;
};

#endif