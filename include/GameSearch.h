#ifndef GAMESEARCH_H
#define GAMESEARCH_H

#include <QString>
#include <QVector>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <algorithm>
#include "Logger.h"

struct GameResult {
    int id;
    QString name;
    int weight = 0;
};

class GameSearch {
public:
    static GameSearch& instance() {
        static GameSearch inst;
        return inst;
    }

    // Loads DB into memory once for instant searching
    bool init(const QString& dbPath) {
        m_data.clear();
        
        // Scope for DB connection to ensure it closes properly
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "SearchInit");
            db.setDatabaseName(dbPath);
            if (!db.open()) {
                Logger::log()->error("Search DB failed to open: {}", db.lastError().text().toStdString());
                return false;
            }

            QSqlQuery q("SELECT appid, name FROM games", db);
            m_data.reserve(150000); // Pre-allocate for Steam's massive library

            while (q.next()) {
                m_data.append({
                    q.value(0).toInt(), 
                    q.value(1).toString()
                });
            }
        }
        QSqlDatabase::removeDatabase("SearchInit");
        return !m_data.isEmpty();
    }

    QVector<GameResult> search(const QString& query, int limit = 100) {
        QString term = query.trimmed().toLower();
        if (term.isEmpty()) return getDefault(limit);

        QVector<GameResult> matches;
        bool isId;
        int targetId = term.toInt(&isId);

        for (const auto& g : m_data) {
            int score = 0;
            QString nameLower = g.name.toLower();

            // Tier 1: Exact AppID Match (Absolute Priority)
            if (isId && g.id == targetId) {
                score = 1000;
            } 
            // Tier 2: Exact Name Match
            else if (nameLower == term) {
                score = 800;
            }
            // Tier 3: Starts With
            else if (nameLower.startsWith(term)) {
                score = 500 - (nameLower.length() - term.length());
            }
            // Tier 4: Contains
            else if (nameLower.contains(term)) {
                score = 200 - (nameLower.length() - term.length());
            }
            // Tier 5: Fuzzy Match (Only for queries longer than 3 chars)
            else if (term.length() > 3) {
                int dist = levenshteinDistance(term, nameLower);
                // Allow 1 mistake for every 4 letters
                int maxErrors = term.length() / 4 + 1; 
                if (dist <= maxErrors) {
                    score = 100 - dist;
                }
            }

            if (score > 0) {
                GameResult res = g;
                res.weight = score;
                matches.append(res);
                // Safety cap: don't process more than 500 matches to keep sort fast
                if (matches.size() >= 500) break; 
            }
        }

        std::sort(matches.begin(), matches.end(), [](const GameResult& a, const GameResult& b) {
            if (a.weight != b.weight) return a.weight > b.weight;
            return a.name < b.name; 
        });

        if (matches.size() > limit) matches.resize(limit);
        return matches;
    }

    // Forces a refresh of the in-memory cache from the DB
    void reload(const QString& dbPath) { init(dbPath); }

private:
    GameSearch() = default;

    QVector<GameResult> getDefault(int limit) {
        if (m_data.isEmpty()) return {};
        int count = std::min(limit, (int)m_data.size());
        return m_data.mid(0, count);
    }

    QVector<GameResult> m_data;

    int levenshteinDistance(const QString& s1, const QString& s2) {
        const int m = s1.length();
        const int n = s2.length();
        
        // We only care about the first part of s2 if it's much longer than s1
        // to prevent "Counter-Strike" matching "C" via fuzzy logic
        if (qAbs(m - n) > 3) return 99; 

        QVector<int> col(n + 1);
        QVector<int> prevCol(n + 1);

        for (int i = 0; i <= n; i++) prevCol[i] = i;

        for (int i = 0; i < m; i++) {
            col[0] = i + 1;
            for (int j = 0; j < n; j++) {
                int cost = (s1[i] == s2[j]) ? 0 : 1;
                col[j + 1] = qMin(qMin(col[j] + 1, prevCol[j + 1] + 1), prevCol[j] + cost);
            }
            col.swap(prevCol);
        }
        return prevCol[n];
    }
};

// Global helper for the Steam API sync process
inline bool updateAppDatabase(const QVariantList& apps) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    db.transaction();

    // Use TRUNCATE logic for SQLite
    q.exec("DELETE FROM games");
    q.exec("DELETE FROM sqlite_sequence WHERE name='games'");

    q.prepare("INSERT INTO games (appid, name) VALUES (:id, :name)");
    
    for (const QVariant& v : apps) {
        QVariantMap app = v.toMap();
        QString name = app["name"].toString();
        if (name.isEmpty()) continue; // Skip unnamed apps (junk data)

        q.bindValue(":id", app["appid"]);
        q.bindValue(":name", name);
        q.exec();
    }

    bool success = db.commit();
    if (success) {
        // Trigger the search engine to reload the new data immediately
        GameSearch::instance().reload(db.databaseName());
    }
    return success;
}

#endif