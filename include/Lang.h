#ifndef LANG_H
#define LANG_H

#include <QString>
#include <QJsonObject>

class Lang {
public:
    static void load(const QString& langCode);
    
    // Updated overloads to match Lang.cpp
    static QString get(const QString& key, const QString& arg1 = "");
    static QString get(const QString& key, const QString& arg1, const QString& arg2);
    
    static bool isRTL();

private:
    static QJsonObject m_translations;
    static QString m_currentLang; // Added missing member
};

#endif