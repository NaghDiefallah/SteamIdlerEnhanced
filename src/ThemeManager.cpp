#include "ThemeManager.h"
#include "Logger.h"
#include "AppConfig.h"
#include <QApplication>
#include <QStyleFactory>

ThemeManager::ThemeManager(QObject* parent) 
    : QObject(parent), m_darkMode(true), m_autoTheme(false) {
    Logger::log()->info("ThemeManager initialized");
}

ThemeManager::~ThemeManager() {
}

bool ThemeManager::init() {
    m_currentTheme = AppConfig::instance().theme.isEmpty() ? "Material Blue" : AppConfig::instance().theme;
    m_darkMode = AppConfig::instance().darkMode;
    
    // Load theme colors
    applyMaterialBlue();
    Logger::log()->info("Theme initialized: {}", m_currentTheme.toStdString());
    return true;
}

void ThemeManager::setTheme(const QString& themeName) {
    m_currentTheme = themeName;
    AppConfig::instance().theme = themeName;
    AppConfig::instance().save();
    emit themeChanged(themeName);
    Logger::log()->info("Theme set to: {}", themeName.toStdString());
}

QString ThemeManager::getCurrentTheme() {
    return m_currentTheme;
}

QStringList ThemeManager::getAvailableThemes() {
    return {"Material Blue", "Material Purple", "Material Teal", "Material Orange", "Custom"};
}

void ThemeManager::setColorScheme(const QString& schemeName) {
    m_currentScheme = schemeName;
    AppConfig::instance().colorScheme = schemeName;
    AppConfig::instance().save();
    emit colorsChanged();
}

QString ThemeManager::getCurrentColorScheme() {
    return m_currentScheme;
}

QStringList ThemeManager::getAvailableColorSchemes() {
    return {"Material Blue", "Material Purple", "Material Teal", "Material Orange"};
}

ThemeColors ThemeManager::getCurrentColors() {
    return m_currentColors;
}

QColor ThemeManager::getColor(const QString& colorName) {
    if (colorName == "primary") return m_currentColors.primary;
    if (colorName == "secondary") return m_currentColors.secondary;
    if (colorName == "background") return m_currentColors.background;
    return Qt::white;
}

void ThemeManager::applyMaterialBlue() {
    if (m_darkMode) {
        m_currentColors = createMaterialBlueDark();
    } else {
        m_currentColors = createMaterialBlueLight();
    }
    emit colorsChanged();
}

void ThemeManager::applyMaterialPurple() {
    if (m_darkMode) {
        m_currentColors = createMaterialPurpleDark();
    } else {
        m_currentColors = createMaterialPurpleLight();
    }
}

void ThemeManager::applyMaterialTeal() {
    if (m_darkMode) {
        m_currentColors = createMaterialTealDark();
    } else {
        m_currentColors = createMaterialTealLight();
    }
}

void ThemeManager::applyMaterialOrange() {
    if (m_darkMode) {
        m_currentColors = createMaterialOrangeDark();
    } else {
        m_currentColors = createMaterialOrangeLight();
    }
}

void ThemeManager::createCustomTheme(const QString& name, const ThemeColors& colors) {
    m_themes[name] = colors;
    Logger::log()->info("Custom theme created: {}", name.toStdString());
}

void ThemeManager::deleteCustomTheme(const QString& name) {
    m_themes.remove(name);
}

QString ThemeManager::generateStylesheet() {
    return generateStylesheetForTheme(m_currentTheme);
}

QString ThemeManager::generateStylesheetForTheme(const QString& themeName) {
    // Generate Material 3 compliant stylesheet
    return "";
}

void ThemeManager::setDarkMode(bool enabled) {
    m_darkMode = enabled;
    AppConfig::instance().darkMode = enabled;
    AppConfig::instance().save();
    applyMaterialBlue();  // Reapply with new mode
}

bool ThemeManager::isDarkMode() {
    return m_darkMode;
}

void ThemeManager::setAutoTheme(bool enabled) {
    m_autoTheme = enabled;
}

