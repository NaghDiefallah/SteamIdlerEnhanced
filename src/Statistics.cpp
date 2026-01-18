#include "Statistics.h"
#include "Logger.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSettings>
#include <QDir>

Statistics::Statistics(QObject* parent) : QObject(parent) {
    Logger::log()->info("Statistics initialized");
}

Statistics::~Statistics() {
}

bool Statistics::init(const QString& dbPath) {
    m_dbPath = dbPath;
    createTables();
    Logger::log()->info("Statistics database initialized: {}", dbPath.toStdString());
    return true;
}

void Statistics::createTables() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    
    // Sessions table
    if (!query.exec("CREATE TABLE IF NOT EXISTS sessions ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "appId INTEGER NOT NULL,"
                   "gameName TEXT NOT NULL,"
                   "startTime DATETIME NOT NULL,"
                   "endTime DATETIME,"
                   "duration INTEGER DEFAULT 0,"
                   "cardsEarned INTEGER DEFAULT 0)")) {
        Logger::log()->error("Failed to create sessions table: {}", query.lastError().text().toStdString());
    }
    
    // Game stats table
    if (!query.exec("CREATE TABLE IF NOT EXISTS game_stats ("
                   "appId INTEGER PRIMARY KEY,"
                   "gameName TEXT NOT NULL,"
                   "totalIdleTime INTEGER DEFAULT 0,"
                   "cardsEarned INTEGER DEFAULT 0,"
                   "completed BOOLEAN DEFAULT 0,"
                   "achievements INTEGER DEFAULT 0)")) {
        Logger::log()->error("Failed to create game_stats table: {}", query.lastError().text().toStdString());
    }
    
    // Create indices
    query.exec("CREATE INDEX IF NOT EXISTS idx_sessions_appId ON sessions(appId)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_sessions_startTime ON sessions(startTime)");
}

void Statistics::recordGameSession(int appId, const QString& name, qint64 duration) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("INSERT INTO sessions (appId, gameName, startTime, duration) "
                  "VALUES (:appId, :name, :start, :duration)");
    query.addBindValue(appId);
    query.addBindValue(name);
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(duration);
    
    if (!query.exec()) {
        Logger::log()->error("Failed to record session: {}", query.lastError().text().toStdString());
        return;
    }
    
    // Update game stats
    updateGameStats(appId, name, duration);
    Logger::log()->info("Session recorded: AppID {} - Duration: {}s", appId, duration);
}

void Statistics::updateGameStats(int appId, const QString& name, qint64 duration) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    
    // Check if game stats exist
    query.prepare("SELECT totalIdleTime FROM game_stats WHERE appId = :appId");
    query.addBindValue(appId);
    
    if (query.exec() && query.next()) {
        // Update existing
        query.prepare("UPDATE game_stats SET totalIdleTime = totalIdleTime + :duration WHERE appId = :appId");
        query.addBindValue(duration);
        query.addBindValue(appId);
    } else {
        // Insert new
        query.prepare("INSERT INTO game_stats (appId, gameName, totalIdleTime) VALUES (:appId, :name, :duration)");
        query.addBindValue(appId);
        query.addBindValue(name);
        query.addBindValue(duration);
    }
    
    query.exec();
}

void Statistics::markGameCompleted(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("UPDATE game_stats SET completed = 1 WHERE appId = :appId");
    query.addBindValue(appId);
    query.exec();
    
    Logger::log()->info("Game marked as completed: AppID {}", appId);
    emit gameCompleted(appId);
}

GameStatistics Statistics::getGameStats(int appId) {
    GameStatistics stats;
    stats.appId = appId;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return stats;
    
    QSqlQuery query;
    query.prepare("SELECT gameName, totalIdleTime, cardsEarned, completed, achievements "
                  "FROM game_stats WHERE appId = :appId");
    query.addBindValue(appId);
    
    if (query.exec() && query.next()) {
        stats.gameName = query.value(0).toString();
        stats.totalIdleTime = query.value(1).toLongLong();
        stats.cardsEarned = query.value(2).toInt();
        stats.completed = query.value(3).toBool();
        stats.totalAchievements = query.value(4).toInt();
    }
    
    return stats;
}

