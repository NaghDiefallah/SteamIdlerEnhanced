#ifndef SESSIONHISTORY_H
#define SESSIONHISTORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QMutex>
#include <QTimer>
#include "Logger.h"

struct SessionRecord {
    int id;
    int appId;
    QString gameName;
    QDateTime startTime;
    QDateTime endTime;
    qint64 duration; // in seconds
    QString status; // "active", "completed", "stopped"
};

class SessionHistory : public QObject {
    Q_OBJECT

public:
    static SessionHistory& instance() {
        static SessionHistory inst;
        return inst;
    }

    bool init(const QString& dbPath);
    void logSessionStart(int appId, const QString& gameName);
    void logSessionEnd(int appId, qint64 duration);
    void logSessionStop(int appId, qint64 duration);

    QList<SessionRecord> getRecentSessions(int limit = 50);
    QList<SessionRecord> getSessionsForGame(int appId, int limit = 20);
    qint64 getTotalPlayTime(int appId = -1); // -1 for all games

    void cleanupOldRecords(int daysToKeep = 90);

signals:
    void sessionLogged(const SessionRecord& record);

private:
    SessionHistory(QObject* parent = nullptr);
    ~SessionHistory();

    QSqlDatabase m_db;
    QMutex m_dbMutex;
    QTimer* m_cleanupTimer;

    bool createTables();
    bool executeQuery(const QString& query, const QVariantMap& params = QVariantMap());
    QList<SessionRecord> queryToRecords(QSqlQuery& query);
};

#endif