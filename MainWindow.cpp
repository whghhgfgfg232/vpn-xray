#include "MainWindow.h"

#include "RunOptions.h"
#include "SubscriptionFetcher.h"
#include "UriParser.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>

namespace {
QJsonArray profilesToJson(const QList<Profile> &profiles)
{
    QJsonArray array;
    for (const Profile &profile : profiles) {
        array.append(profile.toJson());
    }
    return array;
}

QJsonArray subscriptionsToJson(const QList<Subscription> &subscriptions)
{
    QJsonArray array;
    for (const Subscription &subscription : subscriptions) {
        array.append(subscription.toJson());
    }
    return array;
}

QList<Profile> profilesFromJsonArray(const QJsonArray &array)
{
    QList<Profile> profiles;
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            profiles.append(Profile::fromJson(value.toObject()));
        }
    }
    return profiles;
}

QList<Subscription> subscriptionsFromJsonArray(const QJsonArray &array)
{
    QList<Subscription> subscriptions;
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            subscriptions.append(Subscription::fromJson(value.toObject()));
        }
    }
    return subscriptions;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , settings_()
{
    setupUi();
    setupTray();
    loadSettings();
    loadProfiles();
    loadSubscriptions();
    refreshProfileTable();
    refreshSubscriptionTable();
    updateConnectButtonStyle(false);
    applyTheme(darkThemeCheck_->isChecked());
    applySystemProxyForCurrentState(false);
    updateAutoRefreshTimer();

    connect(importButton_, &QPushButton::clicked, this, &MainWindow::importUri);
    connect(editButton_, &QPushButton::clicked, this, &MainWindow::editSelectedProfile);
    connect(removeButton_, &QPushButton::clicked, this, &MainWindow::removeSelectedProfile);
    connect(pingButton_, &QPushButton::clicked, this, [this]() {
        const int row = selectedProfileIndex();
        if (row < 0) {
            QMessageBox::information(this, QStringLiteral("Пинг"), QStringLiteral("Сначала выберите профиль."));
            return;
        }
        pingProfile(row);
    });
    connect(updateButton_, &QPushButton::clicked, this, &MainWindow::refreshAllPings);
    connect(importJsonButton_, &QPushButton::clicked, this, &MainWindow::importJson);
    connect(exportJsonButton_, &QPushButton::clicked, this, &MainWindow::exportJson);
    connect(connectButton_, &QPushButton::clicked, this, &MainWindow::handleConnectToggle);

    connect(addSubscriptionButton_, &QPushButton::clicked, this, &MainWindow::addSubscription);
    connect(updateSubscriptionsButton_, &QPushButton::clicked, this, [this]() { updateSubscriptions(false); });
    connect(removeSubscriptionButton_, &QPushButton::clicked, this, &MainWindow::removeSelectedSubscription);

    connect(profileTable_, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *) { editSelectedProfile(); });
    connect(profileTable_, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = selectedProfileIndex();
        if (row >= 0 && row < profiles_.size()) {
            const Profile &profile = profiles_.at(row);
            setStatusText(QStringLiteral("Выбран сервер: %1 (%2:%3)").arg(profile.name).arg(profile.host).arg(profile.port));
        }
    });

    connect(autoUpdateCheck_, &QCheckBox::toggled, this, [this](bool) {
        saveSettings();
        updateAutoRefreshTimer();
    });
    connect(autoUpdateSpin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        saveSettings();
        updateAutoRefreshTimer();
    });
    connect(systemProxyCheck_, &QCheckBox::toggled, this, [this](bool) {
        saveSettings();
        applySystemProxyForCurrentState(true);
    });
    connect(darkThemeCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        saveSettings();
        applyTheme(checked);
    });
    connect(minimizeToTrayCheck_, &QCheckBox::toggled, this, [this](bool) {
        saveSettings();
    });
    connect(tunModeCheck_, &QCheckBox::toggled, this, [this](bool) {
        saveSettings();
    });

    connect(&autoRefreshTimer_, &QTimer::timeout, this, [this]() {
        updateSubscriptions(true);
    });

    connect(&xrayProcess_, &XrayProcess::runningChanged, this, [this](bool running) {
        setRunningUiState(running);
    });
    connect(&xrayProcess_, &XrayProcess::logLine, this, [this](const QString &line) {
        if (!line.isEmpty()) {
            appendLog(line);
        }
    });
    connect(&xrayProcess_, &XrayProcess::errorText, this, [this](const QString &text) {
        if (!text.isEmpty()) {
            appendLog(QStringLiteral("[XRAY] %1").arg(text));
            setStatusText(text);
        }
    });

    connect(&statsMonitor_, &XrayStatsMonitor::totalsChanged, this, [this](const QString &down, const QString &up) {
        downloadValue_->setText(down);
        uploadValue_->setText(up);
    });
    connect(&statsMonitor_, &XrayStatsMonitor::errorText, this, [this](const QString &text) {
        if (!text.isEmpty()) {
            appendLog(QStringLiteral("[STATS] %1").arg(text));
        }
    });
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Xray Qt Client"));
    resize(1240, 860);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("Xray / Nekoray Style Client"), central);
    title->setStyleSheet(QStringLiteral("font-size:24px; font-weight:700;"));
    auto *subtitle = new QLabel(QStringLiteral("Поддержка URI, подписок, JSON-импорта, системного прокси, tray icon и статистики xray-core."), central);
    subtitle->setStyleSheet(QStringLiteral("color:#8b949e;"));

    auto *importBox = new QGroupBox(QStringLiteral("Импорт и управление профилями"), central);
    auto *importLayout = new QVBoxLayout(importBox);
    auto *importRow = new QHBoxLayout();
    uriEdit_ = new QLineEdit(importBox);
    uriEdit_->setPlaceholderText(QStringLiteral("Вставьте vless:// / vmess:// / trojan:// / ss://"));
    uriEdit_->setClearButtonEnabled(true);
    importButton_ = new QPushButton(QStringLiteral("Импорт URI"), importBox);
    editButton_ = new QPushButton(QStringLiteral("Редактировать"), importBox);
    pingButton_ = new QPushButton(QStringLiteral("Пинг"), importBox);
    updateButton_ = new QPushButton(QStringLiteral("Обновить серверы"), importBox);
    removeButton_ = new QPushButton(QStringLiteral("Удалить"), importBox);
    importJsonButton_ = new QPushButton(QStringLiteral("Импорт JSON"), importBox);
    exportJsonButton_ = new QPushButton(QStringLiteral("Экспорт JSON"), importBox);

    importRow->addWidget(uriEdit_, 1);
    importRow->addWidget(importButton_);
    importRow->addWidget(editButton_);
    importRow->addWidget(pingButton_);
    importRow->addWidget(updateButton_);
    importRow->addWidget(removeButton_);
    importRow->addWidget(importJsonButton_);
    importRow->addWidget(exportJsonButton_);
    importLayout->addLayout(importRow);

    auto *tabs = new QTabWidget(central);

    auto *profilesTab = new QWidget(tabs);
    auto *profilesLayout = new QVBoxLayout(profilesTab);
    profileTable_ = new QTableWidget(profilesTab);
    profileTable_->setColumnCount(6);
    profileTable_->setHorizontalHeaderLabels({
        QStringLiteral("Название"),
        QStringLiteral("Тип"),
        QStringLiteral("Сервер"),
        QStringLiteral("Порт"),
        QStringLiteral("Пинг"),
        QStringLiteral("Источник")
    });
    profileTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    profileTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    profileTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    profileTable_->setAlternatingRowColors(true);
    profileTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    profileTable_->verticalHeader()->setVisible(false);
    profilesLayout->addWidget(profileTable_);
    tabs->addTab(profilesTab, QStringLiteral("Серверы"));

    auto *subscriptionsTab = new QWidget(tabs);
    auto *subscriptionsLayout = new QVBoxLayout(subscriptionsTab);
    auto *subscriptionRow = new QHBoxLayout();
    subscriptionUrlEdit_ = new QLineEdit(subscriptionsTab);
    subscriptionUrlEdit_->setPlaceholderText(QStringLiteral("Subscription URL"));
    subscriptionUrlEdit_->setClearButtonEnabled(true);
    addSubscriptionButton_ = new QPushButton(QStringLiteral("Добавить подписку"), subscriptionsTab);
    updateSubscriptionsButton_ = new QPushButton(QStringLiteral("Обновить подписки"), subscriptionsTab);
    removeSubscriptionButton_ = new QPushButton(QStringLiteral("Удалить подписку"), subscriptionsTab);
    subscriptionRow->addWidget(subscriptionUrlEdit_, 1);
    subscriptionRow->addWidget(addSubscriptionButton_);
    subscriptionRow->addWidget(updateSubscriptionsButton_);
    subscriptionRow->addWidget(removeSubscriptionButton_);

    subscriptionTable_ = new QTableWidget(subscriptionsTab);
    subscriptionTable_->setColumnCount(4);
    subscriptionTable_->setHorizontalHeaderLabels({
        QStringLiteral("Имя"),
        QStringLiteral("URL"),
        QStringLiteral("Обновлено"),
        QStringLiteral("Статус")
    });
    subscriptionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    subscriptionTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    subscriptionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    subscriptionTable_->setAlternatingRowColors(true);
    subscriptionTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    subscriptionTable_->verticalHeader()->setVisible(false);

    auto *settingsBox = new QGroupBox(QStringLiteral("Настройки клиента"), subscriptionsTab);
    auto *settingsLayout = new QGridLayout(settingsBox);
    autoUpdateCheck_ = new QCheckBox(QStringLiteral("Автообновление подписок"), settingsBox);
    autoUpdateSpin_ = new QSpinBox(settingsBox);
    autoUpdateSpin_->setRange(5, 1440);
    autoUpdateSpin_->setSuffix(QStringLiteral(" мин"));
    systemProxyCheck_ = new QCheckBox(QStringLiteral("Включать системный прокси Windows"), settingsBox);
    darkThemeCheck_ = new QCheckBox(QStringLiteral("Тёмная тема"), settingsBox);
    minimizeToTrayCheck_ = new QCheckBox(QStringLiteral("Сворачивать в трей при закрытии"), settingsBox);
    tunModeCheck_ = new QCheckBox(QStringLiteral("TUN mode (экспериментально)"), settingsBox);

    settingsLayout->addWidget(autoUpdateCheck_, 0, 0);
    settingsLayout->addWidget(autoUpdateSpin_, 0, 1);
    settingsLayout->addWidget(systemProxyCheck_, 1, 0, 1, 2);
    settingsLayout->addWidget(darkThemeCheck_, 2, 0, 1, 2);
    settingsLayout->addWidget(minimizeToTrayCheck_, 3, 0, 1, 2);
    settingsLayout->addWidget(tunModeCheck_, 4, 0, 1, 2);

    subscriptionsLayout->addLayout(subscriptionRow);
    subscriptionsLayout->addWidget(subscriptionTable_, 1);
    subscriptionsLayout->addWidget(settingsBox);
    tabs->addTab(subscriptionsTab, QStringLiteral("Подписки и настройки"));

    connectButton_ = new QPushButton(QStringLiteral("Подключить"), central);
    connectButton_->setMinimumHeight(126);

    auto *statsBox = new QGroupBox(QStringLiteral("Статус соединения"), central);
    auto *statsLayout = new QGridLayout(statsBox);
    statsLayout->addWidget(new QLabel(QStringLiteral("Статус:"), statsBox), 0, 0);
    statusValue_ = new QLabel(QStringLiteral("Отключено"), statsBox);
    statsLayout->addWidget(statusValue_, 0, 1);
    statsLayout->addWidget(new QLabel(QStringLiteral("Входящий трафик (xray stats):"), statsBox), 1, 0);
    downloadValue_ = new QLabel(QStringLiteral("0 B"), statsBox);
    statsLayout->addWidget(downloadValue_, 1, 1);
    statsLayout->addWidget(new QLabel(QStringLiteral("Исходящий трафик (xray stats):"), statsBox), 2, 0);
    uploadValue_ = new QLabel(QStringLiteral("0 B"), statsBox);
    statsLayout->addWidget(uploadValue_, 2, 1);
    statsLayout->addWidget(new QLabel(QStringLiteral("Системный прокси:"), statsBox), 3, 0);
    systemProxyValue_ = new QLabel(QStringLiteral("выкл"), statsBox);
    statsLayout->addWidget(systemProxyValue_, 3, 1);
    statsLayout->addWidget(new QLabel(QStringLiteral("Режим:"), statsBox), 4, 0);
    modeValue_ = new QLabel(QStringLiteral("SOCKS/HTTP proxy"), statsBox);
    statsLayout->addWidget(modeValue_, 4, 1);

    auto *logBox = new QGroupBox(QStringLiteral("Лог"), central);
    auto *logLayout = new QVBoxLayout(logBox);
    logView_ = new QTextEdit(logBox);
    logView_->setReadOnly(true);
    logView_->setPlaceholderText(QStringLiteral("Логи клиента, xray-core, обновления подписок и смена режимов будут здесь."));
    logLayout->addWidget(logView_);

    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);
    mainLayout->addWidget(importBox);
    mainLayout->addWidget(tabs, 2);
    mainLayout->addWidget(connectButton_);
    mainLayout->addWidget(statsBox);
    mainLayout->addWidget(logBox, 1);

    setCentralWidget(central);
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    trayIcon_ = new QSystemTrayIcon(style()->standardIcon(QStyle::SP_ComputerIcon), this);
    auto *menu = new QMenu(this);
    trayShowAction_ = menu->addAction(QStringLiteral("Показать окно"));
    trayConnectAction_ = menu->addAction(QStringLiteral("Подключить / Отключить"));
    menu->addSeparator();
    trayQuitAction_ = menu->addAction(QStringLiteral("Выход"));

    connect(trayShowAction_, &QAction::triggered, this, &MainWindow::showFromTray);
    connect(trayConnectAction_, &QAction::triggered, this, &MainWindow::toggleFromTray);
    connect(trayQuitAction_, &QAction::triggered, this, &MainWindow::quitFromTray);
    connect(trayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showFromTray();
        }
    });

    trayIcon_->setContextMenu(menu);
    trayIcon_->show();
}

