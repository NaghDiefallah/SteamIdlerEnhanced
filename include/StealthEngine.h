#ifndef STEALTHENGINE_H
#define STEALTHENGINE_H

#include <QString>
#include <QByteArray>
#include <QProcess>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTimer>
#include "Logger.h"

class StealthEngine {
public:
    static StealthEngine& instance() {
        static StealthEngine inst;
        return inst;
    }

    // AES-256 Encryption for config files
    QByteArray encryptConfig(const QByteArray& data);
    QByteArray decryptConfig(const QByteArray& encryptedData);

    // Process name randomization
    QString generateRandomProcessName();
    void randomizeProcessName(QProcess* process);

    // Anti-detection measures
    void applyStealthMeasures();
    void randomizeFileNames(const QString& dirPath);
    void createDecoyProcesses();

    // Memory protection
    void secureMemory(void* ptr, size_t size);
    void wipeMemory(void* ptr, size_t size);

private:
    StealthEngine();
    ~StealthEngine();

    QByteArray m_aesKey;
    QByteArray m_aesIV;

    QStringList m_decoyProcessNames = {
        "svchost.exe", "explorer.exe", "System", "csrss.exe",
        "winlogon.exe", "services.exe", "lsass.exe", "smss.exe"
    };

    void generateAESKey();
    QByteArray aesEncrypt(const QByteArray& data, const QByteArray& key, const QByteArray& iv);
    QByteArray aesDecrypt(const QByteArray& encryptedData, const QByteArray& key, const QByteArray& iv);
};

#endif