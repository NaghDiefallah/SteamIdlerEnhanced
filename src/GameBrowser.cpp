#include "GameBrowser.h"
#include "AppConfig.h"
#include "Logger.h"
#include "ProcessManager.h"
#include "GameSearch.h"
#include "Lang.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QDialog>
#include <QDateTime>
#include <QMenuBar>
#include <QAction>
#include <QTimer>
#include <QFile>
#include <QMenu>
#include <QStatusBar>
#include <QCloseEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QCheckBox>
#include <QComboBox>
#include <QProgressBar>
#include <QFrame>
#include <QSpacerItem>
#include <QPixmap>
#include <QStyle>
#include <QShortcut>
#include <QSettings>

const int SIDEBAR_WIDTH = 340;
const int CARD_HEIGHT = 85;      // Fixed from 30 to proper card height
const int CARD_SPACING = 6;

GameBrowser::GameBrowser(GameSearch* engine, QWidget *parent) 
    : QMainWindow(parent), m_engine(engine) 
{
    Logger::log()->info("Initializing GameBrowser with modern fluent design");
    
    m_procManager = new ProcessManager(this);
    m_artFetcher = &SteamArtFetcher::instance();
    m_updateChecker = &UpdateChecker::instance();
    
    // Core Logic Connections
    connect(m_procManager, &ProcessManager::sessionsChanged, this, &GameBrowser::onSessionsChanged);
    connect(m_artFetcher, &SteamArtFetcher::artLoaded, this, &GameBrowser::onArtLoaded);
    connect(m_artFetcher, &SteamArtFetcher::artLoadFailed, this, &GameBrowser::onArtLoadFailed);
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this, &GameBrowser::onUpdateAvailable);

    // UI Update Timer
    m_uiUpdateTimer = new QTimer(this);
    connect(m_uiUpdateTimer, &QTimer::timeout, this, &GameBrowser::updateTimers);
    m_uiUpdateTimer->start(1000);

    // Search Debouncer
    m_searchDebounceTimer = new QTimer(this);
    m_searchDebounceTimer->setSingleShot(true);
    m_searchDebounceTimer->setInterval(300);
    connect(m_searchDebounceTimer, &QTimer::timeout, this, &GameBrowser::refreshView);

    setupUi();
    setupModernTheme();
    setupKeyboardShortcuts();
    
    setWindowTitle("Steam Idler Enhanced");
    setMinimumSize(900, 600);
    resize(1280, 800);
    
    // Load search history and favorites from config
    loadSearchHistory();
    loadFavorites();
    
    refreshView();
}

GameBrowser::~GameBrowser() {
    m_uiUpdateTimer->stop();
    m_searchDebounceTimer->stop();
}

void GameBrowser::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    setMenuBar(m_menuBar);
    
    // File Menu
    m_fileMenu = m_menuBar->addMenu("File");
    m_refreshAction = m_fileMenu->addAction("Refresh (Ctrl+R)");
    m_fileMenu->addSeparator();
    m_settingsAction = m_fileMenu->addAction("Settings (Ctrl+,)");
    m_fileMenu->addSeparator();
    m_exitAction = m_fileMenu->addAction("Exit");
    
    // View Menu
    auto* viewMenu = m_menuBar->addMenu("View");
    m_sortAscAction = viewMenu->addAction("Sort by Name (A-Z)");
    m_sortDescAction = viewMenu->addAction("Sort by Name (Z-A)");
    viewMenu->addSeparator();
    m_showFavoritesAction = viewMenu->addAction("Show Favorites Only");
    m_showFavoritesAction->setCheckable(true);
    viewMenu->addSeparator();
    m_showRecentAction = viewMenu->addAction("Show Recently Played");
    m_showRecentAction->setCheckable(true);
    
    connect(m_refreshAction, &QAction::triggered, this, &GameBrowser::refreshView);
    connect(m_settingsAction, &QAction::triggered, this, &GameBrowser::showSettings);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    connect(m_sortAscAction, &QAction::triggered, this, [this]() {
        m_sortOrder = SortOrder::NameAsc;
        refreshView();
    });
    connect(m_sortDescAction, &QAction::triggered, this, [this]() {
        m_sortOrder = SortOrder::NameDesc;
        refreshView();
    });
    connect(m_showFavoritesAction, &QAction::triggered, this, [this](bool checked) {
        m_showOnlyFavorites = checked;
        refreshView();
    });
    connect(m_showRecentAction, &QAction::triggered, this, [this](bool checked) {
        m_showOnlyRecent = checked;
        refreshView();
    });
    
    retranslateUi();
}

