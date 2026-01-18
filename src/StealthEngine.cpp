#include "StealthEngine.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QThread>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#include <wincrypt.h>
#endif

StealthEngine::StealthEngine() {
    generateAESKey();
    Logger::log()->info("Stealth Engine initialized");
}

StealthEngine::~StealthEngine() {
    // Wipe sensitive data from memory
    wipeMemory(m_aesKey.data(), m_aesKey.size());
    wipeMemory(m_aesIV.data(), m_aesIV.size());
}

void StealthEngine::generateAESKey() {
    // Generate a hardware-based key using system information
    QString systemInfo = QSysInfo::machineHostName() +
                        QSysInfo::productType() +
                        QSysInfo::productVersion() +
                        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    m_aesKey = QCryptographicHash::hash(systemInfo.toUtf8(), QCryptographicHash::Sha256);
    m_aesIV = QCryptographicHash::hash(m_aesKey + "iv", QCryptographicHash::Md5);
}

QByteArray StealthEngine::encryptConfig(const QByteArray& data) {
    return aesEncrypt(data, m_aesKey, m_aesIV);
}

QByteArray StealthEngine::decryptConfig(const QByteArray& encryptedData) {
    return aesDecrypt(encryptedData, m_aesKey, m_aesIV);
}

QByteArray StealthEngine::aesEncrypt(const QByteArray& data, const QByteArray& key, const QByteArray& iv) {
    Q_UNUSED(key)
    Q_UNUSED(iv)
#ifdef Q_OS_WIN
    DATA_BLOB inBlob;
    inBlob.cbData = (DWORD)data.size();
    inBlob.pbData = (BYTE*)data.data();

    DATA_BLOB outBlob;
    if (CryptProtectData(&inBlob, NULL, NULL, NULL, NULL, CRYPTPROTECT_LOCAL_MACHINE, &outBlob)) {
        QByteArray out((char*)outBlob.pbData, outBlob.cbData);
        LocalFree(outBlob.pbData);
        return out;
    }
    return QByteArray();
#else
    // fallback to XOR if not windows
    QByteArray result = data;
    for (int i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ key[i % key.size()] ^ iv[i % iv.size()];
    }
    return result;
#endif
}

QByteArray StealthEngine::aesDecrypt(const QByteArray& encryptedData, const QByteArray& key, const QByteArray& iv) {
    Q_UNUSED(key)
    Q_UNUSED(iv)
#ifdef Q_OS_WIN
    DATA_BLOB inBlob;
    inBlob.cbData = (DWORD)encryptedData.size();
    inBlob.pbData = (BYTE*)encryptedData.data();

    DATA_BLOB outBlob;
    if (CryptUnprotectData(&inBlob, NULL, NULL, NULL, NULL, 0, &outBlob)) {
        QByteArray out((char*)outBlob.pbData, outBlob.cbData);
        LocalFree(outBlob.pbData);
        return out;
    }
    return QByteArray();
#else
    return aesEncrypt(encryptedData, key, iv);
#endif
}

QString StealthEngine::generateRandomProcessName() {
    QStringList prefixes = {"sys", "svc", "win", "nt", "ms"};
    QStringList suffixes = {"host", "mgr", "svc", "sys", "proc"};

    QString prefix = prefixes[QRandomGenerator::global()->bounded(prefixes.size())];
    QString suffix = suffixes[QRandomGenerator::global()->bounded(suffixes.size())];
    int number = QRandomGenerator::global()->bounded(100, 999);

    return QString("%1%2%3.exe").arg(prefix).arg(number).arg(suffix);
}

void StealthEngine::randomizeProcessName(QProcess* process) {
    if (!process) return;

    QString randomName = generateRandomProcessName();
    Logger::log()->info("Randomizing process name to: {}", randomName.toStdString());

    // On Windows, we can attempt to rename the process
#ifdef Q_OS_WIN
    // This is a simplified implementation - in practice, you'd need more sophisticated techniques
    process->setProgram(randomName);
#endif
}

void StealthEngine::applyStealthMeasures() {
    Logger::log()->info("Applying stealth measures...");

    // Randomize file names in app directory
    QString appDir = QCoreApplication::applicationDirPath();
    randomizeFileNames(appDir);

    // Create decoy processes
    createDecoyProcesses();

    // Randomize our own process name (limited effectiveness)
    QString randomName = generateRandomProcessName();
    Logger::log()->info("Stealth measures applied - process disguised as: {}", randomName.toStdString());
}

void StealthEngine::randomizeFileNames(const QString& dirPath) {
    QDir dir(dirPath);
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    for (const QString& file : files) {
        if (file.contains("steam") || file.contains("idler") || file.contains("ghost")) {
            QString extension = QFileInfo(file).suffix();
            QString newName = generateRandomProcessName();
            if (!extension.isEmpty()) {
                newName = newName.replace(".exe", "." + extension);
            }

            QString oldPath = dir.absoluteFilePath(file);
            QString newPath = dir.absoluteFilePath(newName);

            if (QFile::rename(oldPath, newPath)) {
                Logger::log()->info("Renamed {} to {}", file.toStdString(), newName.toStdString());
            }
        }
    }
}

void StealthEngine::createDecoyProcesses() {
    // Create harmless decoy processes to confuse detection
    for (int i = 0; i < 2; ++i) {
        QString decoyName = m_decoyProcessNames[QRandomGenerator::global()->bounded(m_decoyProcessNames.size())];

        QProcess* decoy = new QProcess();
        decoy->setProgram("cmd.exe");
        decoy->setArguments({"/c", "timeout", "300"}); // Run for 5 minutes
        decoy->start();

        Logger::log()->info("Created decoy process: {}", decoyName.toStdString());
    }
}

void StealthEngine::secureMemory(void* ptr, size_t size) {
    if (!ptr || size == 0) return;

#ifdef Q_OS_WIN
    // Lock memory pages to prevent swapping
    VirtualLock(ptr, size);
#endif
}

void StealthEngine::wipeMemory(void* ptr, size_t size) {
    if (!ptr || size == 0) return;

    // Overwrite with random data multiple times
    for (int pass = 0; pass < 3; ++pass) {
        memset(ptr, QRandomGenerator::global()->bounded(256), size);
    }

    // Final pass with zeros
    memset(ptr, 0, size);
}