QList<GameStatistics> Statistics::getAllGameStats() {
    QList<GameStatistics> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query("SELECT appId, gameName, totalIdleTime, cardsEarned, completed, achievements FROM game_stats");
    
    while (query.next()) {
        GameStatistics stats;
        stats.appId = query.value(0).toInt();
        stats.gameName = query.value(1).toString();
        stats.totalIdleTime = query.value(2).toLongLong();
        stats.cardsEarned = query.value(3).toInt();
        stats.completed = query.value(4).toBool();
        stats.totalAchievements = query.value(5).toInt();
        results.append(stats);
    }
    
    return results;
}

qint64 Statistics::getTotalIdleTime() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return 0;
    
    QSqlQuery query("SELECT SUM(totalIdleTime) FROM game_stats");
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

qint64 Statistics::getTotalSessions() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return 0;
    
    QSqlQuery query("SELECT COUNT(*) FROM sessions");
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}
qint64 Statistics::getTotalCardsEarned() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return 0;
    
    QSqlQuery query("SELECT SUM(cardsEarned) FROM game_stats");
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

int Statistics::getTotalGamesCompleted() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return 0;
    
    QSqlQuery query("SELECT COUNT(*) FROM game_stats WHERE completed = 1");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QList<GameStatistics> Statistics::getTopPerformers(int limit) {
    QList<GameStatistics> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query;
    query.prepare("SELECT appId, gameName, totalIdleTime, cardsEarned, completed, achievements "
                  "FROM game_stats ORDER BY cardsEarned DESC LIMIT :limit");
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            GameStatistics stats;
            stats.appId = query.value(0).toInt();
            stats.gameName = query.value(1).toString();
            stats.totalIdleTime = query.value(2).toLongLong();
            stats.cardsEarned = query.value(3).toInt();
            stats.completed = query.value(4).toBool();
            stats.totalAchievements = query.value(5).toInt();
            results.append(stats);
        }
    }
    
    return results;
}

QList<GameStatistics> Statistics::getMostPlayed(int limit) {
    QList<GameStatistics> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query;
    query.prepare("SELECT appId, gameName, totalIdleTime, cardsEarned, completed, achievements "
                  "FROM game_stats ORDER BY totalIdleTime DESC LIMIT :limit");
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            GameStatistics stats;
            stats.appId = query.value(0).toInt();
            stats.gameName = query.value(1).toString();
            stats.totalIdleTime = query.value(2).toLongLong();
            stats.cardsEarned = query.value(3).toInt();
            stats.completed = query.value(4).toBool();
            stats.totalAchievements = query.value(5).toInt();
            results.append(stats);
        }
    }
    
    return results;
}

QList<SessionStat> Statistics::getSessionHistory(int days) {
    QList<SessionStat> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query;
    query.prepare("SELECT startTime, endTime, duration, cardsEarned FROM sessions "
                  "WHERE startTime >= datetime('now', '-' || :days || ' days') "
                  "ORDER BY startTime DESC");
    query.addBindValue(days);
    
    if (query.exec()) {
        while (query.next()) {
            SessionStat stat;
            stat.startTime = query.value(0).toDateTime();
            stat.endTime = query.value(1).toDateTime();
            stat.duration = query.value(2).toLongLong();
            stat.cardsEarned = query.value(3).toInt();
            results.append(stat);
        }
    }
    
    return results;
}

