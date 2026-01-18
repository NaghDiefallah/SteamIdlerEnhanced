#include "Lang.h"
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <QGuiApplication>

// Static member initialization
QJsonObject Lang::m_translations;
QString Lang::m_currentLang = "en";

void Lang::load(const QString& langCode) {
    // 1. FORCE RESOURCE INITIALIZATION
    // This is vital for MinGW/Static builds where resources might be optimized out.
    // 'resources' must match the filename of your .qrc file (without the extension)
    Q_INIT_RESOURCE(resources);

    m_currentLang = langCode;
    QString path = QString(":/data/lang/%1.json").arg(langCode);
    
    // 2. EXISTENCE CHECK & FALLBACK
    if (!QFile::exists(path)) {
        qWarning() << "Requested language not found:" << path;
        if (langCode != "en") {
            qDebug() << "Falling back to English...";
            return load("en"); // Recursive fallback to English
        }
        return; 
    }

    // 3. SECURE FILE LOADING
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            qCritical() << "JSON Parse Error in" << path << ":" << parseError.errorString();
        } else {
            m_translations = doc.object();
            qDebug() << "Successfully loaded" << m_translations.size() << "keys for" << langCode;
        }
        file.close();
    } else {
        qCritical() << "Could not open language resource:" << path << "Error:" << file.errorString();
    }

    // 4. GLOBAL UI DIRECTION (RTL vs LTR)
    // Supports languages like Arabic (ar) or Persian (fa)
    Qt::LayoutDirection dir = (langCode == "ar" || langCode == "fa") ? Qt::RightToLeft : Qt::LeftToRight;
    qApp->setLayoutDirection(dir);
}

QString Lang::get(const QString& key, const QString& arg1) {
    // If key missing, return the key itself so the dev notices it
    if (!m_translations.contains(key)) return key;

    QString text = m_translations.value(key).toString();

    // Only apply .arg() if the source string actually expects a placeholder
    // This prevents "QString::arg: Argument missing" warnings in the console
    if (!arg1.isEmpty() && text.contains("%1")) {
        return text.arg(arg1);
    }

    return text;
}

QString Lang::get(const QString& key, const QString& arg1, const QString& arg2) {
    if (!m_translations.contains(key)) return key;
    
    QString text = m_translations.value(key).toString();
    
    // Sequential replacement for multiple placeholders
    if (text.contains("%1")) text = text.arg(arg1);
    if (text.contains("%2")) text = text.arg(arg2);
    
    return text;
}

bool Lang::isRTL() {
    return qApp->layoutDirection() == Qt::RightToLeft;
}