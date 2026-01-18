#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QFile>
#include <QCommandLineParser>
#include <QSharedMemory>
#include <QCoreApplication>
#include "GameBrowser.h"
#include "GameSearch.h"
#include "Logger.h"
#include "AppConfig.h"
#include "Lang.h"
#include "ProcessManager.h"
#include "StealthEngine.h"
#include "SessionHistory.h"
#include "Statistics.h"
#include "Categories.h"
#include "ThemeManager.h"
#include "DllLoader.h"

int main(int argc, char *argv[]) {
    // 1. Command Line Parsing
    QCommandLineParser parser;
    parser.setApplicationDescription("Steam Idler Enhanced");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption noGuiOption("no-gui", "Run in headless CLI mode");
    parser.addOption(noGuiOption);
    
    QCommandLineOption portableOption("portable", "Run in portable mode");
    parser.addOption(portableOption);
    
    QStringList args;
    for (int i = 0; i < argc; ++i) args << argv[i];
    parser.process(args);
    
    bool headless = parser.isSet(noGuiOption);
    bool portable = parser.isSet(portableOption);
    
    // 2. Singleton Check
    QSharedMemory sharedMemory("SteamIdlerEnhanced_Singleton");
    if (!sharedMemory.create(1)) {
        if (!headless) {
            QMessageBox::critical(nullptr, "Already Running", "Another instance is already running.");
        }
        return 1;
    }
    
    // 3. Portable Mode
    if (portable) {
        QDir::setCurrent(QCoreApplication::applicationDirPath());
    }
    
    // 4. Core Initialization
    Q_INIT_RESOURCE(resources);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    
    QCoreApplication* coreApp = nullptr;
    QApplication* guiApp = nullptr;
    
    if (headless) {
        coreApp = new QCoreApplication(argc, argv);
        coreApp->setApplicationName("SteamIdlerEnhanced");
        coreApp->setOrganizationName("IdlerCorp");
    } else {
        guiApp = new QApplication(argc, argv);
        guiApp->setApplicationName("SteamIdlerEnhanced");
        guiApp->setOrganizationName("IdlerCorp");
    }
    
    // 5. Environment Setup
    Logger::init();
    Logger::log()->info("=== Steam Idler Enhanced {} ===", headless ? "CLI Mode" : "GUI Mode");
    
    // 5.5. Initialize DLL Loader - Extract embedded DLLs
    if (!DllLoader::initialize()) {
        Logger::log()->warn("Warning: Some critical DLLs could not be extracted. The application may not function properly.");
    }
    
    // 6. Initialize Stealth Engine
    StealthEngine::instance().applyStealthMeasures();
    
    // 7. Load Configuration
    AppConfig::instance().load();
    Lang::load(AppConfig::instance().currentLang);
    
    // 8. Dependency Check - search multiple likely locations and extract bundled copies if needed
    QStringList dependencies = {"ghost_launcher.exe", "steam_api64.dll"};
    QStringList missing;

    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidateDirs;
    candidateDirs << appDir;
    candidateDirs << QDir::currentPath();
    candidateDirs << QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
    candidateDirs.removeDuplicates();

    // Ensure embedded resource extraction before final check
    for (const QString& dep : dependencies) {
        bool found = false;
        for (const QString& dir : candidateDirs) {
            QString full = QDir(dir).absoluteFilePath(dep);
            if (QFile::exists(full)) { found = true; break; }
        }

        if (!found) {
            // Attempt to extract from embedded resources
            QString resPath = ":/" + dep;
            QFile res(resPath);
            if (res.exists() && res.open(QIODevice::ReadOnly)) {
                QByteArray data = res.readAll();
                res.close();

                // Prefer application directory for extraction; fall back to AppData
                QString targetDir = appDir;
                if (!QFileInfo(targetDir).isWritable()) {
                    targetDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
                    QDir().mkpath(targetDir);
                }

                QString outPath = QDir(targetDir).absoluteFilePath(dep);
                QFile out(outPath);
                if (out.open(QIODevice::WriteOnly)) {
                    out.write(data);
                    out.close();
                    Logger::log()->info("Extracted embedded {} to {}", dep.toStdString(), outPath.toStdString());
                    // If we extracted to AppData, also copy to candidateDirs list for checks
                    if (!candidateDirs.contains(targetDir)) candidateDirs << targetDir;
                    found = true;
                } else {
                    Logger::log()->error("Failed to write embedded {} to {}", dep.toStdString(), outPath.toStdString());
                }
            } else {
                Logger::log()->info("Embedded resource not found for {}", dep.toStdString());
            }
        }

        if (!found) missing << dep;
    }

    if (!missing.isEmpty()) {
        QString details = "Tried paths:\n";
        for (const QString& dir : candidateDirs) details += QString("  %1\n").arg(dir);
        QString msg = "Missing dependencies: " + missing.join(", ") + "\n\n" + details;
        if (!headless) QMessageBox::critical(nullptr, "Missing Dependencies", msg);
        Logger::log()->error("{}", msg.toStdString());
        return 1;
    }
    
    // 9. Database and Search
    QString dbPath = "data/app_data.db";
    if (!GameSearch::instance().init(dbPath)) {
        QString msg = "Failed to initialize database";
        if (!headless) QMessageBox::critical(nullptr, "Database Error", msg);
        Logger::log()->error("{}", msg.toStdString());
        return 1;
    }
    
    // 10. Session History
    QString historyDbPath = "data/session_history.db";
    if (!SessionHistory::instance().init(historyDbPath)) {
        QString msg = "Failed to initialize session history";
        if (!headless) QMessageBox::critical(nullptr, "History Error", msg);
        Logger::log()->error("{}", msg.toStdString());
        return 1;
    }
    
    // 11. Statistics System
    QString statsDbPath = "data/app_data.db";  // Use same DB as GameSearch
    if (!Statistics::instance().init(statsDbPath)) {
        Logger::log()->warn("Failed to initialize statistics system");
    }
    
    // 12. Categories and Tags System
    if (!Categories::instance().init(statsDbPath)) {
        Logger::log()->warn("Failed to initialize categories system");
    }
    
    // 13. Theme Manager
    if (!ThemeManager::instance().init()) {
        Logger::log()->warn("Failed to initialize theme manager");
    }
    
    if (headless) {
        // Headless Mode
        ProcessManager* procManager = new ProcessManager(nullptr);
        Logger::log()->info("Running in headless mode. Use commands or signals to control.");
        return coreApp->exec();
    } else {
        // GUI Mode
        GameBrowser* browser = new GameBrowser(&GameSearch::instance(), nullptr);
        browser->show();
        
        // Start update checker
        UpdateChecker::instance().checkForUpdates();
        
        return guiApp->exec();
    }
}