#include "Categories.h"
#include "Logger.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>

Categories::Categories(QObject* parent) : QObject(parent) {
    Logger::log()->info("Categories initialized");
}

Categories::~Categories() {
}

bool Categories::init(const QString& dbPath) {
    m_dbPath = dbPath;
    createTables();
    createDefaultCategories();
    Logger::log()->info("Categories database initialized: {}", dbPath.toStdString());
    return true;
}

void Categories::createTables() {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    
    // Categories table
    if (!query.exec("CREATE TABLE IF NOT EXISTS categories ("
                   "id TEXT PRIMARY KEY,"
                   "name TEXT NOT NULL,"
                   "color TEXT DEFAULT '#0078D4',"
                   "description TEXT,"
                   "gameCount INTEGER DEFAULT 0)")) {
        Logger::log()->error("Failed to create categories table: {}", query.lastError().text().toStdString());
    }
    
    // Tags table
    if (!query.exec("CREATE TABLE IF NOT EXISTS tags ("
                   "name TEXT PRIMARY KEY,"
                   "color TEXT DEFAULT '#9C27B0',"
                   "gameCount INTEGER DEFAULT 0)")) {
        Logger::log()->error("Failed to create tags table: {}", query.lastError().text().toStdString());
    }
    
    // Game categories junction table
    if (!query.exec("CREATE TABLE IF NOT EXISTS game_categories ("
                   "appId INTEGER,"
                   "categoryId TEXT,"
                   "PRIMARY KEY (appId, categoryId))")) {
        Logger::log()->error("Failed to create game_categories table: {}", query.lastError().text().toStdString());
    }
    
    // Game tags junction table
    if (!query.exec("CREATE TABLE IF NOT EXISTS game_tags ("
                   "appId INTEGER,"
                   "tagName TEXT,"
                   "PRIMARY KEY (appId, tagName))")) {
        Logger::log()->error("Failed to create game_tags table: {}", query.lastError().text().toStdString());
    }
    
    // Game organization table
    if (!query.exec("CREATE TABLE IF NOT EXISTS game_organization ("
                   "appId INTEGER PRIMARY KEY,"
                   "isFavorite BOOLEAN DEFAULT 0,"
                   "isPinned BOOLEAN DEFAULT 0,"
                   "isHidden BOOLEAN DEFAULT 0)")) {
        Logger::log()->error("Failed to create game_organization table: {}", query.lastError().text().toStdString());
    }
    
    // Create indices
    query.exec("CREATE INDEX IF NOT EXISTS idx_game_categories_appId ON game_categories(appId)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_game_tags_appId ON game_tags(appId)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_organization_favorite ON game_organization(isFavorite)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_organization_pinned ON game_organization(isPinned)");
}

void Categories::createDefaultCategories() {
    QStringList defaults = {"RPG", "Action", "Strategy", "Adventure", "Puzzle", "Simulation", "Sports"};
    QStringList colors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#FFA07A", "#98D8C8", "#F7DC6F", "#BB8FCE"};
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    for (int i = 0; i < defaults.size(); ++i) {
        QString id = defaults[i].toLower();
        QSqlQuery query;
        query.prepare("INSERT OR IGNORE INTO categories (id, name, color) VALUES (:id, :name, :color)");
        query.addBindValue(id);
        query.addBindValue(defaults[i]);
        query.addBindValue(colors[i]);
        query.exec();
    }
}

void Categories::createCategory(const QString& id, const QString& name, const QColor& color) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("INSERT INTO categories (id, name, color) VALUES (:id, :name, :color)");
    query.addBindValue(id);
    query.addBindValue(name);
    query.addBindValue(color.name());
    
    if (query.exec()) {
        Logger::log()->info("Category created: {}", id.toStdString());
        emit categoryCreated(id);
    }
}

void Categories::deleteCategory(const QString& id) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("DELETE FROM categories WHERE id = :id");
    query.addBindValue(id);
    query.exec();
    
    emit categoryDeleted(id);
}

void Categories::updateCategory(const QString& id, const QString& name, const QColor& color) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("UPDATE categories SET name = :name, color = :color WHERE id = :id");
    query.addBindValue(name);
    query.addBindValue(color.name());
    query.addBindValue(id);
    query.exec();
    
    Logger::log()->info("Category updated: {}", id.toStdString());
}

