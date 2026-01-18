#ifndef STATISTICS_H
#define STATISTICS_H

#include <QString>
#include <QDateTime>
#include <QMap>
#include <QObject>

struct GameStatistics {
    int appId;
    QString gameName;
    qint64 totalIdleTime = 0;      // seconds
    qint64 totalSessions = 0;
    qint64 cardsEarned = 0;
    QDateTime lastPlayed;
    bool completed = false;
    int cardsRemaining = 0;
    int achievementsUnlocked = 0;
    int totalAchievements = 0;
};

struct SessionStat {
    QDateTime startTime;
    QDateTime endTime;
    int appId;
    QString gameName;
    qint64 duration;
    int cardsEarned = 0;
};

class Statistics : public QObject {
    Q_OBJECT

public:
    static Statistics& instance() {
        static Statistics inst;
        return inst;
    }

    bool init(const QString& dbPath);
    
    // Game statistics
    void recordGameSession(int appId, const QString& name, qint64 duration);
    void markGameCompleted(int appId);
    GameStatistics getGameStats(int appId);
    QList<GameStatistics> getAllGameStats();
    
    // Totals
    qint64 getTotalIdleTime();
    qint64 getTotalSessions();
    qint64 getTotalCardsEarned();
    int getTotalGamesCompleted();
    
    // Rankings
    QList<GameStatistics> getTopPerformers(int limit = 10);
    QList<GameStatistics> getMostPlayed(int limit = 10);
    
    // History
    QList<SessionStat> getSessionHistory(int days = 0);  // 0 = all time
    QList<SessionStat> getSessionHistoryForGame(int appId);
    
    // Weekly/Monthly reports
    qint64 getIdleTimeThisWeek();
    qint64 getIdleTimeThisMonth();
    int getSessionsThisWeek();
    int getSessionsThisMonth();

    // Export/Import
    bool exportHistory(const QString& filePath);
    bool importHistory(const QString& filePath);
    bool backupSettings(const QString& filePath);
    bool restoreSettings(const QString& filePath);

signals:
    void sessionRecorded(int appId);
    void gameCompleted(int appId);

private:
    Statistics(QObject* parent = nullptr);
    ~Statistics();
    
    void createTables();
    void updateGameStats(int appId, const QString& name, qint64 duration);
    QString m_dbPath;
};

#endif