void GameBrowser::setupKeyboardShortcuts() {
    // Ctrl+F to focus search
    auto* sc1 = new QShortcut(Qt::CTRL | Qt::Key_F, this);
    connect(sc1, &QShortcut::activated, this, &GameBrowser::onSearchBarFocus);
    
    // Ctrl+R to refresh
    auto* sc2 = new QShortcut(Qt::CTRL | Qt::Key_R, this);
    connect(sc2, &QShortcut::activated, this, &GameBrowser::refreshView);
    
    // Ctrl+, to open settings
    auto* sc3 = new QShortcut(Qt::CTRL | Qt::Key_Comma, this);
    connect(sc3, &QShortcut::activated, this, &GameBrowser::showSettings);
}

void GameBrowser::onSearchBarFocus() {
    m_searchBar->setFocus();
    m_searchBar->selectAll();
}

void GameBrowser::loadSearchHistory() {
    QSettings s("Nagh Diefalla", "SteamIdlerEnhanced");
    m_searchHistory = s.value("SearchHistory", QStringList()).toStringList();
}

void GameBrowser::saveSearHistory() {
    QSettings s("Nagh Diefalla", "SteamIdlerEnhanced");
    s.setValue("SearchHistory", m_searchHistory);
    s.sync();
}

void GameBrowser::loadFavorites() {
    QSettings s("Nagh Diefalla", "SteamIdlerEnhanced");
    QVariant v = s.value("FavoriteGames", QVariant());
    if (v.isValid()) {
        m_favoriteGames = v.value<QSet<int>>();
    }
}

void GameBrowser::saveFavorites() {
    QSettings s("Nagh Diefalla", "SteamIdlerEnhanced");
    s.setValue("FavoriteGames", QVariant::fromValue(m_favoriteGames));
    s.sync();
}

void GameBrowser::createLeftPanel() {
    m_leftPanel = new QWidget();
    m_leftPanel->setObjectName("SideBar");
    m_leftPanel->setMinimumWidth(280);
    m_leftPanel->setMaximumWidth(400);
    
    auto* sideLayout = new QVBoxLayout(m_leftPanel);
    sideLayout->setContentsMargins(12, 12, 12, 12);
    sideLayout->setSpacing(8);
    
    // Header
    m_activeHeaderLabel = new QLabel("Active Sessions");
    m_activeHeaderLabel->setObjectName("HeaderLabel");
    sideLayout->addWidget(m_activeHeaderLabel);
    
    // Stats
    m_sessionStatsLabel = new QLabel("No sessions");
    m_sessionStatsLabel->setObjectName("StatLabel");
    sideLayout->addWidget(m_sessionStatsLabel);
    
    sideLayout->addSpacing(12);
    
    // Active Sessions Container
    auto* sessionsContainer = new QWidget();
    m_activeSessionsLayout = new QVBoxLayout(sessionsContainer);
    m_activeSessionsLayout->setContentsMargins(0, 0, 0, 0);
    m_activeSessionsLayout->setSpacing(8);
    
    m_activeSessionsScroll = new QScrollArea();
    m_activeSessionsScroll->setWidget(sessionsContainer);
    m_activeSessionsScroll->setWidgetResizable(true);
    sideLayout->addWidget(m_activeSessionsScroll);
    
    // Spacer
    sideLayout->addStretch();
    
    // Control Buttons
    auto* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(8);
    
    m_btnPauseAll = new QPushButton("⏸  Pause All");
    m_btnPauseAll->setObjectName("PauseBtn");
    m_btnResumeAll = new QPushButton("▶  Resume All");
    m_btnResumeAll->setObjectName("SecondaryBtn");
    m_btnStopAll = new QPushButton("⊗  Stop All");
    m_btnStopAll->setObjectName("DangerBtn");
    
    connect(m_btnPauseAll, &QPushButton::clicked, this, &GameBrowser::onPauseAllClicked);
    connect(m_btnResumeAll, &QPushButton::clicked, this, &GameBrowser::onResumeAllClicked);
    connect(m_btnStopAll, &QPushButton::clicked, this, &GameBrowser::onStopAllClicked);
    
    btnLayout->addWidget(m_btnPauseAll);
    btnLayout->addWidget(m_btnResumeAll);
    btnLayout->addWidget(m_btnStopAll);
    sideLayout->addLayout(btnLayout);
}

