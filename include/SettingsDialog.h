#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVariantList>

// Forward declarations to speed up compilation
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QProgressBar;
class QHBoxLayout;
class QNetworkAccessManager;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    
    // Virtual destructor is good practice for QObject derived classes
    virtual ~SettingsDialog() = default;

    void retranslateUi();

signals:
    void settingsChanged();

protected:
    void reject() override; 

private slots:
    void onThemePreview(int index);
    void onLanguageChanged(int index);
    void onResetDefaults();
    void onApply();
    void startAppSync();
    void fetchNextBatch();

private:
    // UI Initialization
    void setupUI();
    void applyDialogStyle();
    
    // Logic Helpers
    void applyThemeFile(const QString& filename);
    void updateRegistry(bool enable);
    void finalizeSync();
    
    // Layout & Widget Factories (Make them const where possible)
    void updateHeader(QHBoxLayout* layout, const QString& emoji, const QString& key);
    QHBoxLayout* createSettingRow(QLabel* label, QWidget* widget);
    QHBoxLayout* createSectionLabel(const QString& text, const QString& emoji);
    QCheckBox* createStyledCheck(const QString& text, bool checked);

    // --- UI Elements ---
    QWidget* m_contentWidget = nullptr;
    
    // Labels
    QLabel *m_titleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QLabel *m_themeLabel = nullptr;
    QLabel *m_langLabel = nullptr;

    // Layouts (Stored to update text during retranslate)
    QHBoxLayout *m_appearanceHeader = nullptr;
    QHBoxLayout *m_behaviorHeader = nullptr;
    QHBoxLayout *m_advancedHeader = nullptr;

    // Controls
    QCheckBox *m_autoResumeCheck = nullptr;
    QCheckBox *m_verboseCheck = nullptr;
    QCheckBox *m_startupCheck = nullptr;
    QComboBox *m_langCombo = nullptr;
    QComboBox *m_themeCombo = nullptr;

    // Buttons
    QPushButton *m_btnOk = nullptr;
    QPushButton *m_btnCancel = nullptr;
    QPushButton *m_btnReset = nullptr;
    QPushButton *m_btnRefreshApps = nullptr;

    QProgressBar* m_updateProgress = nullptr;

    // --- Backend ---
    QNetworkAccessManager* m_netManager = nullptr;
    QVariantList m_collectedApps;
    int m_lastAppId = 0;
    QString m_initialTheme; // Stores theme on open for 'Cancel' rollback
};

#endif // SETTINGSDIALOG_H