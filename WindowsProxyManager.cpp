#include "WindowsProxyManager.h"

#include <QSettings>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wininet.h>
#endif

WindowsProxyManager::~WindowsProxyManager()
{
    QString ignored;
    restore(&ignored);
}

bool WindowsProxyManager::isManaged() const
{
    return managed_;
}

bool WindowsProxyManager::captureCurrentSettings()
{
#ifdef Q_OS_WIN
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
                       QSettings::NativeFormat);
    previousEnabled_ = settings.value(QStringLiteral("ProxyEnable"), 0).toInt() != 0;
    previousProxyServer_ = settings.value(QStringLiteral("ProxyServer")).toString();
    previousProxyOverride_ = settings.value(QStringLiteral("ProxyOverride")).toString();
    return true;
#else
    return false;
#endif
}

bool WindowsProxyManager::apply(bool enabled, const QString &proxyServer, const QString &proxyOverride, QString *error)
{
#ifdef Q_OS_WIN
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
                       QSettings::NativeFormat);
    settings.setValue(QStringLiteral("ProxyEnable"), enabled ? 1 : 0);
    settings.setValue(QStringLiteral("ProxyServer"), proxyServer);
    settings.setValue(QStringLiteral("ProxyOverride"), proxyOverride);
    settings.sync();

    if (!InternetSetOptionW(nullptr, INTERNET_OPTION_SETTINGS_CHANGED, nullptr, 0) ||
        !InternetSetOptionW(nullptr, INTERNET_OPTION_REFRESH, nullptr, 0)) {
        if (error) {
            *error = QStringLiteral("Windows не применил настройки системного прокси");
        }
        return false;
    }
    return true;
#else
    Q_UNUSED(enabled);
    Q_UNUSED(proxyServer);
    Q_UNUSED(proxyOverride);
    if (error) {
        *error = QStringLiteral("Системный прокси реализован только для Windows");
    }
    return false;
#endif
}

bool WindowsProxyManager::enableProxy(QString *error)
{
#ifdef Q_OS_WIN
    if (!managed_) {
        captureCurrentSettings();
    }

    const QString proxyServer = QStringLiteral("http=127.0.0.1:10809;https=127.0.0.1:10809;socks=127.0.0.1:10808");
    const QString proxyOverride = QStringLiteral("<local>");
    if (!apply(true, proxyServer, proxyOverride, error)) {
        return false;
    }

    managed_ = true;
    return true;
#else
    if (error) {
        *error = QStringLiteral("Системный прокси реализован только для Windows");
    }
    return false;
#endif
}

bool WindowsProxyManager::restore(QString *error)
{
#ifdef Q_OS_WIN
    if (!managed_) {
        return true;
    }

    const bool ok = apply(previousEnabled_, previousProxyServer_, previousProxyOverride_, error);
    if (ok) {
        managed_ = false;
    }
    return ok;
#else
    if (error) {
        *error = QStringLiteral("Системный прокси реализован только для Windows");
    }
    return false;
#endif
}