void GameBrowser::createMainContentArea() {
    m_contentWidget = new QWidget();
    auto* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(10);
    
    // Search Bar Row
    auto* searchFrame = new QFrame();
    searchFrame->setStyleSheet("QFrame { background-color: transparent; }");
    auto* searchLayout = new QHBoxLayout(searchFrame);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(12);
    
    m_searchBar = new QLineEdit();
    m_searchBar->setObjectName("SearchBar");
    m_searchBar->setPlaceholderText("Search games...");
    m_searchBar->setMinimumHeight(36);
    m_searchBar->setMaximumHeight(36);
    connect(m_searchBar, &QLineEdit::textChanged, this, &GameBrowser::onSearchTextChanged);
    searchLayout->addWidget(m_searchBar);
    
    m_filterCombo = new QComboBox();
    m_filterCombo->setObjectName("FilterCombo");
    m_filterCombo->addItem("All Games");
    m_filterCombo->addItem("Running");
    m_filterCombo->addItem("Not Running");
    m_filterCombo->setMinimumWidth(130);
    m_filterCombo->setMaximumWidth(150);
    m_filterCombo->setMinimumHeight(36);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GameBrowser::refreshView);
    searchLayout->addWidget(m_filterCombo);
    
    m_resultCountLabel = new QLabel();
    m_resultCountLabel->setObjectName("StatLabel");
    m_resultCountLabel->setMinimumWidth(180);
    m_resultCountLabel->setMaximumWidth(250);
    m_resultCountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    searchLayout->addWidget(m_resultCountLabel);
    
    // Favorites Star Button
    auto* favStarBtn = new QPushButton("★");
    favStarBtn->setObjectName("FavBtn");
    favStarBtn->setMaximumWidth(40);
    favStarBtn->setMaximumHeight(36);
    favStarBtn->setToolTip("Toggle Favorites Only");
    favStarBtn->setCheckable(true);
    connect(favStarBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_showOnlyFavorites = checked;
        refreshView();
    });
    searchLayout->addWidget(favStarBtn);
    
    contentLayout->addWidget(searchFrame);
    
    // Batch Action Bar (Hidden by default)
    m_batchActionBar = new QFrame();
    m_batchActionBar->setStyleSheet(R"(
        QFrame {
            background-color: #1c2128;
            border: 1px solid #30363d;
            border-radius: 6px;
        }
    )");
    m_batchActionBar->setVisible(false);
    
    auto* batchLayout = new QHBoxLayout(m_batchActionBar);
    batchLayout->setContentsMargins(12, 10, 12, 10);
    batchLayout->setSpacing(12);
    
    m_selectAllCheckBox = new QCheckBox("Select All");
    connect(m_selectAllCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            for (const auto& game : m_currentSearchResults) {
                m_selectedGames.insert(game.id);
            }
        } else {
            m_selectedGames.clear();
        }
        onBatchSelectionChanged();
    });
    batchLayout->addWidget(m_selectAllCheckBox);
    
    m_batchStatusLabel = new QLabel();
    m_batchStatusLabel->setObjectName("StatLabel");
    batchLayout->addWidget(m_batchStatusLabel);
    
    batchLayout->addSpacing(20);
    
    m_btnBatchIdle = new QPushButton("▶ Start");
    m_btnBatchIdle->setObjectName("PrimaryBtn");
    m_btnBatchIdle->setMaximumWidth(90);
    m_btnBatchPause = new QPushButton("⏸ Pause");
    m_btnBatchPause->setObjectName("PauseBtn");
    m_btnBatchPause->setMaximumWidth(90);
    m_btnBatchStop = new QPushButton("⊗ Stop");
    m_btnBatchStop->setObjectName("DangerBtn");
    m_btnBatchStop->setMaximumWidth(80);
    m_btnBatchClear = new QPushButton("Clear");
    m_btnBatchClear->setObjectName("SecondaryBtn");
    m_btnBatchClear->setMaximumWidth(80);
    
    connect(m_btnBatchIdle, &QPushButton::clicked, this, &GameBrowser::startBatchIdle);
    connect(m_btnBatchPause, &QPushButton::clicked, this, &GameBrowser::pauseBatchIdle);
    connect(m_btnBatchStop, &QPushButton::clicked, this, &GameBrowser::stopBatchIdle);
    connect(m_btnBatchClear, &QPushButton::clicked, this, &GameBrowser::clearBatchSelection);
    
    batchLayout->addWidget(m_btnBatchIdle);
    batchLayout->addWidget(m_btnBatchPause);
    batchLayout->addWidget(m_btnBatchStop);
    batchLayout->addWidget(m_btnBatchClear);
    batchLayout->addStretch();
    
    contentLayout->addWidget(m_batchActionBar);
    
    // Game Results
    auto* resultsContainer = new QWidget();
    m_resultsLayout = new QVBoxLayout(resultsContainer);
    m_resultsLayout->setContentsMargins(0, 0, 0, 0);
    m_resultsLayout->setSpacing(CARD_SPACING);
    
    m_resultsScroll = new QScrollArea();
    m_resultsScroll->setWidget(resultsContainer);
    m_resultsScroll->setWidgetResizable(true);
    contentLayout->addWidget(m_resultsScroll);
    
    // Central widget
    auto* centralWidget = new QWidget();
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    mainLayout->addWidget(m_leftPanel);
    mainLayout->addWidget(m_contentWidget);
    
    setCentralWidget(centralWidget);
}

