#include "SettingsDialog.h"
#include "SettingsDialog.h"
#include "AppConfig.h"
#include "Lang.h"
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDir>
#include <QApplication>
#include <QHeaderView>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    m_initialTheme = AppConfig::instance().currentTheme;
    m_netManager = new QNetworkAccessManager(this);
    setupUI();
}

void SettingsDialog::setupUI() {
    setWindowTitle(Lang::get("menu_settings"));
    setMinimumWidth(550);
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_contentWidget = new QWidget();
    auto* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(40, 30, 40, 30);
    contentLayout->setSpacing(8);

    // Header
    m_titleLabel = new QLabel(Lang::get("menu_settings"));
    m_titleLabel->setStyleSheet("font-size: 26px; font-weight: 800; color: #ffffff; margin-bottom: 5px;");
    contentLayout->addWidget(m_titleLabel);

    m_descLabel = new QLabel(Lang::get("settings_desc"));
    m_descLabel->setStyleSheet("color: #777; font-size: 13px; margin-bottom: 20px;");
    contentLayout->addWidget(m_descLabel);

    // --- Appearance Section ---
    m_appearanceHeader = createSectionLabel(Lang::get("section_appearance"), "🎨");
    contentLayout->addLayout(m_appearanceHeader);
    
    m_themeCombo = new QComboBox();
    m_themeCombo->setFixedWidth(200);
    QDir themeDir(QCoreApplication::applicationDirPath() + "/themes");
    QStringList themes = themeDir.entryList({"*.qss"}, QDir::Files);
    for (const QString& t : themes) m_themeCombo->addItem(t.section('.', 0, 0), t);
    m_themeCombo->setCurrentIndex(m_themeCombo->findData(AppConfig::instance().currentTheme));
    
    m_themeLabel = new QLabel(Lang::get("label_interface_theme"));
    contentLayout->addLayout(createSettingRow(m_themeLabel, m_themeCombo));

    m_langCombo = new QComboBox();
    m_langCombo->setFixedWidth(200);
    m_langCombo->addItem("🇺🇸 English", "en");
    m_langCombo->addItem("🇸🇦 العربية", "ar");
    m_langCombo->addItem("🇩🇪 Deutsch", "de");
    m_langCombo->addItem("🇪🇸 Español", "es");
    m_langCombo->addItem("🇨🇳 简体中文", "zh");
    m_langCombo->setCurrentIndex(m_langCombo->findData(AppConfig::instance().currentLang));
    
    m_langLabel = new QLabel(Lang::get("label_display_language"));
    contentLayout->addLayout(createSettingRow(m_langLabel, m_langCombo));

    // --- Behavior Section ---
    m_behaviorHeader = createSectionLabel(Lang::get("section_behavior"), "⚙️");
    contentLayout->addLayout(m_behaviorHeader);
    
    m_autoResumeCheck = createStyledCheck(Lang::get("opt_autoresume"), AppConfig::instance().autoResume);
    m_startupCheck = createStyledCheck(Lang::get("opt_startup"), AppConfig::instance().startOnStartup);
    contentLayout->addWidget(m_autoResumeCheck);
    contentLayout->addWidget(m_startupCheck);

    // --- Advanced / Sync Section ---
    m_advancedHeader = createSectionLabel(Lang::get("section_advanced"), "🧪");
    contentLayout->addLayout(m_advancedHeader);
    
    m_verboseCheck = createStyledCheck(Lang::get("opt_verbose"), AppConfig::instance().verboseLogging);
    contentLayout->addWidget(m_verboseCheck);

    m_btnRefreshApps = new QPushButton(Lang::get("btn_refresh"));
    m_btnRefreshApps->setObjectName("SecondaryBtn");
    contentLayout->addWidget(m_btnRefreshApps);

    m_updateProgress = new QProgressBar();
    m_updateProgress->setVisible(false);
    m_updateProgress->setFixedHeight(4);
    m_updateProgress->setTextVisible(false);
    contentLayout->addWidget(m_updateProgress);

    mainLayout->addWidget(m_contentWidget);

    // --- Footer ---
    auto* footer = new QFrame();
    footer->setObjectName("FooterFrame");
    footer->setFixedHeight(80);
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(30, 0, 30, 0);

    m_btnReset = new QPushButton(Lang::get("btn_reset"));
    m_btnReset->setObjectName("ResetBtn");
    m_btnOk = new QPushButton(Lang::get("btn_save"));
    m_btnCancel = new QPushButton(Lang::get("btn_cancel"));
    m_btnOk->setObjectName("PrimaryBtn");
    m_btnCancel->setObjectName("SecondaryBtn");

    footerLayout->addWidget(m_btnReset);
    footerLayout->addStretch();
    footerLayout->addWidget(m_btnCancel);
    footerLayout->addWidget(m_btnOk);
    mainLayout->addWidget(footer);

    applyDialogStyle();

    // --- Connections ---
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onThemePreview);
    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onLanguageChanged);
    connect(m_btnOk, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_btnCancel, &QPushButton::clicked, this, &SettingsDialog::reject);
    connect(m_btnReset, &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);
    connect(m_btnRefreshApps, &QPushButton::clicked, this, &SettingsDialog::startAppSync);
}

