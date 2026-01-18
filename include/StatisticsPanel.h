#ifndef STATISTICSPANEL_H
#define STATISTICSPANEL_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QPushButton>

class StatisticsPanel : public QWidget {
    Q_OBJECT

public:
    explicit StatisticsPanel(QWidget *parent = nullptr);
    ~StatisticsPanel();

    void refreshStats();

private slots:
    // void onExportHistory();
    void onRefreshStats();
    void exportSessionHistory();

private:
    void setupUI();
    void setupConnections();
    void updateStatLabels();
    void populateSessionHistory();

    // Stat display labels
    QLabel* m_totalIdleTimeLabel;
    QLabel* m_totalSessionsLabel;
    QLabel* m_totalCardsLabel;
    QLabel* m_gamesCompletedLabel;
    QLabel* m_weekIdleTimeLabel;
    QLabel* m_monthIdleTimeLabel;
    QLabel* m_weekSessionsLabel;
    QLabel* m_monthSessionsLabel;

    // Top performers and most played
    QTableWidget* m_topPerformersTable;
    QTableWidget* m_mostPlayedTable;
    QTableWidget* m_sessionHistoryTable;

    // Control buttons
    QPushButton* m_refreshButton;
    QPushButton* m_exportButton;

    QVBoxLayout* m_mainLayout;
};

#endif // STATISTICSPANEL_H