void GameBrowser::createBottomBar() {
    auto* statusBar = new QStatusBar();
    setStatusBar(statusBar);
    
    m_statusLabel = new QLabel("Ready");
    statusBar->addWidget(m_statusLabel);
    
    statusBar->addPermanentWidget(new QWidget());  // Spacer
    
    m_artLoadProgress = new QProgressBar();
    m_artLoadProgress->setMaximumWidth(200);
    m_artLoadProgress->setMaximumHeight(18);
    m_artLoadProgress->setValue(0);
    m_artLoadProgress->setVisible(false);
    statusBar->addPermanentWidget(m_artLoadProgress);
}

void GameBrowser::setupUi() {
    setupMenuBar();
    createLeftPanel();
    createMainContentArea();
    createBottomBar();
}

void GameBrowser::onSessionsChanged() {
    updateSessionStats();
    refreshView();
}

void GameBrowser::updateSessionStats() {
    auto sessions = m_procManager->activeSessions();
    int active = 0, paused = 0;
    
    for (const auto& s : sessions) {
        if (s.paused) paused++;
        else active++;
    }
    
    QString statsText = QString("%1 running • %2 paused").arg(active).arg(paused);
    m_sessionStatsLabel->setText(statsText);
    
    // Update session cards
    m_activeSessionsLayout->parentWidget()->layout()->takeAt(0);  // Remove previous widgets
    for (const auto& s : sessions) {
        auto* card = createSessionCard(s);
        m_activeSessionsLayout->insertWidget(0, card);
    }
    m_activeSessionsLayout->addStretch();
}

QWidget* GameBrowser::createSessionCard(const IdleSession& session) {
    auto* card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: #0f1419;
            border-radius: 6px;
            border: 1px solid #262d34;
        }
        QFrame:hover {
            background-color: #151b22;
            border: 2px solid #1f6feb;
            padding: 0px;
        }
    )");
    
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(6);
    
    // Title and Timer Row
    auto* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);
    
    // Status indicator
    auto* statusDot = new QLabel("●");
    statusDot->setStyleSheet(R"(
        QLabel {
            color: #3fb950;
            font-size: 8px;
        }
    )");
    statusDot->setMaximumWidth(8);
    topLayout->addWidget(statusDot);
    
    // Game name
    auto* titleLabel = new QLabel(session.name);
    titleLabel->setMaximumWidth(200);
    titleLabel->setWordWrap(false);
    titleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 12px;
            font-weight: 600;
            color: #e6edf3;
        }
    )");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    
    // Timer
    auto* timerLabel = new QLabel(formatDuration(m_procManager->getDuration(session.appId)));
    timerLabel->setStyleSheet(R"(
        QLabel {
            font-size: 11px;
            color: #3fb950;
            font-weight: 600;
        }
    )");
    topLayout->addWidget(timerLabel);
    
    layout->addLayout(topLayout);
    
    // Controls
    auto* ctrlLayout = new QHBoxLayout();
    ctrlLayout->setContentsMargins(0, 0, 0, 0);
    ctrlLayout->setSpacing(6);
    
    if (session.paused) {
        auto* btn = new QPushButton("▶ Resume");
        btn->setObjectName("PrimaryBtn");
        btn->setMaximumWidth(85);
        btn->setMaximumHeight(28);
        btn->setStyleSheet(R"(
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #238636, stop:1 #1a7f37);
                color: #ffffff;
                border: none;
                border-radius: 4px;
                font-weight: 500;
                font-size: 11px;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2ea043, stop:1 #238636);
            }
        )");
        connect(btn, &QPushButton::clicked, this, [this, appId = session.appId]() {
            m_procManager->resumeIdling(appId);
        });
        ctrlLayout->addWidget(btn);
    } else {
        auto* btn = new QPushButton("⏸ Pause");
        btn->setObjectName("PauseBtn");
        btn->setMaximumWidth(85);
        btn->setMaximumHeight(28);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: #d29922;
                color: #ffffff;
                border: none;
                border-radius: 4px;
                font-weight: 500;
                font-size: 11px;
            }
            QPushButton:hover {
                background-color: #e3b341;
            }
        )");
        connect(btn, &QPushButton::clicked, this, [this, appId = session.appId]() {
            m_procManager->pauseIdling(appId);
        });
        ctrlLayout->addWidget(btn);
    }
    
    auto* stopBtn = new QPushButton("⊗");
    stopBtn->setObjectName("DangerBtn");
    stopBtn->setMaximumWidth(36);
    stopBtn->setMaximumHeight(28);
    stopBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #da3633;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #f85149;
        }
    )");
    connect(stopBtn, &QPushButton::clicked, this, [this, appId = session.appId]() {
        m_procManager->stopIdling(appId);
    });
    ctrlLayout->addWidget(stopBtn);
    
    ctrlLayout->addStretch();
    layout->addLayout(ctrlLayout);
    
    return card;
}