QList<SessionStat> Statistics::getSessionHistoryForGame(int appId) {
    QList<SessionStat> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query;
    query.prepare("SELECT startTime, endTime, duration, cardsEarned FROM sessions "
                  "WHERE appId = :appId ORDER BY startTime DESC");
    query.addBindValue(appId);
    
    if (query.exec()) {
        while (query.next()) {
            SessionStat stat;
            stat.startTime = query.value(0).toDateTime();
            stat.endTime = query.value(1).toDateTime();
            stat.duration = query.value(2).toLongLong();
            stat.cardsEarned = query.value(3).toInt();
            results.append(stat);
        }
    }
    
    return results;
}

qint64 Statistics::getIdleTimeThisWeek() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return 0;
    
    QSqlQuery query("SELECT SUM(duration) FROM sessions "
                    "WHERE startTime >= datetime('now', '-7 days')");
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

qint64 Statistics::getIdleTimeThisMonth() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return 0;
    
    QSqlQuery query("SELECT SUM(duration) FROM sessions "
                    "WHERE startTime >= datetime('now', '-30 days')");
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

int Statistics::getSessionsThisWeek() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return 0;
    
    QSqlQuery query("SELECT COUNT(*) FROM sessions "
                    "WHERE startTime >= datetime('now', '-7 days')");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int Statistics::getSessionsThisMonth() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return 0;
    
    QSqlQuery query("SELECT COUNT(*) FROM sessions "
                    "WHERE startTime >= datetime('now', '-30 days')");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool Statistics::exportHistory(const QString& filePath) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return false;
    
    QJsonArray sessionsArray;
    QSqlQuery query("SELECT appId, gameName, startTime, endTime, duration, cardsEarned FROM sessions");
    
    while (query.next()) {
        QJsonObject obj;
        obj["appId"] = query.value(0).toInt();
        obj["gameName"] = query.value(1).toString();
        obj["startTime"] = query.value(2).toDateTime().toString(Qt::ISODate);
        obj["endTime"] = query.value(3).toDateTime().toString(Qt::ISODate);
        obj["duration"] = static_cast<int>(query.value(4).toLongLong());
        obj["cardsEarned"] = query.value(5).toInt();
        sessionsArray.append(obj);
    }
    
    QJsonObject root;
    root["sessions"] = sessionsArray;
    root["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
        Logger::log()->info("History exported to {}", filePath.toStdString());
        return true;
    }
    
    return false;
}

bool Statistics::importHistory(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) return false;
    
    QJsonArray sessions = doc.object()["sessions"].toArray();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return false;
    
    for (const auto& sessionVal : sessions) {
        QJsonObject obj = sessionVal.toObject();
        QSqlQuery query;
        query.prepare("INSERT INTO sessions (appId, gameName, startTime, endTime, duration, cardsEarned) "
                      "VALUES (:appId, :name, :start, :end, :duration, :cards)");
        query.addBindValue(obj["appId"].toInt());
        query.addBindValue(obj["gameName"].toString());
        query.addBindValue(obj["startTime"].toString());
        query.addBindValue(obj["endTime"].toString());
        query.addBindValue(obj["duration"].toInt());
        query.addBindValue(obj["cardsEarned"].toInt());
        query.exec();
    }
    
    Logger::log()->info("History imported from {}", filePath.toStdString());
    return true;
}

bool Statistics::backupSettings(const QString& filePath) {
    Logger::log()->info("Backing up settings to {}", filePath.toStdString());
    // Use QSettings to backup
    QSettings settings;
    QJsonObject backup;
    
    for (const auto& key : settings.allKeys()) {
        backup[key] = QJsonValue::fromVariant(settings.value(key));
    }
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(backup).toJson());
        file.close();
        return true;
    }
    return false;
}

bool Statistics::restoreSettings(const QString& filePath) {
    Logger::log()->info("Restoring settings from {}", filePath.toStdString());
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) return false;
    
    QSettings settings;
    for (auto it = doc.object().constBegin(); it != doc.object().constEnd(); ++it) {
        settings.setValue(it.key(), it.value().toVariant());
    }
    
    return true;
}