void MainWindow::loadSettings()
{
    const bool darkTheme = settings_.value(QStringLiteral("ui/darkTheme"), true).toBool();
    const bool enableSystemProxy = settings_.value(QStringLiteral("network/systemProxy"), false).toBool();
    const bool autoUpdate = settings_.value(QStringLiteral("subscriptions/autoUpdate"), true).toBool();
    const int autoUpdateMinutes = settings_.value(QStringLiteral("subscriptions/autoUpdateMinutes"), 30).toInt();
    const bool minimizeToTray = settings_.value(QStringLiteral("ui/minimizeToTray"), true).toBool();
    const bool tunMode = settings_.value(QStringLiteral("network/tunMode"), false).toBool();

    darkThemeCheck_->setChecked(darkTheme);
    systemProxyCheck_->setChecked(enableSystemProxy);
    autoUpdateCheck_->setChecked(autoUpdate);
    autoUpdateSpin_->setValue(std::clamp(autoUpdateMinutes, 5, 1440));
    minimizeToTrayCheck_->setChecked(minimizeToTray);
    tunModeCheck_->setChecked(tunMode);
    modeValue_->setText(tunMode ? QStringLiteral("TUN (экспериментально)") : QStringLiteral("SOCKS/HTTP proxy"));
}

void MainWindow::saveSettings()
{
    settings_.setValue(QStringLiteral("ui/darkTheme"), darkThemeCheck_->isChecked());
    settings_.setValue(QStringLiteral("network/systemProxy"), systemProxyCheck_->isChecked());
    settings_.setValue(QStringLiteral("subscriptions/autoUpdate"), autoUpdateCheck_->isChecked());
    settings_.setValue(QStringLiteral("subscriptions/autoUpdateMinutes"), autoUpdateSpin_->value());
    settings_.setValue(QStringLiteral("ui/minimizeToTray"), minimizeToTrayCheck_->isChecked());
    settings_.setValue(QStringLiteral("network/tunMode"), tunModeCheck_->isChecked());
    settings_.sync();
}

