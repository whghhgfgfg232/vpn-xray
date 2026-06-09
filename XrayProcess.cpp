#include "XrayProcess.h"

#include "XrayConfigBuilder.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTcpSocket>

XrayProcess::XrayProcess(QObject *parent)
    : QObject(parent)
{
    connect(&process_, &QProcess::readyReadStandardOutput, this, &XrayProcess::emitProcessOutput);
    connect(&process_, &QProcess::readyReadStandardError, this, &XrayProcess::emitProcessOutput);

    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit errorText(process_.errorString());
    });

    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int, QProcess::ExitStatus) {
                setRunning(false);
            });
}

QString XrayProcess::resolveExecutablePath()
{
    const QString base = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(base).filePath(QStringLiteral("third_party/xray/xray.exe")),
        QDir(base).filePath(QStringLiteral("xray.exe")),
        QDir(QDir::currentPath()).filePath(QStringLiteral("third_party/xray/xray.exe")),
        QDir(QDir::currentPath()).filePath(QStringLiteral("xray.exe"))
    };

    for (const QString &candidate : candidates) {
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }

    return QString();
}

bool XrayProcess::start(const Profile &profile, const ClientRunOptions &options, QString *error)
{
    stop();

    executablePath_ = resolveExecutablePath();
    if (executablePath_.isEmpty()) {
        if (error) {
            *error = QStringLiteral("xray.exe не найден. Поместите его в third_party/xray/xray.exe рядом с .exe приложения.");
        }
        return false;
    }

    const QString runtimeDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime"));
    QDir().mkpath(runtimeDir);
    runtimeConfigPath_ = QDir(runtimeDir).filePath(QStringLiteral("current-config.json"));

    QFile configFile(runtimeConfigPath_);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("Не удалось сохранить временный xray-конфиг");
        }
        return false;
    }
    configFile.write(XrayConfigBuilder::buildJson(profile, options));
    configFile.close();

    process_.setProgram(executablePath_);
    process_.setArguments({QStringLiteral("run"), QStringLiteral("-config"), runtimeConfigPath_});
    process_.setWorkingDirectory(QCoreApplication::applicationDirPath());
    process_.start();

    if (!process_.waitForStarted(5000)) {
        if (error) {
            *error = QStringLiteral("Не удалось запустить xray.exe: %1").arg(process_.errorString());
        }
        executablePath_.clear();
        return false;
    }

    setRunning(true);
    return true;
}

void XrayProcess::stop()
{
    if (process_.state() == QProcess::NotRunning) {
        setRunning(false);
        return;
    }

    process_.terminate();
    if (!process_.waitForFinished(3000)) {
        process_.kill();
        process_.waitForFinished(2000);
    }

    setRunning(false);
}

bool XrayProcess::isRunning() const
{
    return running_;
}

QString XrayProcess::executablePath() const
{
    return executablePath_;
}

int XrayProcess::tcpPingMs(const QString &host, int port, int timeoutMs, QString *error)
{
    QTcpSocket socket;
    QElapsedTimer timer;
    timer.start();

    socket.connectToHost(host, static_cast<quint16>(port));
    if (socket.waitForConnected(timeoutMs)) {
        const int elapsed = static_cast<int>(timer.elapsed());
        socket.disconnectFromHost();
        return elapsed;
    }

    if (error) {
        *error = socket.errorString();
    }
    return -1;
}

void XrayProcess::emitProcessOutput()
{
    const QByteArray out = process_.readAllStandardOutput();
    const QByteArray err = process_.readAllStandardError();

    if (!out.isEmpty()) {
        emit logLine(QString::fromLocal8Bit(out).trimmed());
    }
    if (!err.isEmpty()) {
        emit logLine(QString::fromLocal8Bit(err).trimmed());
    }
}

void XrayProcess::setRunning(bool running)
{
    if (running_ == running) {
        return;
    }

    running_ = running;
    emit runningChanged(running_);
}
