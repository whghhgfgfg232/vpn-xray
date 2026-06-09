#pragma once

#include "Profile.h"
#include "RunOptions.h"

#include <QObject>
#include <QProcess>

class XrayProcess : public QObject {
    Q_OBJECT

public:
    explicit XrayProcess(QObject *parent = nullptr);

    bool start(const Profile &profile, const ClientRunOptions &options, QString *error = nullptr);
    void stop();
    bool isRunning() const;
    QString executablePath() const;

    static QString resolveExecutablePath();
    static int tcpPingMs(const QString &host, int port, int timeoutMs, QString *error = nullptr);

signals:
    void logLine(const QString &line);
    void runningChanged(bool running);
    void errorText(const QString &text);

private:
    void emitProcessOutput();
    void setRunning(bool running);

    QProcess process_;
    QString runtimeConfigPath_;
    QString executablePath_;
    bool running_ = false;
};