void MainWindow::loadProfiles()
{
    QString error;
    profiles_ = repository_.load(&error);
    if (!error.isEmpty()) {
        appendLog(QStringLiteral("[WARN] %1").arg(error));
    }
}

void MainWindow::loadSubscriptions()
{
    QString error;
    subscriptions_ = subscriptionRepository_.load(&error);
    if (!error.isEmpty()) {
        appendLog(QStringLiteral("[WARN] %1").arg(error));
    }
}

void MainWindow::refreshProfileTable()
{
    profileTable_->setRowCount(profiles_.size());

    for (int row = 0; row < profiles_.size(); ++row) {
        const Profile &p = profiles_.at(row);
        profileTable_->setItem(row, 0, new QTableWidgetItem(p.name));
        profileTable_->setItem(row, 1, new QTableWidgetItem(p.scheme));
        profileTable_->setItem(row, 2, new QTableWidgetItem(p.host));
        profileTable_->setItem(row, 3, new QTableWidgetItem(QString::number(p.port)));
        profileTable_->setItem(row, 4, new QTableWidgetItem(pingText(p.lastPingMs)));
        const QString sourceText = p.source == QStringLiteral("subscription")
            ? QStringLiteral("subscription: %1").arg(subscriptionNameById(p.subscriptionId))
            : QStringLiteral("manual");
        profileTable_->setItem(row, 5, new QTableWidgetItem(sourceText));
    }

    if (!profiles_.isEmpty() && profileTable_->currentRow() < 0) {
        profileTable_->selectRow(0);
    }
}