GameCategory Categories::getCategory(const QString& id) {
    GameCategory cat;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return cat;
    
    QSqlQuery query;
    query.prepare("SELECT id, name, color, description, gameCount FROM categories WHERE id = :id");
    query.addBindValue(id);
    
    if (query.exec() && query.next()) {
        cat.id = query.value(0).toString();
        cat.name = query.value(1).toString();
        cat.color = QColor(query.value(2).toString());
        cat.description = query.value(3).toString();
        cat.gameCount = query.value(4).toInt();
    }
    
    return cat;
}

QList<GameCategory> Categories::getAllCategories() {
    QList<GameCategory> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query("SELECT id, name, color, description, gameCount FROM categories");
    
    while (query.next()) {
        GameCategory cat;
        cat.id = query.value(0).toString();
        cat.name = query.value(1).toString();
        cat.color = QColor(query.value(2).toString());
        cat.description = query.value(3).toString();
        cat.gameCount = query.value(4).toInt();
        results.append(cat);
    }
    
    return results;
}

void Categories::setCategoryForGame(int appId, const QString& categoryId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    // First remove any existing category
    QSqlQuery query;
    query.prepare("DELETE FROM game_categories WHERE appId = :appId");
    query.addBindValue(appId);
    query.exec();
    
    // Add new category
    query.prepare("INSERT INTO game_categories (appId, categoryId) VALUES (:appId, :categoryId)");
    query.addBindValue(appId);
    query.addBindValue(categoryId);
    query.exec();
    
    Logger::log()->info("Set category {} for app {}", categoryId.toStdString(), appId);
}

QString Categories::getCategoryForGame(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return QString();
    
    QSqlQuery query;
    query.prepare("SELECT categoryId FROM game_categories WHERE appId = :appId LIMIT 1");
    query.addBindValue(appId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

void Categories::createTag(const QString& tagName, const QColor& color) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO tags (name, color) VALUES (:name, :color)");
    query.addBindValue(tagName);
    query.addBindValue(color.name());
    query.exec();
    
    Logger::log()->info("Tag created: {}", tagName.toStdString());
    emit tagCreated(tagName);
}

void Categories::deleteTag(const QString& tagName) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("DELETE FROM tags WHERE name = :name");
    query.addBindValue(tagName);
    query.exec();
    
    emit tagDeleted(tagName);
}

void Categories::updateTag(const QString& tagName, const QColor& color) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("UPDATE tags SET color = :color WHERE name = :name");
    query.addBindValue(color.name());
    query.addBindValue(tagName);
    query.exec();
}

GameTag Categories::getTag(const QString& tagName) {
    GameTag tag;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return tag;
    
    QSqlQuery query;
    query.prepare("SELECT name, color, gameCount FROM tags WHERE name = :name");
    query.addBindValue(tagName);
    
    if (query.exec() && query.next()) {
        tag.name = query.value(0).toString();
        tag.color = QColor(query.value(1).toString());
        tag.gameCount = query.value(2).toInt();
    }
    
    return tag;
}

QList<GameTag> Categories::getAllTags() {
    QList<GameTag> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query("SELECT name, color, gameCount FROM tags");
    
    while (query.next()) {
        GameTag tag;
        tag.name = query.value(0).toString();
        tag.color = QColor(query.value(1).toString());
        tag.gameCount = query.value(2).toInt();
        results.append(tag);
    }
    
    return results;
}

void Categories::addTagToGame(int appId, const QString& tagName) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO game_tags (appId, tagName) VALUES (:appId, :tagName)");
    query.addBindValue(appId);
    query.addBindValue(tagName);
    query.exec();
    
    Logger::log()->info("Added tag {} to app {}", tagName.toStdString(), appId);
    emit gameTagged(appId, tagName);
}

void Categories::removeTagFromGame(int appId, const QString& tagName) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("DELETE FROM game_tags WHERE appId = :appId AND tagName = :tagName");
    query.addBindValue(appId);
    query.addBindValue(tagName);
    query.exec();
    
    Logger::log()->info("Removed tag {} from app {}", tagName.toStdString(), appId);
}

QStringList Categories::getTagsForGame(int appId) {
    QStringList results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query;
    query.prepare("SELECT tagName FROM game_tags WHERE appId = :appId");
    query.addBindValue(appId);
    
    if (query.exec()) {
        while (query.next()) {
            results.append(query.value(0).toString());
        }
    }
    
    return results;
}

