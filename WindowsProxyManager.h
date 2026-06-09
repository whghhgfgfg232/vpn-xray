#pragma once

#include <QString>

class WindowsProxyManager {
public:
    WindowsProxyManager() = default;
    ~WindowsProxyManager();

    bool enableProxy(QString *error = nullptr);
    bool restore(QString *error = nullptr);
    bool isManaged() const;

private:
    bool captureCurrentSettings();
    bool apply(bool enabled, const QString &proxyServer, const QString &proxyOverride, QString *error);

    bool managed_ = false;
    bool previousEnabled_ = false;
    QString previousProxyServer_;
    QString previousProxyOverride_;
};