// Material 3 Color definitions
ThemeColors ThemeManager::createMaterialBlueLight() {
    ThemeColors c;
    c.primary = QColor(0, 120, 212);
    c.primaryContainer = QColor(229, 245, 255);
    c.secondary = QColor(89, 100, 127);
    c.secondaryContainer = QColor(219, 226, 255);
    c.background = QColor(255, 251, 254);
    c.surface = QColor(255, 251, 254);
    c.error = QColor(179, 38, 30);
    c.errorContainer = QColor(249, 222, 220);
    c.success = QColor(0, 150, 136);
    c.onPrimary = QColor(255, 255, 255);
    c.onSurface = QColor(25, 27, 30);
    c.onBackground = QColor(25, 27, 30);
    return c;
}

ThemeColors ThemeManager::createMaterialBlueDark() {
    ThemeColors c;
    c.primary = QColor(0, 120, 212);
    c.primaryContainer = QColor(0, 90, 180);
    c.secondary = QColor(191, 198, 220);
    c.secondaryContainer = QColor(66, 77, 104);
    c.background = QColor(25, 27, 30);
    c.surface = QColor(25, 27, 30);
    c.surfaceVariant = QColor(71, 71, 78);
    c.error = QColor(242, 184, 181);
    c.errorContainer = QColor(140, 29, 64);
    c.success = QColor(0, 200, 185);
    c.onPrimary = QColor(255, 255, 255);
    c.onSurface = QColor(227, 226, 230);
    c.onBackground = QColor(227, 226, 230);
    return c;
}

ThemeColors ThemeManager::createMaterialPurpleLight() {
    ThemeColors c;
    c.primary = QColor(103, 58, 183);
    c.primaryContainer = QColor(240, 230, 255);
    c.secondary = QColor(108, 91, 149);
    c.background = QColor(255, 251, 254);
    c.surface = QColor(255, 251, 254);
    c.onPrimary = QColor(255, 255, 255);
    c.onSurface = QColor(25, 27, 30);
    return c;
}

ThemeColors ThemeManager::createMaterialPurpleDark() {
    ThemeColors c;
    c.primary = QColor(206, 168, 255);
    c.primaryContainer = QColor(79, 39, 131);
    c.secondary = QColor(204, 184, 226);
    c.background = QColor(25, 27, 30);
    c.surface = QColor(25, 27, 30);
    c.onPrimary = QColor(56, 6, 122);
    c.onSurface = QColor(227, 226, 230);
    return c;
}

ThemeColors ThemeManager::createMaterialTealLight() {
    ThemeColors c;
    c.primary = QColor(0, 150, 136);
    c.primaryContainer = QColor(177, 245, 232);
    c.secondary = QColor(82, 96, 109);
    c.background = QColor(255, 251, 254);
    c.surface = QColor(255, 251, 254);
    c.onPrimary = QColor(255, 255, 255);
    c.onSurface = QColor(25, 27, 30);
    return c;
}

ThemeColors ThemeManager::createMaterialTealDark() {
    ThemeColors c;
    c.primary = QColor(177, 245, 232);
    c.primaryContainer = QColor(0, 120, 108);
    c.secondary = QColor(197, 209, 224);
    c.background = QColor(25, 27, 30);
    c.surface = QColor(25, 27, 30);
    c.onPrimary = QColor(0, 61, 55);
    c.onSurface = QColor(227, 226, 230);
    return c;
}

ThemeColors ThemeManager::createMaterialOrangeLight() {
    ThemeColors c;
    c.primary = QColor(247, 99, 12);
    c.primaryContainer = QColor(255, 237, 224);
    c.secondary = QColor(121, 84, 63);
    c.background = QColor(255, 251, 254);
    c.surface = QColor(255, 251, 254);
    c.onPrimary = QColor(255, 255, 255);
    c.onSurface = QColor(25, 27, 30);
    return c;
}

ThemeColors ThemeManager::createMaterialOrangeDark() {
    ThemeColors c;
    c.primary = QColor(255, 184, 128);
    c.primaryContainer = QColor(196, 63, 0);
    c.secondary = QColor(224, 189, 169);
    c.background = QColor(25, 27, 30);
    c.surface = QColor(25, 27, 30);
    c.onPrimary = QColor(108, 36, 0);
    c.onSurface = QColor(227, 226, 230);
    return c;
}