QWidget* GameBrowser::createGameCard(const GameResult& game, bool isRunning, bool isFavorite) {
    auto* card = new QFrame();
    
    // Modern card design with gradient effect simulation
    card->setStyleSheet(R"(
        QFrame {
            background-color: #0f1419;
            border-radius: 8px;
            border: 1px solid #262d34;
            min-height: 95px;
        }
        QFrame:hover {
            background-color: #151b22;
            border: 2px solid #1f6feb;
            padding: 0px;
        }
    )");
    card->setMinimumHeight(CARD_HEIGHT);
    
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(10);
    
    // === GAME THUMBNAIL/ICON ===
    auto* artLabel = new QLabel();
    artLabel->setFixedSize(70, 70);
    artLabel->setStyleSheet(R"(
        QLabel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1f6feb, stop:1 #0969da);
            border-radius: 6px;
            font-size: 38px;
            font-weight: bold;
            color: #ffffff;
        }
    )");
    artLabel->setAlignment(Qt::AlignCenter);
    artLabel->setText("🎮");
    layout->addWidget(artLabel);
    
    // === GAME INFO SECTION ===
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(4);
    
    // Game Title
    auto* titleLabel = new QLabel(game.name);
    titleLabel->setObjectName("HeaderLabel");
    titleLabel->setMaximumWidth(320);
    titleLabel->setWordWrap(false);
    titleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 12px;
            font-weight: 600;
            color: #e6edf3;
            letter-spacing: 0.3px;
        }
    )");
    infoLayout->addWidget(titleLabel);
    
    // App ID
    auto* idLabel = new QLabel(QString("ID: %1").arg(game.id));
    idLabel->setStyleSheet(R"(
        QLabel {
            font-size: 10px;
            color: #7d8590;
            font-weight: 500;
        }
    )");
    infoLayout->addWidget(idLabel);
    
    // Status/Timer (only for running games)
    if (isRunning) {
        auto* statusLayout = new QHBoxLayout();
        statusLayout->setContentsMargins(0, 0, 0, 0);
        statusLayout->setSpacing(6);
        
        auto* statusDot = new QLabel("●");
        statusDot->setStyleSheet(R"(
            QLabel {
                color: #3fb950;
                font-size: 10px;
            }
        )");
        statusDot->setMaximumWidth(12);
        statusLayout->addWidget(statusDot);
        
        auto* timerLabel = new QLabel();
        timerLabel->setObjectName("TimerLabel");
        timerLabel->setText("Running • " + formatDuration(m_procManager->getDuration(game.id)));
        timerLabel->setStyleSheet(R"(
            QLabel {
                font-size: 11px;
                color: #3fb950;
                font-weight: 600;
            }
        )");
        statusLayout->addWidget(timerLabel);
        
        infoLayout->addLayout(statusLayout);
    } else {
        auto* statusLabel = new QLabel("Idle");
        statusLabel->setStyleSheet(R"(
            QLabel {
                font-size: 11px;
                color: #8b949e;
                font-weight: 500;
            }
        )");
        infoLayout->addWidget(statusLabel);
    }
    
    layout->addLayout(infoLayout);
    layout->addStretch();
    
    // === FAVORITE BUTTON ===
    auto* favBtn = new QPushButton(isFavorite ? "★" : "☆");
    favBtn->setObjectName("FavSmallBtn");
    favBtn->setFixedSize(36, 32);
    favBtn->setToolTip(isFavorite ? "Remove from Favorites" : "Add to Favorites");
    favBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1f6feb;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            font-weight: bold;
            font-size: 16px;
        }
        QPushButton:hover {
            background-color: #388bfd;
        }
    )");
    connect(favBtn, &QPushButton::clicked, this, [this, appId = game.id, isFav = isFavorite]() {
        if (isFav) {
            m_favoriteGames.remove(appId);
        } else {
            m_favoriteGames.insert(appId);
            m_recentGames.removeAll(appId);
            m_recentGames.prepend(appId);
            while (m_recentGames.count() > 50) m_recentGames.removeLast();
        }
        saveFavorites();
        refreshView();
    });
    layout->addWidget(favBtn);
    
    // === BATCH SELECTION CHECKBOX ===
    auto* checkBox = new QCheckBox();
    checkBox->setMaximumWidth(32);
    checkBox->setStyleSheet(R"(
        QCheckBox {
            spacing: 4px;
        }
        QCheckBox::indicator {
            width: 20px;
            height: 20px;
            border-radius: 4px;
            background-color: #0d1117;
            border: 2px solid #30363d;
        }
        QCheckBox::indicator:hover {
            border: 2px solid #58a6ff;
            background-color: #0f1419;
        }
        QCheckBox::indicator:checked {
            background-color: #238636;
            border: 2px solid #238636;
        }
    )");
    connect(checkBox, &QCheckBox::toggled, this, [this, appId = game.id](bool checked) {
        if (checked) {
            m_selectedGames.insert(appId);
        } else {
            m_selectedGames.remove(appId);
        }
        onBatchSelectionChanged();
    });
    layout->addWidget(checkBox);
    
    // === ACTION BUTTONS ===
    auto* btnLayout = new QVBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(6);
    
    if (isRunning) {
        // Pause Button
        auto* pauseBtn = new QPushButton("⏸");
        pauseBtn->setObjectName("PauseBtn");
        pauseBtn->setFixedSize(36, 32);
        pauseBtn->setToolTip("Pause idling");
        pauseBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #d29922;
                color: #ffffff;
                border: none;
                border-radius: 6px;
                font-weight: bold;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #e3b341;
            }
            QPushButton:pressed {
                background-color: #c4970c;
            }
        )");
        connect(pauseBtn, &QPushButton::clicked, this, [this, appId = game.id]() {
            m_procManager->togglePauseResume(appId);
        });
        btnLayout->addWidget(pauseBtn);
        
        // Stop Button
        auto* stopBtn = new QPushButton("⊗");
        stopBtn->setObjectName("DangerBtn");
        stopBtn->setFixedSize(36, 32);
        stopBtn->setToolTip("Stop idling");
        stopBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #da3633;
                color: #ffffff;
                border: none;
                border-radius: 6px;
                font-weight: bold;
                font-size: 13px;
            }
            QPushButton:hover {
                background-color: #f85149;
            }
            QPushButton:pressed {
                background-color: #bd2c11;
            }
        )");
        connect(stopBtn, &QPushButton::clicked, this, [this, appId = game.id]() {
            m_procManager->stopIdling(appId);
        });
        btnLayout->addWidget(stopBtn);
    } else {
        // Start Button
        auto* startBtn = new QPushButton("▶");
        startBtn->setObjectName("PrimaryBtn");
        startBtn->setFixedSize(36, 70);
        startBtn->setToolTip("Start idling");
        startBtn->setStyleSheet(R"(
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #238636, stop:1 #1a7f37);
                color: #ffffff;
                border: none;
                border-radius: 6px;
                font-weight: bold;
                font-size: 18px;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2ea043, stop:1 #238636);
            }
            QPushButton:pressed {
                background-color: #1a7f37;
            }
        )");
        connect(startBtn, &QPushButton::clicked, this, [this, name = game.name, appId = game.id]() {
            m_procManager->startIdling(appId, name);
        });
        btnLayout->addWidget(startBtn);
    }
    
    layout->addLayout(btnLayout);
    
    m_gameCards[game.id] = card;
    return card;
}

