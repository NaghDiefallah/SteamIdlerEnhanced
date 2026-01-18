#include "ProcessManager.h"
#include "Logger.h"
#include "AppConfig.h"
#include "Lang.h"
#include "SessionHistory.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QStyle>
#include <QFile>
#include <QCoreApplication>
#include <QMenu>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ProcessManager::ProcessManager(QObject* parent) : QObject(parent) {
    // UI Optimization: Batch refreshes within 50ms windows
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(50);
    connect(m_updateTimer, &QTimer::timeout, this, &ProcessManager::finalizeUpdate);

    setupTray();
    QTimer::singleShot(100, this, &ProcessManager::restoreSessions);
}

ProcessManager::~ProcessManager() {
    stopAll();
}

void ProcessManager::startIdling(int appId, const QString& name) {
    if (m_sessions.contains(appId) && !m_sessions[appId].paused) return;

    Logger::log()->info("Starting idle session for {} (ID: {})", name.toStdString(), appId);

    // 1. Path Setup
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString ghostBase = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ghost";
    const QString ghostDir = ghostBase + "/" + QString::number(appId);
    QDir().mkpath(ghostDir);

    Logger::log()->debug("Ghost directory: {}", ghostDir.toStdString());

    // 2. Performance: Hardlink files instead of copying (instant I/O)
    auto fastSync = [](const QString& src, const QString& dst) {
        if (QFile::exists(dst)) return;
#ifdef Q_OS_WIN
        if (!CreateHardLinkW((LPCWSTR)dst.utf16(), (LPCWSTR)src.utf16(), NULL)) {
            QFile::copy(src, dst);
        }
#else
        QFile::copy(src, dst);
#endif
    };

    fastSync(appDir + "/ghost_launcher.exe", ghostDir + "/ghost_launcher.exe");
    fastSync(appDir + "/steam_api64.dll", ghostDir + "/steam_api64.dll");

    Logger::log()->debug("Files synced to ghost directory");

    // 3. AppID Persistence
    QFile f(ghostDir + "/steam_appid.txt");
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&f) << appId;
        f.close();
    }

    // 4. Optimized Process Launch
    QProcess* p = new QProcess(this);
    p->setWorkingDirectory(ghostDir);
    p->setProperty("appId", appId);
    
#ifdef Q_OS_WIN
    // Performance: Start without creating console handles or pipes
    p->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NO_WINDOW;
        args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
    });
#endif

    connect(p, &QProcess::finished, this, &ProcessManager::onProcessFinished);
    
    Logger::log()->debug("Launching ghost_launcher.exe...");
    p->start(ghostDir + "/ghost_launcher.exe");

    if (p->waitForStarted(2000)) {
        if (m_sessions.contains(appId)) {
            m_sessions[appId].process = p;
            m_sessions[appId].paused = false;
        } else {
            m_sessions.insert(appId, {p, appId, name, QDateTime::currentSecsSinceEpoch(), false});
            // Log session start
            SessionHistory::instance().logSessionStart(appId, name);
        }
        Logger::log()->info("Idle session started successfully for {} (ID: {})", name.toStdString(), appId);
        requestUpdate();
    } else {
        p->deleteLater();
        Logger::log()->error("Failed to start launcher for ID: {} - Process error: {}", appId, p->errorString().toStdString());
    }
}

void ProcessManager::stopIdling(int appId) {
    if (!m_sessions.contains(appId)) return;

    auto& s = m_sessions[appId];
    qint64 duration = QDateTime::currentSecsSinceEpoch() - s.startTime;

    if (s.process) {
        disconnect(s.process, nullptr, this, nullptr);
        QProcess* p = s.process;
        p->terminate();
        
        // Clean up the process object once it actually exits
        connect(p, &QProcess::finished, p, &QObject::deleteLater);
        QTimer::singleShot(3000, p, [p]() { if (p && p->state() != QProcess::NotRunning) p->kill(); });
    }

    // Log session end
    SessionHistory::instance().logSessionStop(appId, duration);

    m_sessions.remove(appId);
    
    // Asynchronous folder cleanup
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ghost/" + QString::number(appId);
    QDir(path).removeRecursively();

    requestUpdate();
}