void MainWindow::refreshSubscriptionTable()
{
    subscriptionTable_->setRowCount(subscriptions_.size());

    for (int row = 0; row < subscriptions_.size(); ++row) {
        const Subscription &subscription = subscriptions_.at(row);
        subscriptionTable_->setItem(row, 0, new QTableWidgetItem(subscription.name));
        subscriptionTable_->setItem(row, 1, new QTableWidgetItem(subscription.url));
        subscriptionTable_->setItem(row, 2, new QTableWidgetItem(subscription.lastUpdatedIso.isEmpty() ? QStringLiteral("—") : subscription.lastUpdatedIso));
        subscriptionTable_->setItem(row, 3, new QTableWidgetItem(subscription.lastError.isEmpty() ? QStringLiteral("OK") : subscription.lastError));
    }

    if (!subscriptions_.isEmpty() && subscriptionTable_->currentRow() < 0) {
        subscriptionTable_->selectRow(0);
    }
}

bool MainWindow::saveProfiles()
{
    QString error;
    const bool ok = repository_.save(profiles_, &error);
    if (!ok) {
        appendLog(QStringLiteral("[ERROR] %1").arg(error));
    }
    return ok;
}

bool MainWindow::saveSubscriptions()
{
    QString error;
    const bool ok = subscriptionRepository_.save(subscriptions_, &error);
    if (!ok) {
        appendLog(QStringLiteral("[ERROR] %1").arg(error));
    }
    return ok;
}