void GameBrowser::refreshView() {
    // Clear results
    clearLayout(m_resultsLayout);
    m_gameCards.clear();
    
    // Get search results
    QString query = m_searchBar->text();
    QList<GameResult> results = m_engine->search(query.isEmpty() ? "%" : query, 100);
    
    // Save to search history
    if (!query.isEmpty() && query.length() > 2) {
        m_searchHistory.removeAll(query);
        m_searchHistory.prepend(query);
        while (m_searchHistory.count() > 20) m_searchHistory.removeLast();
        saveSearHistory();
    }
    
    m_currentSearchResults = results;
    
    // Apply sorting
    if (m_sortOrder == SortOrder::NameAsc) {
        std::sort(results.begin(), results.end(), [](const GameResult& a, const GameResult& b) {
            return a.name < b.name;
        });
    } else if (m_sortOrder == SortOrder::NameDesc) {
        std::sort(results.begin(), results.end(), [](const GameResult& a, const GameResult& b) {
            return a.name > b.name;
        });
    }
    
    // Apply filters
    int filter = m_filterCombo->currentIndex();  // 0: All, 1: Running, 2: Not Running
    
    int runningCount = 0;
    int displayCount = 0;
    
    for (const auto& game : results) {
        bool isRunning = m_procManager->isIdling(game.id);
        bool isFavorite = m_favoriteGames.contains(game.id);
        
        // Apply filter conditions
        if (filter == 1 && !isRunning) continue;  // Show only running
        if (filter == 2 && isRunning) continue;   // Show only not running
        if (m_showOnlyFavorites && !isFavorite) continue;  // Show only favorites
        if (m_showOnlyRecent && m_recentGames.indexOf(game.id) < 0) continue;  // Show only recent
        
        auto* card = createGameCard(game, isRunning, isFavorite);
        m_resultsLayout->addWidget(card);
        displayCount++;
        
        if (isRunning) runningCount++;
    }
    
    m_resultsLayout->addStretch();
    
    // Update result count
    m_resultCountLabel->setText(QString("%1 / %2 games").arg(displayCount).arg(results.count()));
    
    // Update status
    m_statusLabel->setText(QString("Showing %1 games • %2 running").arg(displayCount).arg(runningCount));
}

