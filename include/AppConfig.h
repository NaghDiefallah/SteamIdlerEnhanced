#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QSettings>
#include <QRect>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>

class AppConfig {
public:
    static AppConfig& instance() {
        static AppConfig inst;
        return inst;
    }

    QRect lastGeometry;
    QStringList activeSessions;
    QString currentLang = "en";
    bool autoResume = true;
    bool verboseLogging = false;
    bool startOnStartup = false;
    QString currentTheme = "Dark.qss";
    QString theme = "Material Blue";
    QString colorScheme = "Material Blue";
    bool darkMode = true;

    void load() {
        QMutexLocker locker(&m_mutex);
        QSettings s(m_org, m_app);
        
        lastGeometry = s.value("ui/geometry").toRect();
        activeSessions = s.value("state/active_sessions").toStringList();
        currentLang = s.value("ui/language", "en").toString();
        autoResume = s.value("logic/auto_resume", true).toBool();
        verboseLogging = s.value("logic/verbose_logging", false).toBool();
        startOnStartup = s.value("logic/start_on_startup", false).toBool();
        currentTheme = s.value("ui/theme", "Dark.qss").toString();
        theme = s.value("ui/material_theme", "Material Blue").toString();
        colorScheme = s.value("ui/color_scheme", "Material Blue").toString();
        darkMode = s.value("ui/dark_mode", true).toBool();
    }

    void save() {
        QMutexLocker locker(&m_mutex);
        QSettings s(m_org, m_app);
        
        s.setValue("ui/geometry", lastGeometry);
        s.setValue("ui/language", currentLang);
        s.setValue("ui/theme", currentTheme);
        s.setValue("ui/material_theme", theme);
        s.setValue("ui/color_scheme", colorScheme);
        s.setValue("ui/dark_mode", darkMode);
        s.setValue("state/active_sessions", activeSessions);
        s.setValue("logic/auto_resume", autoResume);
        s.setValue("logic/verbose_logging", verboseLogging);
        s.setValue("logic/start_on_startup", startOnStartup);
        s.sync();
    }

    void updateActiveSessions(const QStringList& sessions) {
        if (activeSessions != sessions) {
            activeSessions = sessions;
            save();
        }
    }

private:
    AppConfig() { load(); }
    
    QMutex m_mutex;
    const QString m_org = "Nagh Diefalla";
    const QString m_app = "SteamIdlerEnhanced";

    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
};

#endif