void MainWindow::importUri()
{
    const QString input = uriEdit_->text().trimmed();
    QString error;
    const auto parsed = UriParser::parse(input, &error);
    if (!parsed.has_value()) {
        QMessageBox::warning(this, QStringLiteral("Ошибка импорта"), error);
        return;
    }

    Profile profile = parsed.value();
    profile.source = QStringLiteral("manual");
    profile.subscriptionId.clear();
    profiles_.append(profile);
    saveProfiles();
    refreshProfileTable();
    profileTable_->selectRow(profiles_.size() - 1);
    uriEdit_->clear();
    setStatusText(QStringLiteral("Импортирован профиль: %1").arg(profile.name));
    appendLog(QStringLiteral("[IMPORT] %1 (%2://)").arg(profile.name).arg(profile.scheme));
}

void MainWindow::editSelectedProfile()
{
    const int row = selectedProfileIndex();
    if (row < 0 || row >= profiles_.size()) {
        QMessageBox::information(this, QStringLiteral("Редактор"), QStringLiteral("Сначала выберите профиль."));
        return;
    }

    ProfileEditDialog dialog(this);
    dialog.setProfile(profiles_.at(row));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Profile edited = dialog.profile();
    if (edited.host.trimmed().isEmpty() || edited.port <= 0) {
        QMessageBox::warning(this, QStringLiteral("Редактор"), QStringLiteral("Проверьте host и port."));
        return;
    }

    profiles_[row] = edited;
    saveProfiles();
    refreshProfileTable();
    profileTable_->selectRow(row);
    appendLog(QStringLiteral("[EDIT] %1").arg(edited.name));
}

void MainWindow::removeSelectedProfile()
{
    const int row = selectedProfileIndex();
    if (row < 0 || row >= profiles_.size()) {
        QMessageBox::information(this, QStringLiteral("Удаление"), QStringLiteral("Сначала выберите профиль из списка."));
        return;
    }

    if (xrayProcess_.isRunning()) {
        xrayProcess_.stop();
    }

    appendLog(QStringLiteral("[REMOVE] %1").arg(profiles_.at(row).name));
    profiles_.removeAt(row);
    saveProfiles();
    refreshProfileTable();
    setStatusText(QStringLiteral("Профиль удалён"));
}

void MainWindow::pingProfile(int index, bool silent)
{
    if (index < 0 || index >= profiles_.size()) {
        return;
    }

    QString error;
    const int ping = XrayProcess::tcpPingMs(profiles_[index].host, profiles_[index].port, 3000, &error);
    profiles_[index].lastPingMs = ping;

    if (profileTable_->item(index, 4) == nullptr) {
        profileTable_->setItem(index, 4, new QTableWidgetItem(pingText(ping)));
    } else {
        profileTable_->item(index, 4)->setText(pingText(ping));
    }

    if (ping >= 0) {
        if (!silent) {
            setStatusText(QStringLiteral("Пинг %1: %2 мс").arg(profiles_[index].name).arg(ping));
        }
        appendLog(QStringLiteral("[PING] %1 -> %2 ms").arg(profiles_[index].name).arg(ping));
    } else {
        if (!silent) {
            setStatusText(QStringLiteral("Пинг не выполнен: %1").arg(error));
        }
        appendLog(QStringLiteral("[PING] %1 -> timeout/error (%2)").arg(profiles_[index].name).arg(error));
    }

    saveProfiles();
}

void MainWindow::refreshAllPings()
{
    if (profiles_.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Обновить"), QStringLiteral("Список профилей пуст."));
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (int i = 0; i < profiles_.size(); ++i) {
        pingProfile(i, true);
        qApp->processEvents();
    }
    QApplication::restoreOverrideCursor();

    refreshProfileTable();
    setStatusText(QStringLiteral("Обновление серверов завершено"));
    appendLog(QStringLiteral("[UPDATE] Серверы перепроверены"));
}

void MainWindow::addSubscription()
{
    const QString url = subscriptionUrlEdit_->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Подписка"), QStringLiteral("Введите URL подписки."));
        return;
    }

    Subscription subscription;
    subscription.url = url;
    subscription.name = QUrl(url).host();
    if (subscription.name.isEmpty()) {
        subscription.name = QStringLiteral("Subscription %1").arg(subscriptions_.size() + 1);
    }

    subscriptions_.append(subscription);
    saveSubscriptions();
    refreshSubscriptionTable();
    subscriptionUrlEdit_->clear();
    appendLog(QStringLiteral("[SUB] Добавлена подписка %1").arg(subscription.name));
    updateSubscriptions(false);
}