void GameBrowser::updateTimers() {
    // Update session card display times
    for (const auto& session : m_procManager->activeSessions()) {
        if (m_sessionCards.contains(session.appId)) {
            // The timer display is already in the cards, we just need to update active sessions
            // This is handled by onSessionsChanged which refreshes the view
        }
    }
}

void GameBrowser::onSearchTextChanged(const QString& text) {
    m_searchDebounceTimer->stop();
    m_searchDebounceTimer->start();
}

void GameBrowser::onArtLoaded(int appId, const GameArt& art) {
    // Update art in game card if it exists
}

void GameBrowser::onArtLoadFailed(int appId, const QString& error) {
    // Log and continue
}

void GameBrowser::onUpdateAvailable(const QString& version, const QString& changelog, const QUrl& downloadUrl) {
    // Show notification
    m_statusLabel->setText(QString("Update available: v%1").arg(version));
}

void GameBrowser::onBatchSelectionChanged() {
    bool hasSelection = !m_selectedGames.isEmpty();
    m_batchActionBar->setVisible(hasSelection);
    if (hasSelection) {
        m_batchStatusLabel->setText(QString("%1 selected").arg(m_selectedGames.count()));
    }
}

void GameBrowser::startBatchIdle() {
    for (int appId : m_selectedGames) {
        // Find the game name
        for (const auto& game : m_currentSearchResults) {
            if (game.id == appId) {
                m_procManager->startIdling(appId, game.name);
                break;
            }
        }
    }
    clearBatchSelection();
}

void GameBrowser::stopBatchIdle() {
    for (int appId : m_selectedGames) {
        m_procManager->stopIdling(appId);
    }
    clearBatchSelection();
}

void GameBrowser::pauseBatchIdle() {
    for (int appId : m_selectedGames) {
        m_procManager->pauseIdling(appId);
    }
}

void GameBrowser::clearBatchSelection() {
    m_selectedGames.clear();
    m_selectAllCheckBox->setChecked(false);
    m_batchActionBar->setVisible(false);
    refreshView();
}

void GameBrowser::onPauseAllClicked() {
    m_procManager->pauseAll();
}

void GameBrowser::onResumeAllClicked() {
    m_procManager->resumeAll();
}

void GameBrowser::onStopAllClicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Stop All Sessions",
        "Are you sure you want to stop all idling sessions?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_procManager->stopAll();
    }
}

void GameBrowser::showSettings() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
        connect(m_settingsDialog, &SettingsDialog::settingsChanged, this, &GameBrowser::onSettingsChanged);
    }
    m_settingsDialog->exec();
}

void GameBrowser::onSettingsChanged() {
    setupModernTheme();
    refreshView();
}

void GameBrowser::retranslateUi() {
    if (m_fileMenu) m_fileMenu->setTitle("File");
    if (m_refreshAction) m_refreshAction->setText("Refresh");
    if (m_settingsAction) m_settingsAction->setText("Settings");
    if (m_exitAction) m_exitAction->setText("Exit");
}

void GameBrowser::showContextMenu(const QPoint &pos) {
    // Context menu implementation
}

QString GameBrowser::formatDuration(qint64 seconds) const {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0'))
                              .arg(minutes, 2, 10, QChar('0'))
                              .arg(secs, 2, 10, QChar('0'));
}

