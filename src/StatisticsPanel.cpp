#include "StatisticsPanel.h"
#include "Statistics.h"
#include "Logger.h"
#include "Lang.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QStandardPaths>

StatisticsPanel::StatisticsPanel(QWidget *parent)
    : QWidget(parent), m_mainLayout(nullptr) {
    setupUI();
    setupConnections();
    refreshStats();
}

StatisticsPanel::~StatisticsPanel() {
}

void StatisticsPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(16);

    // Overall Statistics Section
    QGroupBox* overallGroup = new QGroupBox(Lang::get("stats_session_history"), this);
    QVBoxLayout* overallLayout = new QVBoxLayout(overallGroup);

    // Create a grid of stat labels
    QHBoxLayout* row1 = new QHBoxLayout();
    m_totalIdleTimeLabel = new QLabel("0h", this);
    m_totalSessionsLabel = new QLabel("0", this);
    m_totalCardsLabel = new QLabel("0", this);
    m_gamesCompletedLabel = new QLabel("0", this);

    row1->addWidget(new QLabel(Lang::get("stats_total_idle_time") + ":"));
    row1->addWidget(m_totalIdleTimeLabel);
    row1->addSpacing(20);
    row1->addWidget(new QLabel(Lang::get("stats_total_sessions") + ":"));
    row1->addWidget(m_totalSessionsLabel);
    row1->addSpacing(20);
    row1->addWidget(new QLabel(Lang::get("stats_total_cards") + ":"));
    row1->addWidget(m_totalCardsLabel);
    row1->addSpacing(20);
    row1->addWidget(new QLabel(Lang::get("stats_games_completed") + ":"));
    row1->addWidget(m_gamesCompletedLabel);
    row1->addStretch();

    overallLayout->addLayout(row1);

    QHBoxLayout* row2 = new QHBoxLayout();
    m_weekIdleTimeLabel = new QLabel("0h", this);
    m_monthIdleTimeLabel = new QLabel("0h", this);
    m_weekSessionsLabel = new QLabel("0", this);
    m_monthSessionsLabel = new QLabel("0", this);

    row2->addWidget(new QLabel(Lang::get("stats_idle_time_week") + ":"));
    row2->addWidget(m_weekIdleTimeLabel);
    row2->addSpacing(20);
    row2->addWidget(new QLabel(Lang::get("stats_idle_time_month") + ":"));
    row2->addWidget(m_monthIdleTimeLabel);
    row2->addSpacing(20);
    row2->addWidget(new QLabel(Lang::get("stats_sessions_week") + ":"));
    row2->addWidget(m_weekSessionsLabel);
    row2->addSpacing(20);
    row2->addWidget(new QLabel(Lang::get("stats_sessions_month") + ":"));
    row2->addWidget(m_monthSessionsLabel);
    row2->addStretch();

    overallLayout->addLayout(row2);

    m_mainLayout->addWidget(overallGroup);

    // Top Performers Table
    QGroupBox* topGroup = new QGroupBox(Lang::get("stats_top_performers"), this);
    QVBoxLayout* topLayout = new QVBoxLayout(topGroup);

    m_topPerformersTable = new QTableWidget(this);
    m_topPerformersTable->setColumnCount(4);
    m_topPerformersTable->setHorizontalHeaderLabels({
        Lang::get("gamename"),
        Lang::get("stats_total_cards"),
        Lang::get("stats_total_idle_time"),
        "Progress"
    });
    m_topPerformersTable->setMaximumHeight(200);
    m_topPerformersTable->horizontalHeader()->setStretchLastSection(true);
    topLayout->addWidget(m_topPerformersTable);
    m_mainLayout->addWidget(topGroup);

    // Most Played Table
    QGroupBox* playedGroup = new QGroupBox(Lang::get("stats_most_played"), this);
    QVBoxLayout* playedLayout = new QVBoxLayout(playedGroup);

    m_mostPlayedTable = new QTableWidget(this);
    m_mostPlayedTable->setColumnCount(3);
    m_mostPlayedTable->setHorizontalHeaderLabels({
        Lang::get("gamename"),
        Lang::get("stats_total_idle_time"),
        Lang::get("stats_total_sessions")
    });
    m_mostPlayedTable->setMaximumHeight(200);
    m_mostPlayedTable->horizontalHeader()->setStretchLastSection(true);
    playedLayout->addWidget(m_mostPlayedTable);
    m_mainLayout->addWidget(playedGroup);

    // Session History Table
    QGroupBox* historyGroup = new QGroupBox(Lang::get("stats_session_history"), this);
    QVBoxLayout* historyLayout = new QVBoxLayout(historyGroup);

    m_sessionHistoryTable = new QTableWidget(this);
    m_sessionHistoryTable->setColumnCount(4);
    m_sessionHistoryTable->setHorizontalHeaderLabels({
        "Start Time",
        "Duration",
        "Cards Earned",
        "Game"
    });
    m_sessionHistoryTable->setMaximumHeight(250);
    m_sessionHistoryTable->horizontalHeader()->setStretchLastSection(true);
    historyLayout->addWidget(m_sessionHistoryTable);
    m_mainLayout->addWidget(historyGroup);

    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton(Lang::get("btn_apply"), this);
    m_exportButton = new QPushButton(Lang::get("btn_export"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_exportButton);
    m_mainLayout->addLayout(buttonLayout);

    m_mainLayout->addStretch();
}