void MainWindow::updateSubscriptions(bool silent)
{
    if (subscriptions_.isEmpty()) {
        if (!silent) {
            QMessageBox::information(this, QStringLiteral("Подписки"), QStringLiteral("Список подписок пуст."));
        }
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    int importedProfiles = 0;
    for (Subscription &subscription : subscriptions_) {
        if (!subscription.enabled) {
            continue;
        }

        QList<Profile> fetchedProfiles;
        QString error;
        const bool ok = SubscriptionFetcher::fetch(subscription, &fetchedProfiles, &error);
        if (!ok) {
            subscription.lastError = error;
            appendLog(QStringLiteral("[SUB][ERROR] %1 -> %2").arg(subscription.name).arg(error));
            continue;
        }

        subscription.lastError.clear();
        subscription.lastUpdatedIso = QDateTime::currentDateTime().toString(Qt::ISODate);

        for (int i = profiles_.size() - 1; i >= 0; --i) {
            if (profiles_.at(i).subscriptionId == subscription.id) {
                profiles_.removeAt(i);
            }
        }

        importedProfiles += fetchedProfiles.size();
        for (Profile &profile : fetchedProfiles) {
            profile.subscriptionId = subscription.id;
            profile.source = QStringLiteral("subscription");
            profiles_.append(profile);
        }

        appendLog(QStringLiteral("[SUB] %1 -> импортировано %2 профилей").arg(subscription.name).arg(fetchedProfiles.size()));
    }

    QApplication::restoreOverrideCursor();
    saveProfiles();
    saveSubscriptions();
    refreshProfileTable();
    refreshSubscriptionTable();

    if (!silent) {
        setStatusText(QStringLiteral("Подписки обновлены, профилей импортировано: %1").arg(importedProfiles));
    }
}

void MainWindow::removeSelectedSubscription()
{
    const int row = selectedSubscriptionIndex();
    if (row < 0 || row >= subscriptions_.size()) {
        QMessageBox::information(this, QStringLiteral("Подписки"), QStringLiteral("Сначала выберите подписку."));
        return;
    }

    const QString subscriptionId = subscriptions_.at(row).id;
    appendLog(QStringLiteral("[SUB] Удалена подписка %1").arg(subscriptions_.at(row).name));
    subscriptions_.removeAt(row);

    for (int i = profiles_.size() - 1; i >= 0; --i) {
        if (profiles_.at(i).subscriptionId == subscriptionId) {
            profiles_.removeAt(i);
        }
    }

    saveSubscriptions();
    saveProfiles();
    refreshSubscriptionTable();
    refreshProfileTable();
}

void MainWindow::importJson()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Импорт JSON"), QString(), QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("Импорт JSON"), QStringLiteral("Не удалось открыть файл."));
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, QStringLiteral("Импорт JSON"), parseError.errorString());
        return;
    }

    QList<Profile> importedProfiles;
    QList<Subscription> importedSubscriptions = subscriptions_;

    if (doc.isArray()) {
        importedProfiles = profilesFromJsonArray(doc.array());
    } else if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        importedProfiles = profilesFromJsonArray(obj.value(QStringLiteral("profiles")).toArray());
        if (obj.contains(QStringLiteral("subscriptions"))) {
            importedSubscriptions = subscriptionsFromJsonArray(obj.value(QStringLiteral("subscriptions")).toArray());
        }

        const QJsonObject settingsObject = obj.value(QStringLiteral("settings")).toObject();
        if (!settingsObject.isEmpty()) {
            darkThemeCheck_->setChecked(settingsObject.value(QStringLiteral("darkTheme")).toBool(darkThemeCheck_->isChecked()));
            systemProxyCheck_->setChecked(settingsObject.value(QStringLiteral("systemProxy")).toBool(systemProxyCheck_->isChecked()));
            autoUpdateCheck_->setChecked(settingsObject.value(QStringLiteral("autoUpdate")).toBool(autoUpdateCheck_->isChecked()));
            autoUpdateSpin_->setValue(settingsObject.value(QStringLiteral("autoUpdateMinutes")).toInt(autoUpdateSpin_->value()));
            minimizeToTrayCheck_->setChecked(settingsObject.value(QStringLiteral("minimizeToTray")).toBool(minimizeToTrayCheck_->isChecked()));
            tunModeCheck_->setChecked(settingsObject.value(QStringLiteral("tunMode")).toBool(tunModeCheck_->isChecked()));
            saveSettings();
            applyTheme(darkThemeCheck_->isChecked());
            updateAutoRefreshTimer();
        }
    }

    if (importedProfiles.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Импорт JSON"), QStringLiteral("В файле не найдено профилей."));
        return;
    }

    profiles_ = importedProfiles;
    subscriptions_ = importedSubscriptions;
    saveProfiles();
    saveSubscriptions();
    refreshProfileTable();
    refreshSubscriptionTable();
    appendLog(QStringLiteral("[JSON] Импортировано профилей: %1").arg(profiles_.size()));
}