void GameBrowser::clearLayout(QLayout* layout) {
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void GameBrowser::onGameCardAction(int appId) {
    // Handled by individual button connections
}

void GameBrowser::onGameCardPause(int appId) {
    m_procManager->togglePauseResume(appId);
}

void GameBrowser::closeEvent(QCloseEvent *event) {
    // Save settings before closing
    event->accept();
}

void GameBrowser::setupModernTheme() {
    // Modern dark theme with accent colors matching GitHub dark theme
    QString styleSheet = R"(
        * {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
        }
        
        QMainWindow, QWidget {
            background-color: #0d1117;
            color: #e6edf3;
        }
        
        #SideBar {
            background-color: #0d1117;
            border-right: 1px solid #30363d;
        }
        
        QLineEdit {
            background-color: #0d1117;
            color: #e6edf3;
            border: 1px solid #30363d;
            border-radius: 6px;
            padding: 8px 12px;
            font-size: 13px;
            selection-background-color: #1f6feb;
        }
        
        QLineEdit:focus {
            border: 1px solid #58a6ff;
            outline: none;
        }
        
        QComboBox {
            background-color: #0d1117;
            color: #e6edf3;
            border: 1px solid #30363d;
            border-radius: 6px;
            padding: 6px 8px;
            font-size: 13px;
        }
        
        QComboBox:focus {
            border: 1px solid #58a6ff;
        }
        
        QComboBox::drop-down {
            border: none;
        }
        
        QComboBox::down-arrow {
            image: none;
            width: 0px;
        }
        
        QComboBox QAbstractItemView {
            background-color: #161b22;
            color: #e6edf3;
            selection-background-color: #1f6feb;
            border: 1px solid #30363d;
        }
        
        QPushButton {
            background-color: #238636;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 6px 12px;
            font-weight: 500;
            font-size: 12px;
            min-width: 60px;
        }
        
        QPushButton:hover {
            background-color: #2ea043;
        }
        
        QPushButton:pressed {
            background-color: #1a7f37;
        }
        
        QPushButton:disabled {
            background-color: #30363d;
            color: #8b949e;
        }
        
        QPushButton#PrimaryBtn {
            background-color: #238636;
        }
        
        QPushButton#PrimaryBtn:hover {
            background-color: #2ea043;
        }
        
        QPushButton#PauseBtn {
            background-color: #d29922;
        }
        
        QPushButton#PauseBtn:hover {
            background-color: #e3b341;
        }
        
        QPushButton#DangerBtn {
            background-color: #da3633;
        }
        
        QPushButton#DangerBtn:hover {
            background-color: #f85149;
        }
        
        QPushButton#SecondaryBtn {
            background-color: #1f6feb;
        }
        
        QPushButton#SecondaryBtn:hover {
            background-color: #388bfd;
        }
        
        QLabel#HeaderLabel {
            color: #e6edf3;
            font-weight: 600;
            font-size: 13px;
        }
        
        QLabel#StatLabel {
            color: #8b949e;
            font-size: 12px;
        }
        
        QLabel#TimerLabel {
            color: #58a6ff;
            font-size: 12px;
            font-weight: 600;
        }
        
        QScrollArea {
            border: none;
            background-color: #0d1117;
        }
        
        QScrollBar:vertical {
            background-color: #0d1117;
            width: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:vertical {
            background-color: #30363d;
            border-radius: 6px;
            min-height: 20px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #484f58;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
        }
        
        QCheckBox {
            color: #e6edf3;
            spacing: 6px;
            background-color: transparent;
            font-size: 12px;
        }
        
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 3px;
            background-color: #0d1117;
            border: 1px solid #30363d;
        }
        
        QCheckBox::indicator:hover {
            border: 1px solid #58a6ff;
        }
        
        QCheckBox::indicator:checked {
            background-color: #238636;
            border: 1px solid #238636;
        }
        
        QProgressBar {
            background-color: #161b22;
            border: 1px solid #30363d;
            border-radius: 4px;
            text-align: center;
            color: #e6edf3;
            height: 16px;
        }
        
        QProgressBar::chunk {
            background-color: #238636;
            border-radius: 3px;
        }
        
        QMenuBar {
            background-color: #0d1117;
            color: #e6edf3;
            border-bottom: 1px solid #30363d;
        }
        
        QMenuBar::item:selected {
            background-color: #1f6feb;
        }
        
        QMenu {
            background-color: #161b22;
            color: #e6edf3;
            border: 1px solid #30363d;
            border-radius: 6px;
        }
        
        QMenu::item:selected {
            background-color: #1f6feb;
        }
        
        QMenu::separator {
            background-color: #30363d;
            height: 1px;
            margin: 4px 0px;
        }
        
        QStatusBar {
            background-color: #0d1117;
            color: #8b949e;
            border-top: 1px solid #30363d;
            font-size: 12px;
        }
        
        QFrame {
            background-color: transparent;
        }
    )";
    
    if (auto app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setStyleSheet(styleSheet);
    }
}

