#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <QString>
#include <QColor>
#include <QMap>
#include <QObject>

struct ThemeColors {
    // Primary colors
    QColor primary;
    QColor primaryContainer;
    QColor secondary;
    QColor secondaryContainer;
    QColor tertiary;
    QColor tertiaryContainer;
    
    // Semantic colors
    QColor error;
    QColor errorContainer;
    QColor success;
    QColor warning;
    
    // Surface colors
    QColor background;
    QColor surface;
    QColor surfaceVariant;
    QColor outline;
    QColor outlineVariant;
    
    // Text colors
    QColor onPrimary;
    QColor onSecondary;
    QColor onTertiary;
    QColor onSurface;
    QColor onBackground;
};

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance() {
        static ThemeManager inst;
        return inst;
    }

    bool init();
    
    // Theme selection
    void setTheme(const QString& themeName);
    QString getCurrentTheme();
    QStringList getAvailableThemes();
    
    // Color scheme
    void setColorScheme(const QString& schemeName);
    QString getCurrentColorScheme();
    QStringList getAvailableColorSchemes();
    
    // Get colors
    ThemeColors getCurrentColors();
    QColor getColor(const QString& colorName);
    
    // Material 3 Schemes
    void applyMaterialBlue();
    void applyMaterialPurple();
    void applyMaterialTeal();
    void applyMaterialOrange();
    
    // Custom themes
    void createCustomTheme(const QString& name, const ThemeColors& colors);
    void deleteCustomTheme(const QString& name);
    
    // Generate stylesheet
    QString generateStylesheet();
    QString generateStylesheetForTheme(const QString& themeName);
    
    // Dark/Light mode
    void setDarkMode(bool enabled);
    bool isDarkMode();
    void setAutoTheme(bool enabled);  // Follow system theme

signals:
    void themeChanged(const QString& themeName);
    void colorsChanged();

private:
    ThemeManager(QObject* parent = nullptr);
    ~ThemeManager();
    
    ThemeColors createMaterialBlueLight();
    ThemeColors createMaterialBlueDark();
    ThemeColors createMaterialPurpleLight();
    ThemeColors createMaterialPurpleDark();
    ThemeColors createMaterialTealLight();
    ThemeColors createMaterialTealDark();
    ThemeColors createMaterialOrangeLight();
    ThemeColors createMaterialOrangeDark();
    
    QString m_currentTheme;
    QString m_currentScheme;
    ThemeColors m_currentColors;
    bool m_darkMode;
    bool m_autoTheme;
    
    QMap<QString, ThemeColors> m_themes;
};

#endif
