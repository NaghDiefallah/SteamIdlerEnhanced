#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QProcess>
#include <QMap>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QDateTime>
#include <QObject>
#include <QApplication>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QTimer>
#include "Logger.h"
#include "AppConfig.h"
#include "Lang.h"

struct IdleSession {
    QProcess* process;
    int appId;
    QString name;
    qint64 startTime;
    bool paused = false;
    qint64 pausedTime = 0;
};

class ProcessManager : public QObject {
    Q_OBJECT
public:
    explicit ProcessManager(QObject* parent = nullptr);
    ~ProcessManager();

    void startIdling(int appId, const QString& name);

    void stopIdling(int appId);

    void pauseIdling(int appId);
    void resumeIdling(int appId);
    void togglePauseResume(int appId);  // Convenience method for toggle UI
    void pauseAll();
    void resumeAll();

    QList<IdleSession> activeSessions() const;
    bool isIdling(int appId) const;
    qint64 getDuration(int appId) const;

    void updatePersistence();
    void stopAll();

signals:
    void sessionsChanged();
    void restoreWindow();

private slots:
    void finalizeUpdate();
    void restoreSessions();

private:
    void setupTray();
    void updateTrayMenu();
    void requestUpdate();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

    QTimer* m_updateTimer = nullptr;
    QMap<int, IdleSession> m_sessions;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
    QAction* m_pauseAllAct = nullptr;
    QAction* m_resumeAllAct = nullptr;
    QAction* m_stopAllAct = nullptr;
    QAction* m_restoreAct = nullptr;
};

#endif