void ProcessManager::pauseIdling(int appId) {
    if (!m_sessions.contains(appId)) return;
    auto& s = m_sessions[appId];
    
    if (!s.paused) {
        if (s.process) {
            disconnect(s.process, nullptr, this, nullptr);
            s.process->terminate();
            connect(s.process, &QProcess::finished, s.process, &QObject::deleteLater);
            s.process = nullptr;
        }
        s.paused = true;
        s.pausedTime = QDateTime::currentSecsSinceEpoch();
        requestUpdate();
        Logger::log()->info("Session paused: AppID {}", appId);
    }
}

void ProcessManager::resumeIdling(int appId) {
    if (!m_sessions.contains(appId)) return;
    auto& s = m_sessions[appId];
    
    if (s.paused) {
        s.paused = false;
        s.startTime = QDateTime::currentSecsSinceEpoch() - (s.startTime);  // Restore time
        startIdling(appId, s.name);
        Logger::log()->info("Session resumed: AppID {}", appId);
    }
}

void ProcessManager::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    QProcess* p = qobject_cast<QProcess*>(sender());
    if (!p) return;

    int appId = p->property("appId").toInt();
    if (m_sessions.contains(appId) && !m_sessions[appId].paused) {
        m_sessions.remove(appId);
        requestUpdate();
    }
    p->deleteLater();
}

void ProcessManager::togglePauseResume(int appId) {
    if (!m_sessions.contains(appId)) return;
    if (m_sessions[appId].paused) {
        resumeIdling(appId);
    } else {
        pauseIdling(appId);
    }
}

void ProcessManager::pauseAll() {
    for (int id : m_sessions.keys()) {
        if (!m_sessions[id].paused) pauseIdling(id);
    }
}

void ProcessManager::resumeAll() {
    for (int id : m_sessions.keys()) {
        if (m_sessions[id].paused) resumeIdling(id);
    }
}

void ProcessManager::stopAll() {
    for (int id : m_sessions.keys()) stopIdling(id);
}

void ProcessManager::requestUpdate() {
    if (!m_updateTimer->isActive()) m_updateTimer->start();
}

void ProcessManager::finalizeUpdate() {
    // Persistent state sync
    QStringList persistentData;
    int activeCount = 0;

    for (const auto& s : m_sessions) {
        if (!s.paused) activeCount++;
        persistentData << QString("%1:%2:%3").arg(s.appId).arg(s.name).arg(s.paused ? 1 : 0);
    }

    AppConfig::instance().activeSessions = persistentData;
    AppConfig::instance().save();
    
    updateTrayMenu();
    emit sessionsChanged();
}

// --- Tray & Helpers ---

void ProcessManager::updateTrayMenu() {
    int active = 0;
    for(const auto& s : m_sessions) if(!s.paused) active++;
    m_trayIcon->setToolTip(QString("Steam Idler: %1 Active").arg(active));
    m_stopAllAct->setEnabled(!m_sessions.isEmpty());
}

void ProcessManager::setupTray() {
    m_trayIcon = new QSystemTrayIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon), this);
    m_trayMenu = new QMenu();
    m_restoreAct = m_trayMenu->addAction("Show App", this, [this](){ emit sessionsChanged(); }); // Simplified for demo
    m_trayMenu->addSeparator();
    m_stopAllAct = m_trayMenu->addAction("Stop All", this, &ProcessManager::stopAll);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();
}

void ProcessManager::restoreSessions() {
    if (!AppConfig::instance().autoResume) return;
    for (const QString& sess : AppConfig::instance().activeSessions) {
        QStringList p = sess.split(':');
        if (p.size() >= 2) {
            int id = p[0].toInt();
            bool paused = (p.size() >= 3) ? p[2].toInt() : false;
            if (paused) m_sessions.insert(id, {nullptr, id, p[1], QDateTime::currentSecsSinceEpoch(), true});
            else startIdling(id, p[1]);
        }
    }
    requestUpdate();
}

QList<IdleSession> ProcessManager::activeSessions() const { return m_sessions.values(); }
bool ProcessManager::isIdling(int appId) const { return m_sessions.contains(appId) && !m_sessions[appId].paused; }
qint64 ProcessManager::getDuration(int appId) const { 
    return m_sessions.contains(appId) ? (QDateTime::currentSecsSinceEpoch() - m_sessions[appId].startTime) : 0; 
}