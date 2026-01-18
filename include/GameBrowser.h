#ifndef GAMEBROWSER_H
#define GAMEBROWSER_H

#include <QMainWindow>
#include <QTimer>
#include <QLineEdit>
#include <QSet>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QCheckBox>
#include <QList>

// Forward declarations
class QMenu;
class QMenuBar;
class QAction;
class QCloseEvent;
class QComboBox;

#include "ProcessManager.h"
#include "GameSearch.h"
#include "SettingsDialog.h"
#include "SteamArtFetcher.h"
#include "UpdateChecker.h"

class GameBrowser : public QMainWindow {
    Q_OBJECT

public:
    explicit GameBrowser(GameSearch* engine, QWidget *parent = nullptr);
    ~GameBrowser();

private slots:
    // View Management
    void refreshView();
    void updateTimers();
    void showSettings();
    void onSettingsChanged();
    void retranslateUi();
    void showContextMenu(const QPoint &pos);
    
    // Art Loading
    void onArtLoaded(int appId, const GameArt& art);
    void onArtLoadFailed(int appId, const QString& error);
    
    // Updates
    void onUpdateAvailable(const QString& version, const QString& changelog, const QUrl& downloadUrl);
    
    // Batch Selection
    void onBatchSelectionChanged();
    void startBatchIdle();
    void stopBatchIdle();
    void pauseBatchIdle();
    void clearBatchSelection();
    
    // Game Control
    void onGameCardAction(int appId);
    void onGameCardPause(int appId);
    
    // Search
    void onSearchTextChanged(const QString& text);
    
    // Session Management
    void onSessionsChanged();
    
    // Session Controls
    void onPauseAllClicked();
    void onResumeAllClicked();
    void onStopAllClicked();
    
    // Keyboard and Favorites
    void onSearchBarFocus();
    void loadSearchHistory();
    void saveSearHistory();
    void loadFavorites();
    void saveFavorites();

private:
    // Setup
    void setupUi();
    void setupMenuBar();
    void setupModernTheme();
    void setupKeyboardShortcuts();
    void createLeftPanel();
    void createMainContentArea();
    void createBottomBar();
    
    // UI Creation
    QWidget* createGameCard(const GameResult& game, bool isRunning, bool isFavorite = false);
    QWidget* createSessionCard(const IdleSession& session);
    void updateGameCardState(int appId);
    void clearLayout(QLayout* layout);
    
    // Utilities
    QString formatDuration(qint64 seconds) const;
    void updateSessionStats();

    // --- Core Components ---
    GameSearch* m_engine;
    ProcessManager* m_procManager;
    SettingsDialog* m_settingsDialog = nullptr;
    SteamArtFetcher* m_artFetcher;
    UpdateChecker* m_updateChecker;

    // --- Menu Bar ---
    QMenuBar* m_menuBar = nullptr;
    QMenu* m_fileMenu = nullptr;
    QAction* m_refreshAction = nullptr;
    QAction* m_settingsAction = nullptr;
    QAction* m_exitAction = nullptr;
    QAction* m_sortAscAction = nullptr;
    QAction* m_sortDescAction = nullptr;
    QAction* m_showFavoritesAction = nullptr;
    QAction* m_showRecentAction = nullptr;

    // --- Left Sidebar (Active Sessions) ---
    QWidget* m_leftPanel = nullptr;
    QLabel* m_activeHeaderLabel = nullptr;
    QVBoxLayout* m_activeSessionsLayout = nullptr;
    QScrollArea* m_activeSessionsScroll = nullptr;
    QLabel* m_sessionStatsLabel = nullptr;
    
    // Session control buttons
    QPushButton* m_btnPauseAll = nullptr;
    QPushButton* m_btnResumeAll = nullptr;
    QPushButton* m_btnStopAll = nullptr;

    // --- Main Content Area ---
    QWidget* m_contentWidget = nullptr;
    
    // Search & Filter Bar
    QLineEdit* m_searchBar = nullptr;
    QComboBox* m_filterCombo = nullptr;
    QLabel* m_resultCountLabel = nullptr;
    
    // Game Results
    QVBoxLayout* m_resultsLayout = nullptr;
    QScrollArea* m_resultsScroll = nullptr;
    
    // --- Bottom Bar ---
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_artLoadProgress = nullptr;
    
    // --- Batch Selection & Actions ---
    QWidget* m_batchActionBar = nullptr;
    QCheckBox* m_selectAllCheckBox = nullptr;
    QLabel* m_batchStatusLabel = nullptr;
    QPushButton* m_btnBatchIdle = nullptr;
    QPushButton* m_btnBatchPause = nullptr;
    QPushButton* m_btnBatchStop = nullptr;
    QPushButton* m_btnBatchClear = nullptr;
    QSet<int> m_selectedGames;
    
    // --- Storage ---
    QMap<int, QWidget*> m_gameCards;
    QMap<int, QWidget*> m_sessionCards;

    // --- Timers ---
    QTimer* m_uiUpdateTimer;     
    QTimer* m_searchDebounceTimer;
    
    // --- State ---
    QList<GameResult> m_currentSearchResults;
    
    // --- Sorting & Filtering ---
    enum class SortOrder {
        Default,
        NameAsc,
        NameDesc
    };
    SortOrder m_sortOrder = SortOrder::Default;
    bool m_showOnlyFavorites = false;
    bool m_showOnlyRecent = false;
    
    // --- Favorites & History ---
    QSet<int> m_favoriteGames;
    QList<int> m_recentGames;
    QStringList m_searchHistory;

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // GAMEBROWSER_H