// --- Steam API Sync Implementation ---
void SettingsDialog::startAppSync() {
    m_lastAppId = 0;
    m_collectedApps.clear();
    m_btnRefreshApps->setEnabled(false);
    m_updateProgress->setVisible(true);
    m_updateProgress->setRange(0, 0); 
    fetchNextBatch();
}

void SettingsDialog::fetchNextBatch() {
    const QString API_KEY = "9FAF65DF02C8F2D6FC71B92DBBADDE52"; // Recommended: move to AppConfig
    QUrl url("https://api.steampowered.com/IStoreService/GetAppList/v1/");
    QUrlQuery query;
    query.addQueryItem("key", API_KEY);
    query.addQueryItem("last_appid", QString::number(m_lastAppId));
    query.addQueryItem("max_results", "50000");
    url.setQuery(query);

    QNetworkReply* reply = m_netManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Sync Error", reply->errorString());
            m_btnRefreshApps->setEnabled(true);
            m_updateProgress->setVisible(false);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject resp = doc.object()["response"].toObject();
        QJsonArray apps = resp["apps"].toArray();

        for (const QJsonValue& v : apps) m_collectedApps.append(v.toVariant());

        if (resp["have_more_results"].toBool() && !apps.isEmpty()) {
            m_lastAppId = apps.last().toObject()["appid"].toInt();
            fetchNextBatch();
        } else {
            finalizeSync();
        }
    });
}

void SettingsDialog::finalizeSync() {
    QSqlDatabase db = QSqlDatabase::database();
    if (db.transaction()) {
        QSqlQuery q;
        q.exec("DELETE FROM games");
        q.prepare("INSERT INTO games (appid, name) VALUES (?, ?)");
        
        for (const QVariant& v : m_collectedApps) {
            QVariantMap m = v.toMap();
            q.addBindValue(m["appid"]);
            q.addBindValue(m["name"]);
            q.exec();
        }
        
        if (!db.commit()) {
            QMessageBox::critical(this, "DB Error", db.lastError().text());
        }
    }

    m_btnRefreshApps->setEnabled(true);
    m_updateProgress->setVisible(false);
    QMessageBox::information(this, Lang::get("menu_settings"), 
                             QString("Successfully synced %1 apps.").arg(m_collectedApps.size()));
}

// --- Logic & Translation ---
void SettingsDialog::onApply() {
    auto& config = AppConfig::instance();
    config.autoResume = m_autoResumeCheck->isChecked();
    config.verboseLogging = m_verboseCheck->isChecked();
    config.startOnStartup = m_startupCheck->isChecked();
    config.currentLang = m_langCombo->currentData().toString();
    config.currentTheme = m_themeCombo->currentData().toString();
    config.save();

    updateRegistry(config.startOnStartup);
    emit settingsChanged();
    this->accept();
}

void SettingsDialog::reject() {
    applyThemeFile(m_initialTheme);
    QDialog::reject();
}

void SettingsDialog::onThemePreview(int index) {
    applyThemeFile(m_themeCombo->itemData(index).toString());
}

void SettingsDialog::onLanguageChanged(int index) {
    QString langCode = m_langCombo->itemData(index).toString();
    Lang::load(langCode);
    retranslateUi();
}

void SettingsDialog::onResetDefaults() {
    if (QMessageBox::question(this, Lang::get("menu_settings"), Lang::get("msg_confirm_reset")) == QMessageBox::Yes) {
        m_autoResumeCheck->setChecked(true);
        m_verboseCheck->setChecked(false);
        m_startupCheck->setChecked(false);
        m_langCombo->setCurrentIndex(m_langCombo->findData("en"));
        int darkIdx = m_themeCombo->findData("Dark.qss");
        if (darkIdx != -1) m_themeCombo->setCurrentIndex(darkIdx);
    }
}