QList<int> Categories::getGamesWithTag(const QString& tagName) {
    QList<int> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query;
    query.prepare("SELECT appId FROM game_tags WHERE tagName = :tagName");
    query.addBindValue(tagName);
    
    if (query.exec()) {
        while (query.next()) {
            results.append(query.value(0).toInt());
        }
    }
    
    return results;
}

QList<int> Categories::getGamesInCategory(const QString& categoryId) {
    QList<int> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query;
    query.prepare("SELECT appId FROM game_categories WHERE categoryId = :categoryId");
    query.addBindValue(categoryId);
    
    if (query.exec()) {
        while (query.next()) {
            results.append(query.value(0).toInt());
        }
    }
    
    return results;
}

QList<int> Categories::getGamesWithMultipleTags(const QStringList& tags) {
    QList<int> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    if (tags.isEmpty()) return results;
    
    QString placeholders = QString("'%1'").arg(tags.join("','"));
    QSqlQuery query;
    query.exec(QString("SELECT appId FROM game_tags WHERE tagName IN (%1) "
                       "GROUP BY appId HAVING COUNT(*) = %2")
               .arg(placeholders).arg(tags.size()));
    
    while (query.next()) {
        results.append(query.value(0).toInt());
    }
    
    return results;
}

void Categories::hideGame(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO game_organization (appId, isHidden) "
                  "VALUES (:appId, 1)");
    query.addBindValue(appId);
    query.exec();
    
    Logger::log()->info("Game hidden: {}", appId);
}

void Categories::showGame(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("UPDATE game_organization SET isHidden = 0 WHERE appId = :appId");
    query.addBindValue(appId);
    query.exec();
    
    Logger::log()->info("Game shown: {}", appId);
}

bool Categories::isGameHidden(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return false;
    
    QSqlQuery query;
    query.prepare("SELECT isHidden FROM game_organization WHERE appId = :appId");
    query.addBindValue(appId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toBool();
    }
    return false;
}

QList<int> Categories::getHiddenGames() {
    QList<int> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query("SELECT appId FROM game_organization WHERE isHidden = 1");
    
    while (query.next()) {
        results.append(query.value(0).toInt());
    }
    
    return results;
}

void Categories::pinGame(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO game_organization (appId, isPinned) "
                  "VALUES (:appId, 1)");
    query.addBindValue(appId);
    query.exec();
    
    Logger::log()->info("Game pinned: {}", appId);
}

void Categories::unpinGame(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("UPDATE game_organization SET isPinned = 0 WHERE appId = :appId");
    query.addBindValue(appId);
    query.exec();
    
    Logger::log()->info("Game unpinned: {}", appId);
}

bool Categories::isGamePinned(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return false;
    
    QSqlQuery query;
    query.prepare("SELECT isPinned FROM game_organization WHERE appId = :appId");
    query.addBindValue(appId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toBool();
    }
    return false;
}

QList<int> Categories::getPinnedGames() {
    QList<int> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query("SELECT appId FROM game_organization WHERE isPinned = 1 ORDER BY appId");
    
    while (query.next()) {
        results.append(query.value(0).toInt());
    }
    
    return results;
}

void Categories::addFavorite(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO game_organization (appId, isFavorite) "
                  "VALUES (:appId, 1)");
    query.addBindValue(appId);
    query.exec();
    
    Logger::log()->info("Game added to favorites: {}", appId);
    emit gameFavorited(appId);
}

void Categories::removeFavorite(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;
    
    QSqlQuery query;
    query.prepare("UPDATE game_organization SET isFavorite = 0 WHERE appId = :appId");
    query.addBindValue(appId);
    query.exec();
    
    Logger::log()->info("Game removed from favorites: {}", appId);
}

bool Categories::isFavorite(int appId) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return false;
    
    QSqlQuery query;
    query.prepare("SELECT isFavorite FROM game_organization WHERE appId = :appId");
    query.addBindValue(appId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toBool();
    }
    return false;
}

QList<int> Categories::getFavorites() {
    QList<int> results;
    
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return results;
    
    QSqlQuery query("SELECT appId FROM game_organization WHERE isFavorite = 1 ORDER BY appId");
    
    while (query.next()) {
        results.append(query.value(0).toInt());
    }
    
    return results;
}
