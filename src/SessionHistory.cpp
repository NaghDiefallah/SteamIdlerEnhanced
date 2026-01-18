#include "SessionHistory.h"
#include <QSqlRecord>
#include <QVariant>
#include <QDir>
#include <QStandardPaths>

SessionHistory::SessionHistory(QObject* parent)
    : QObject(parent)
    , m_cleanupTimer(new QTimer(this))
{
    // Cleanup old records every 24 hours
    m_cleanupTimer->setInterval(24 * 60 * 60 * 1000);
    connect(m_cleanupTimer, &QTimer::timeout, [this]() {
        cleanupOldRecords();
    });
    m_cleanupTimer->start();

    Logger::log()->info("Session History initialized");
}

SessionHistory::~SessionHistory() {
    m_cleanupTimer->stop();
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool SessionHistory::init(const QString& dbPath) {
    QMutexLocker locker(&m_dbMutex);

    m_db = QSqlDatabase::addDatabase("QSQLITE", "SessionHistory");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        Logger::log()->error("Failed to open session history DB: {}", m_db.lastError().text().toStdString());
        return false;
    }

    return createTables();
}

bool SessionHistory::createTables() {
    // Execute each SQL statement separately (SQLite doesn't support multiple statements in one exec)
    QSqlQuery query(m_db);
    
    // Create table
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            app_id INTEGER NOT NULL,
            game_name TEXT NOT NULL,
            start_time DATETIME NOT NULL,
            end_time DATETIME,
            duration INTEGER DEFAULT 0,
            status TEXT NOT NULL DEFAULT 'active'
        )
    )";
    
    if (!query.exec(createTable)) {
        Logger::log()->error("Failed to create sessions table: {}", query.lastError().text().toStdString());
        return false;
    }
    
    // Create indexes separately
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_app_id ON sessions(app_id)")) {
        Logger::log()->error("Failed to create idx_app_id: {}", query.lastError().text().toStdString());
        return false;
    }
    
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_start_time ON sessions(start_time)")) {
        Logger::log()->error("Failed to create idx_start_time: {}", query.lastError().text().toStdString());
        return false;
    }
    
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_status ON sessions(status)")) {
        Logger::log()->error("Failed to create idx_status: {}", query.lastError().text().toStdString());
        return false;
    }

    Logger::log()->info("Session history tables created successfully");
    return true;
}

void SessionHistory::logSessionStart(int appId, const QString& gameName) {
    QVariantMap params;
    params["app_id"] = appId;
    params["game_name"] = gameName;
    params["start_time"] = QDateTime::currentDateTime();
    params["status"] = "active";

    QString query = "INSERT INTO sessions (app_id, game_name, start_time, status) VALUES (:app_id, :game_name, :start_time, :status)";

    if (executeQuery(query, params)) {
        Logger::log()->info("Session started: {} (AppID: {})", gameName.toStdString(), appId);
    }
}

void SessionHistory::logSessionEnd(int appId, qint64 duration) {
    QVariantMap params;
    params["app_id"] = appId;
    params["end_time"] = QDateTime::currentDateTime();
    params["duration"] = duration;
    params["status"] = "completed";

    QString query = "UPDATE sessions SET end_time = :end_time, duration = :duration, status = :status WHERE app_id = :app_id AND status = 'active'";

    if (executeQuery(query, params)) {
        Logger::log()->info("Session ended: AppID {} - Duration: {} seconds", appId, duration);
    }
}

void SessionHistory::logSessionStop(int appId, qint64 duration) {
    QVariantMap params;
    params["app_id"] = appId;
    params["end_time"] = QDateTime::currentDateTime();
    params["duration"] = duration;
    params["status"] = "stopped";

    QString query = "UPDATE sessions SET end_time = :end_time, duration = :duration, status = :status WHERE app_id = :app_id AND status = 'active'";

    if (executeQuery(query, params)) {
        Logger::log()->info("Session stopped: AppID {} - Duration: {} seconds", appId, duration);
    }
}

QList<SessionRecord> SessionHistory::getRecentSessions(int limit) {
    QString query = "SELECT * FROM sessions ORDER BY start_time DESC LIMIT :limit";
    QVariantMap params;
    params["limit"] = limit;

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(query);
    for (auto it = params.begin(); it != params.end(); ++it) {
        sqlQuery.bindValue(":" + it.key(), it.value());
    }

    QList<SessionRecord> records;
    if (sqlQuery.exec()) {
        records = queryToRecords(sqlQuery);
    }

    return records;
}

QList<SessionRecord> SessionHistory::getSessionsForGame(int appId, int limit) {
    QString query = "SELECT * FROM sessions WHERE app_id = :app_id ORDER BY start_time DESC LIMIT :limit";
    QVariantMap params;
    params["app_id"] = appId;
    params["limit"] = limit;

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(query);
    for (auto it = params.begin(); it != params.end(); ++it) {
        sqlQuery.bindValue(":" + it.key(), it.value());
    }

    QList<SessionRecord> records;
    if (sqlQuery.exec()) {
        records = queryToRecords(sqlQuery);
    }

    return records;
}

qint64 SessionHistory::getTotalPlayTime(int appId) {
    QString query;
    QVariantMap params;

    if (appId == -1) {
        query = "SELECT SUM(duration) FROM sessions WHERE status IN ('completed', 'stopped')";
    } else {
        query = "SELECT SUM(duration) FROM sessions WHERE app_id = :app_id AND status IN ('completed', 'stopped')";
        params["app_id"] = appId;
    }

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(query);
    for (auto it = params.begin(); it != params.end(); ++it) {
        sqlQuery.bindValue(":" + it.key(), it.value());
    }

    if (sqlQuery.exec() && sqlQuery.next()) {
        return sqlQuery.value(0).toLongLong();
    }

    return 0;
}

void SessionHistory::cleanupOldRecords(int daysToKeep) {
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-daysToKeep);

    QString query = "DELETE FROM sessions WHERE start_time < :cutoff AND status != 'active'";
    QVariantMap params;
    params["cutoff"] = cutoff;

    if (executeQuery(query, params)) {
        Logger::log()->info("Cleaned up old session records older than {} days", daysToKeep);
    }
}

bool SessionHistory::executeQuery(const QString& query, const QVariantMap& params) {
    QMutexLocker locker(&m_dbMutex);

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(query);

    for (auto it = params.begin(); it != params.end(); ++it) {
        sqlQuery.bindValue(":" + it.key(), it.value());
    }

    if (!sqlQuery.exec()) {
        Logger::log()->error("Query execution failed: {} - Error: {}", query.toStdString(), sqlQuery.lastError().text().toStdString());
        return false;
    }

    return true;
}

QList<SessionRecord> SessionHistory::queryToRecords(QSqlQuery& query) {
    QList<SessionRecord> records;

    while (query.next()) {
        SessionRecord record;
        record.id = query.value("id").toInt();
        record.appId = query.value("app_id").toInt();
        record.gameName = query.value("game_name").toString();
        record.startTime = query.value("start_time").toDateTime();
        record.endTime = query.value("end_time").toDateTime();
        record.duration = query.value("duration").toLongLong();
        record.status = query.value("status").toString();

        records.append(record);
    }

    return records;
}