void StatisticsPanel::setupConnections() {
    connect(m_refreshButton, &QPushButton::clicked, this, &StatisticsPanel::onRefreshStats);
    connect(m_exportButton, &QPushButton::clicked, this, &StatisticsPanel::exportSessionHistory);
}

void StatisticsPanel::refreshStats() {
    updateStatLabels();
    populateSessionHistory();
}

void StatisticsPanel::updateStatLabels() {
    auto& stats = Statistics::instance();

    // Convert seconds to hours
    auto formatTime = [](qint64 seconds) -> QString {
        qint64 hours = seconds / 3600;
        qint64 mins = (seconds % 3600) / 60;
        return QString("%1h %2m").arg(hours).arg(mins);
    };

    m_totalIdleTimeLabel->setText(formatTime(stats.getTotalIdleTime()));
    m_totalSessionsLabel->setText(QString::number(stats.getTotalSessions()));
    m_totalCardsLabel->setText(QString::number(stats.getTotalCardsEarned()));
    m_gamesCompletedLabel->setText(QString::number(stats.getTotalGamesCompleted()));

    m_weekIdleTimeLabel->setText(formatTime(stats.getIdleTimeThisWeek()));
    m_monthIdleTimeLabel->setText(formatTime(stats.getIdleTimeThisMonth()));
    m_weekSessionsLabel->setText(QString::number(stats.getSessionsThisWeek()));
    m_monthSessionsLabel->setText(QString::number(stats.getSessionsThisMonth()));

    // Top Performers
    m_topPerformersTable->setRowCount(0);
    auto topPerformers = stats.getTopPerformers(10);
    for (const auto& game : topPerformers) {
        int row = m_topPerformersTable->rowCount();
        m_topPerformersTable->insertRow(row);
        m_topPerformersTable->setItem(row, 0, new QTableWidgetItem(game.gameName));
        m_topPerformersTable->setItem(row, 1, new QTableWidgetItem(QString::number(game.cardsEarned)));
        m_topPerformersTable->setItem(row, 2, new QTableWidgetItem(formatTime(game.totalIdleTime)));
        m_topPerformersTable->setItem(row, 3, new QTableWidgetItem(game.completed ? "✓" : "In Progress"));
    }

    // Most Played
    m_mostPlayedTable->setRowCount(0);
    auto mostPlayed = stats.getMostPlayed(10);
    for (const auto& game : mostPlayed) {
        int row = m_mostPlayedTable->rowCount();
        m_mostPlayedTable->insertRow(row);
        m_mostPlayedTable->setItem(row, 0, new QTableWidgetItem(game.gameName));
        m_mostPlayedTable->setItem(row, 1, new QTableWidgetItem(formatTime(game.totalIdleTime)));
        m_mostPlayedTable->setItem(row, 2, new QTableWidgetItem(QString::number(game.totalSessions)));
    }
}

void StatisticsPanel::populateSessionHistory() {
    auto& stats = Statistics::instance();
    auto history = stats.getSessionHistory(30);  // Last 30 days

    m_sessionHistoryTable->setRowCount(0);
    for (const auto& session : history) {
        int row = m_sessionHistoryTable->rowCount();
        m_sessionHistoryTable->insertRow(row);
        m_sessionHistoryTable->setItem(row, 0, new QTableWidgetItem(session.startTime.toString("yyyy-MM-dd HH:mm")));
        m_sessionHistoryTable->setItem(row, 1, new QTableWidgetItem(QString::number(session.duration / 60) + "m"));
        m_sessionHistoryTable->setItem(row, 2, new QTableWidgetItem(QString::number(session.cardsEarned)));
        m_sessionHistoryTable->setItem(row, 3, new QTableWidgetItem(session.gameName));
    }
}

void StatisticsPanel::onRefreshStats() {
    refreshStats();
    Logger::log()->info("Statistics panel refreshed");
}

void StatisticsPanel::exportSessionHistory() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QFileDialog::getSaveFileName(this, "Export Session History", 
        dir + "/steam_idler_history.json", "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        if (Statistics::instance().exportHistory(fileName)) {
            Logger::log()->info("Session history exported to {}", fileName.toStdString());
        } else {
            Logger::log()->error("Failed to export session history");
        }
    }
}