void SettingsDialog::retranslateUi() {
    this->setUpdatesEnabled(false);

    auto* effect = new QGraphicsOpacityEffect(m_contentWidget);
    m_contentWidget->setGraphicsEffect(effect);
    auto* anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutQuad);

    setWindowTitle(Lang::get("menu_settings"));
    m_titleLabel->setText(Lang::get("menu_settings"));
    m_descLabel->setText(Lang::get("settings_desc"));
    m_themeLabel->setText(Lang::get("label_interface_theme"));
    m_langLabel->setText(Lang::get("label_display_language"));
    
    m_autoResumeCheck->setText(Lang::get("opt_autoresume"));
    m_startupCheck->setText(Lang::get("opt_startup"));
    m_verboseCheck->setText(Lang::get("opt_verbose"));
    m_btnRefreshApps->setText(Lang::get("btn_refresh"));
    
    m_btnReset->setText(Lang::get("btn_reset"));
    m_btnOk->setText(Lang::get("btn_save"));
    m_btnCancel->setText(Lang::get("btn_cancel"));

    updateHeader(m_appearanceHeader, "🎨", "section_appearance");
    updateHeader(m_behaviorHeader, "⚙️", "section_behavior");
    updateHeader(m_advancedHeader, "🧪", "section_advanced");

    this->setLayoutDirection(AppConfig::instance().currentLang == "ar" ? Qt::RightToLeft : Qt::LeftToRight);
    
    this->setUpdatesEnabled(true);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// --- Helpers ---
void SettingsDialog::updateHeader(QHBoxLayout* layout, const QString& emoji, const QString& key) {
    if (!layout) return;
    auto* item = layout->itemAt(0);
    if (item && item->widget()) {
        if (auto* lbl = qobject_cast<QLabel*>(item->widget())) {
            lbl->setText(QString("%1  %2").arg(emoji, Lang::get(key).toUpper()));
        }
    }
}

QHBoxLayout* SettingsDialog::createSettingRow(QLabel* label, QWidget* widget) {
    auto* layout = new QHBoxLayout();
    label->setStyleSheet("color: #bbb; font-weight: 500;");
    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(widget);
    return layout;
}

QHBoxLayout* SettingsDialog::createSectionLabel(const QString& text, const QString& emoji) {
    auto* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 20, 0, 10);
    auto* lbl = new QLabel(QString("%1  %2").arg(emoji, text.toUpper()));
    lbl->setObjectName("SectionHeader");
    layout->addWidget(lbl);
    return layout;
}

QCheckBox* SettingsDialog::createStyledCheck(const QString& text, bool checked) {
    auto* cb = new QCheckBox(text);
    cb->setChecked(checked);
    cb->setCursor(Qt::PointingHandCursor);
    return cb;
}

void SettingsDialog::applyThemeFile(const QString& filename) {
    if (filename.isEmpty()) return;
    QString path = QCoreApplication::applicationDirPath() + "/themes/" + filename;
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(file.readAll());
        file.close();
    }
}

void SettingsDialog::updateRegistry(bool enable) {
    QSettings runSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enable) {
        QString path = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        runSettings.setValue("SteamIdlerEnhanced", "\"" + path + "\"");
    } else {
        runSettings.remove("SteamIdlerEnhanced");
    }
}

void SettingsDialog::applyDialogStyle() {
    setStyleSheet(R"(
        QDialog { background-color: #0f0f0f; }
        QLabel { color: #e0e0e0; font-family: 'Segoe UI'; font-size: 13px; }
        QLabel#SectionHeader { color: #0078d4; font-size: 11px; font-weight: bold; letter-spacing: 1.5px; }
        QCheckBox { color: #ccc; spacing: 12px; font-size: 13px; padding: 5px 0; }
        QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px; border: 1px solid #333; background: #181818; }
        QCheckBox::indicator:checked { background-color: #0078d4; border-color: #0078d4; }
        QComboBox { background: #181818; color: #fff; border: 1px solid #333; border-radius: 5px; padding: 6px 12px; }
        QFrame#FooterFrame { background-color: #141414; border-top: 1px solid #222; }
        QPushButton { padding: 10px 24px; border-radius: 6px; font-weight: 700; font-size: 12px; }
        QPushButton#PrimaryBtn { background-color: #0078d4; color: white; border: none; }
        QPushButton#SecondaryBtn { background-color: transparent; color: #888; border: 1px solid #333; }
        QPushButton#ResetBtn { background-color: transparent; color: #666; border: none; text-decoration: underline; }
        QProgressBar { background-color: #181818; border: none; border-radius: 2px; }
        QProgressBar::chunk { background-color: #0078d4; }
    )");
}