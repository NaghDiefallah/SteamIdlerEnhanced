#ifndef CATEGORIES_H
#define CATEGORIES_H

#include <QString>
#include <QList>
#include <QMap>
#include <QObject>
#include <QColor>

struct GameTag {
    QString name;
    QColor color;
    int gameCount = 0;
};

struct GameCategory {
    QString id;
    QString name;
    QString description;
    QColor color;
    int gameCount = 0;
    bool isDefault = false;
};

class Categories : public QObject {
    Q_OBJECT

public:
    static Categories& instance() {
        static Categories inst;
        return inst;
    }

    bool init(const QString& dbPath);
    
    // Categories
    void createCategory(const QString& id, const QString& name, const QColor& color = Qt::gray);
    void deleteCategory(const QString& id);
    void updateCategory(const QString& id, const QString& name, const QColor& color);
    GameCategory getCategory(const QString& id);
    QList<GameCategory> getAllCategories();
    void setCategoryForGame(int appId, const QString& categoryId);
    QString getCategoryForGame(int appId);
    
    // Tags
    void createTag(const QString& tagName, const QColor& color = Qt::gray);
    void deleteTag(const QString& tagName);
    void updateTag(const QString& tagName, const QColor& color);
    GameTag getTag(const QString& tagName);
    QList<GameTag> getAllTags();
    void addTagToGame(int appId, const QString& tagName);
    void removeTagFromGame(int appId, const QString& tagName);
    QStringList getTagsForGame(int appId);
    QList<int> getGamesWithTag(const QString& tagName);
    
    // Filtering
    QList<int> getGamesInCategory(const QString& categoryId);
    QList<int> getGamesWithMultipleTags(const QStringList& tags);  // AND filter
    
    // Game organization
    void hideGame(int appId);
    void showGame(int appId);
    bool isGameHidden(int appId);
    QList<int> getHiddenGames();
    
    void pinGame(int appId);
    void unpinGame(int appId);
    bool isGamePinned(int appId);
    QList<int> getPinnedGames();
    
    // Favorites
    void addFavorite(int appId);
    void removeFavorite(int appId);
    bool isFavorite(int appId);
    QList<int> getFavorites();

signals:
    void categoryCreated(const QString& id);
    void categoryDeleted(const QString& id);
    void tagCreated(const QString& tagName);
    void tagDeleted(const QString& tagName);
    void gameTagged(int appId, const QString& tagName);
    void gameOrganized(int appId);
    void gameFavorited(int appId);

private:
    Categories(QObject* parent = nullptr);
    ~Categories();
    
    void createDefaultCategories();
    void createTables();
    QString m_dbPath;
};

#endif
