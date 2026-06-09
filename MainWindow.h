#pragma once

#include "Profile.h"
#include "ProfileRepository.h"
#include "ProfileEditDialog.h"
#include "Subscription.h"
#include "SubscriptionRepository.h"
#include "WindowsProxyManager.h"
#include "XrayProcess.h"
#include "XrayStatsMonitor.h"

#include <QList>
#include <QMainWindow>
#include <QSettings>
#include <QTimer>

class QAction;
class QCheckBox;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QSystemTrayIcon;
class QTabWidget;
class QTableWidget;
class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupTray();
    void loadSettings();
    void saveSettings();
    void loadProfiles();
    void loadSubscriptions();
    void refreshProfileTable();
    void refreshSubscriptionTable();
    bool saveProfiles();
    bool saveSubscriptions();
    void importUri();
    void editSelectedProfile();
    void removeSelectedProfile();
    void pingProfile(int index, bool silent = false);
    void refreshAllPings();
    void addSubscription();
    void updateSubscriptions(bool silent = false);
    void removeSelectedSubscription();
    void importJson();
    void exportJson();
    void handleConnectToggle();
    void updateConnectButtonStyle(bool running);
    void applyTheme(bool dark);
    void updateAutoRefreshTimer();
    void applySystemProxyForCurrentState(bool showErrors = false);
    void setStatusText(const QString &text);
    QString pingText(int ms) const;
    void appendLog(const QString &text);
    int selectedProfileIndex() const;
    int selectedSubscriptionIndex() const;
    QString subscriptionNameById(const QString &id) const;
    void setRunningUiState(bool running);
    void showFromTray();
    void toggleFromTray();
    void quitFromTray();

    ProfileRepository repository_;
    SubscriptionRepository subscriptionRepository_;
    QList<Profile> profiles_;
    QList<Subscription> subscriptions_;
    XrayProcess xrayProcess_;
    XrayStatsMonitor statsMonitor_;
    WindowsProxyManager proxyManager_;
    QSettings settings_;
    QTimer autoRefreshTimer_;
    bool allowClose_ = false;

    QLineEdit *uriEdit_ = nullptr;
    QPushButton *importButton_ = nullptr;
    QPushButton *editButton_ = nullptr;
    QPushButton *pingButton_ = nullptr;
    QPushButton *updateButton_ = nullptr;
    QPushButton *removeButton_ = nullptr;
    QPushButton *importJsonButton_ = nullptr;
    QPushButton *exportJsonButton_ = nullptr;
    QTableWidget *profileTable_ = nullptr;

    QLineEdit *subscriptionUrlEdit_ = nullptr;
    QPushButton *addSubscriptionButton_ = nullptr;
    QPushButton *updateSubscriptionsButton_ = nullptr;
    QPushButton *removeSubscriptionButton_ = nullptr;
    QTableWidget *subscriptionTable_ = nullptr;

    QCheckBox *autoUpdateCheck_ = nullptr;
    QSpinBox *autoUpdateSpin_ = nullptr;
    QCheckBox *systemProxyCheck_ = nullptr;
    QCheckBox *darkThemeCheck_ = nullptr;
    QCheckBox *minimizeToTrayCheck_ = nullptr;
    QCheckBox *tunModeCheck_ = nullptr;

    QPushButton *connectButton_ = nullptr;
    QLabel *statusValue_ = nullptr;
    QLabel *downloadValue_ = nullptr;
    QLabel *uploadValue_ = nullptr;
    QLabel *systemProxyValue_ = nullptr;
    QLabel *modeValue_ = nullptr;
    QTextEdit *logView_ = nullptr;

    QSystemTrayIcon *trayIcon_ = nullptr;
    QAction *trayShowAction_ = nullptr;
    QAction *trayConnectAction_ = nullptr;
    QAction *trayQuitAction_ = nullptr;
};