void MainWindow::exportJson()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Экспорт JSON"), QStringLiteral("xray_client_export.json"), QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }

    QJsonObject root;
    root.insert(QStringLiteral("profiles"), profilesToJson(profiles_));
    root.insert(QStringLiteral("subscriptions"), subscriptionsToJson(subscriptions_));
    root.insert(QStringLiteral("settings"), QJsonObject{
        {QStringLiteral("darkTheme"), darkThemeCheck_->isChecked()},
        {QStringLiteral("systemProxy"), systemProxyCheck_->isChecked()},
        {QStringLiteral("autoUpdate"), autoUpdateCheck_->isChecked()},
        {QStringLiteral("autoUpdateMinutes"), autoUpdateSpin_->value()},
        {QStringLiteral("minimizeToTray"), minimizeToTrayCheck_->isChecked()},
        {QStringLiteral("tunMode"), tunModeCheck_->isChecked()}
    });

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("Экспорт JSON"), QStringLiteral("Не удалось сохранить файл."));
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    appendLog(QStringLiteral("[JSON] Экспорт сохранён: %1").arg(path));
}

void MainWindow::handleConnectToggle()
{
    if (xrayProcess_.isRunning()) {
        xrayProcess_.stop();
        return;
    }

    const int row = selectedProfileIndex();
    if (row < 0 || row >= profiles_.size()) {
        QMessageBox::information(this, QStringLiteral("Подключение"), QStringLiteral("Выберите профиль перед подключением."));
        return;
    }

    if (tunModeCheck_->isChecked()) {
        const auto result = QMessageBox::warning(
            this,
            QStringLiteral("TUN mode"),
            QStringLiteral("TUN mode помечен как экспериментальный. Для полноценной работы на Windows могут потребоваться права администратора и ручная настройка маршрутов/интерфейса. Продолжить запуск?"),
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Cancel
        );
        if (result != QMessageBox::Ok) {
            return;
        }
    }

    ClientRunOptions options;
    options.enableTunMode = tunModeCheck_->isChecked();
    options.enableStatsApi = true;

    QString error;
    if (!xrayProcess_.start(profiles_.at(row), options, &error)) {
        QMessageBox::critical(this, QStringLiteral("Не удалось подключиться"), error);
        setStatusText(error);
        return;
    }

    statsMonitor_.setExecutablePath(xrayProcess_.executablePath());
    setStatusText(QStringLiteral("Подключение к %1").arg(profiles_.at(row).name));
    appendLog(QStringLiteral("[CONNECT] %1").arg(profiles_.at(row).name));
}

void MainWindow::updateConnectButtonStyle(bool running)
{
    if (running) {
        connectButton_->setText(QStringLiteral("Отключить"));
        connectButton_->setStyleSheet(
            QStringLiteral("QPushButton { background:#d84d57; color:white; border:none; border-radius:20px; font-size:30px; font-weight:700; padding:20px 40px; }"
                           "QPushButton:hover { background:#ff6875; }")
        );
    } else {
        connectButton_->setText(QStringLiteral("Подключить"));
        connectButton_->setStyleSheet(
            QStringLiteral("QPushButton { background:#2ea043; color:white; border:none; border-radius:20px; font-size:30px; font-weight:700; padding:20px 40px; }"
                           "QPushButton:hover { background:#3fb950; }")
        );
    }
}

void MainWindow::applyTheme(bool dark)
{
    if (!dark) {
        qApp->setStyleSheet(QString());
        updateConnectButtonStyle(xrayProcess_.isRunning());
        return;
    }

    qApp->setStyleSheet(QStringLiteral(
        "QWidget { background:#0d1117; color:#e6edf3; }"
        "QMainWindow, QDialog { background:#0d1117; }"
        "QGroupBox { border:1px solid #30363d; border-radius:14px; margin-top:10px; font-weight:600; padding-top:12px; }"
        "QGroupBox::title { subcontrol-origin: margin; left:12px; padding:0 6px; color:#9ecbff; }"
        "QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QComboBox, QTableWidget, QTabWidget::pane { background:#161b22; color:#e6edf3; border:1px solid #30363d; border-radius:10px; padding:6px; }"
        "QHeaderView::section { background:#161b22; color:#9ecbff; border:0; border-bottom:1px solid #30363d; padding:8px; }"
        "QTableWidget { gridline-color:#30363d; selection-background-color:#1f6feb; selection-color:white; alternate-background-color:#11161d; }"
        "QPushButton { background:#21262d; color:#e6edf3; border:1px solid #30363d; border-radius:10px; padding:8px 14px; }"
        "QPushButton:hover { background:#30363d; }"
        "QTabBar::tab { background:#161b22; color:#8b949e; border:1px solid #30363d; padding:10px 18px; border-top-left-radius:10px; border-top-right-radius:10px; margin-right:4px; }"
        "QTabBar::tab:selected { color:#ffffff; background:#1f6feb; }"
        "QCheckBox { spacing:8px; }"
        "QLabel { background:transparent; }"
    ));
    updateConnectButtonStyle(xrayProcess_.isRunning());
}

