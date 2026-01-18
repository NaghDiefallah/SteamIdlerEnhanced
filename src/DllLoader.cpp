#include "DllLoader.h"
#include "Logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <windows.h>

static const char* CRITICAL_DLLS[] = {
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll"
};

static const int NUM_DLLS = 3;

QString DllLoader::getCachePath() {
    // Use app cache directory for DLLs
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/dlls";
    return cachePath;
}

bool DllLoader::verifyDll(const QString& filePath) {
    QFile file(filePath);
    if (!file.exists()) return false;
    
    // Verify it's a valid PE/DLL file
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray header = file.read(2);
    file.close();
    
    // Check for MZ header (PE executable)
    return header.size() == 2 && header[0] == 'M' && header[1] == 'Z';
}

bool DllLoader::initialize() {
    Logger::log()->info("Initializing DLL loader...");
    
    QString appDir = QCoreApplication::applicationDirPath();
    Logger::log()->debug("Application directory: {}", appDir.toStdString());
    
    // First, try to use DLLs from the application directory
    // These should be deployed alongside the exe
    bool allFound = true;
    for (int i = 0; i < NUM_DLLS; ++i) {
        QString dllName = CRITICAL_DLLS[i];
        QString appPath = appDir + "/" + dllName;
        
        if (verifyDll(appPath)) {
            Logger::log()->debug("Found DLL in app directory: {}", dllName.toStdString());
        } else {
            Logger::log()->warn("DLL not found in app directory: {}", dllName.toStdString());
            allFound = false;
        }
    }
    
    // Add application directory to DLL search path
    // This ensures the app and child processes can find the DLLs
    if (!SetDllDirectoryA(appDir.toStdString().c_str())) {
        Logger::log()->warn("Failed to add application directory to DLL search path");
    } else {
        Logger::log()->info("Added application directory to DLL search path: {}", appDir.toStdString());
    }
    
    if (!allFound) {
        Logger::log()->warn("Some critical DLLs were not found. Application may not function properly.");
        Logger::log()->info("Expected DLLs in: {}", appDir.toStdString());
    } else {
        Logger::log()->info("All critical DLLs verified");
    }
    
    return allFound;
}