void MainWindow::updateAutoRefreshTimer()
{
    if (!autoUpdateCheck_->isChecked()) {
        autoRefreshTimer_.stop();
        return;
    }

    autoRefreshTimer_.start(autoUpdateSpin_->value() * 60 * 1000);
}

void MainWindow::applySystemProxyForCurrentState(bool showErrors)
{
    if (!systemProxyCheck_->isChecked()) {
        QString error;
        proxyManager_.restore(&error);
        systemProxyValue_->setText(QStringLiteral("выкл"));
        if (showErrors && !error.isEmpty()) {
            appendLog(QStringLiteral("[PROXY] %1").arg(error));
        }
        return;
    }

    if (!xrayProcess_.isRunning()) {
        systemProxyValue_->setText(QStringLiteral("ожидает подключения"));
        return;
    }

    QString error;
    if (!proxyManager_.enableProxy(&error)) {
        systemProxyValue_->setText(QStringLiteral("ошибка"));
        if (showErrors) {
            QMessageBox::warning(this, QStringLiteral("Системный прокси"), error);
        }
        appendLog(QStringLiteral("[PROXY] %1").arg(error));
        return;
    }

    systemProxyValue_->setText(QStringLiteral("вкл (127.0.0.1:10809 / 10808)"));
    appendLog(QStringLiteral("[PROXY] Системный прокси Windows включён"));
}

void MainWindow::setStatusText(const QString &text)
{
    statusValue_->setText(text);
}

QString MainWindow::pingText(int ms) const
{
    return ms >= 0 ? QStringLiteral("%1 ms").arg(ms) : QStringLiteral("—");
}

void MainWindow::appendLog(const QString &text)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    logView_->append(QStringLiteral("[%1] %2").arg(timestamp).arg(text));
}

int MainWindow::selectedProfileIndex() const
{
    const auto rows = profileTable_->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }
    return rows.first().row();
}

int MainWindow::selectedSubscriptionIndex() const
{
    const auto rows = subscriptionTable_->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }
    return rows.first().row();
}

QString MainWindow::subscriptionNameById(const QString &id) const
{
    for (const Subscription &subscription : subscriptions_) {
        if (subscription.id == id) {
            return subscription.name;
        }
    }
    return QStringLiteral("unknown");
}

void MainWindow::setRunningUiState(bool running)
{
    updateConnectButtonStyle(running);
    modeValue_->setText(tunModeCheck_->isChecked() ? QStringLiteral("TUN (экспериментально)") : QStringLiteral("SOCKS/HTTP proxy"));

    if (trayConnectAction_ != nullptr) {
        trayConnectAction_->setText(running ? QStringLiteral("Отключить") : QStringLiteral("Подключить"));
    }

    if (running) {
        statsMonitor_.setExecutablePath(xrayProcess_.executablePath());
        statsMonitor_.start();
        applySystemProxyForCurrentState(false);
    } else {
        statsMonitor_.stop();
        QString restoreError;
        proxyManager_.restore(&restoreError);
        systemProxyValue_->setText(systemProxyCheck_->isChecked() ? QStringLiteral("ожидает подключения") : QStringLiteral("выкл"));
        if (!restoreError.isEmpty()) {
            appendLog(QStringLiteral("[PROXY] %1").arg(restoreError));
        }
        setStatusText(QStringLiteral("Отключено"));
    }
}

void MainWindow::showFromTray()
{
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::toggleFromTray()
{
    handleConnectToggle();
}

void MainWindow::quitFromTray()
{
    allowClose_ = true;
    if (xrayProcess_.isRunning()) {
        xrayProcess_.stop();
    }
    close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!allowClose_ && minimizeToTrayCheck_->isChecked() && trayIcon_ != nullptr && trayIcon_->isVisible()) {
        hide();
        trayIcon_->showMessage(QStringLiteral("Xray Qt Client"), QStringLiteral("Приложение продолжает работать в системном трее."), QSystemTrayIcon::Information, 2500);
        event->ignore();
        return;
    }

    if (xrayProcess_.isRunning()) {
        xrayProcess_.stop();
    }
    proxyManager_.restore();
    event->accept();
}
