#include "XrayStatsMonitor.h"

#include <QProcess>
#include <QRegularExpression>

XrayStatsMonitor::XrayStatsMonitor(QObject *parent)
    : QObject(parent)
{
    timer_.setInterval(2000);
    connect(&timer_, &QTimer::timeout, this, &XrayStatsMonitor::queryNow);
}

void XrayStatsMonitor::setExecutablePath(const QString &path)
{
    executablePath_ = path;
}

void XrayStatsMonitor::start()
{
    emit totalsChanged(QStringLiteral("0 B"), QStringLiteral("0 B"));
    queryNow();
    timer_.start();
}

void XrayStatsMonitor::stop()
{
    timer_.stop();
    emit totalsChanged(QStringLiteral("0 B"), QStringLiteral("0 B"));
}

void XrayStatsMonitor::queryNow()
{
    if (executablePath_.isEmpty()) {
        emit totalsChanged(QStringLiteral("н/д"), QStringLiteral("н/д"));
        return;
    }

    QProcess process;
    process.start(executablePath_, {QStringLiteral("api"), QStringLiteral("statsquery"), QStringLiteral("--server=127.0.0.1:10085")});
    if (!process.waitForStarted(3000)) {
        emit totalsChanged(QStringLiteral("н/д"), QStringLiteral("н/д"));
        return;
    }

    if (!process.waitForFinished(4000)) {
        process.kill();
        process.waitForFinished(1000);
    }
    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput() + process.readAllStandardError());
    const quint64 downlink = parseCounter(output, QStringLiteral("outbound>>>proxy>>>traffic>>>downlink"));
    const quint64 uplink = parseCounter(output, QStringLiteral("outbound>>>proxy>>>traffic>>>uplink"));

    if (downlink == 0 && uplink == 0 && output.trimmed().isEmpty()) {
        emit totalsChanged(QStringLiteral("н/д"), QStringLiteral("н/д"));
        return;
    }

    const QString currentError = process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0
        ? output.trimmed()
        : QString();

    if (!currentError.isEmpty() && currentError != lastError_) {
        lastError_ = currentError;
        emit errorText(currentError);
    } else if (currentError.isEmpty()) {
        lastError_.clear();
    }

    emit totalsChanged(formatBytes(downlink), formatBytes(uplink));
}

quint64 XrayStatsMonitor::parseCounter(const QString &output, const QString &name)
{
    const QString escapedName = QRegularExpression::escape(name);
    const QRegularExpression protoRegex(QStringLiteral("name:\\s*\\\"") + escapedName + QStringLiteral("\\\"\\s*value:\\s*(\\d+)"));
    const auto protoMatch = protoRegex.match(output);
    if (protoMatch.hasMatch()) {
        return protoMatch.captured(1).toULongLong();
    }

    const QRegularExpression compactRegex(escapedName + QStringLiteral("\\s+(\\d+)"));
    const auto compactMatch = compactRegex.match(output);
    if (compactMatch.hasMatch()) {
        return compactMatch.captured(1).toULongLong();
    }

    return 0;
}

QString XrayStatsMonitor::formatBytes(quint64 bytes)
{
    static const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};

    double value = static_cast<double>(bytes);
    int suffix = 0;
    while (value >= 1024.0 && suffix < 4) {
        value /= 1024.0;
        ++suffix;
    }

    if (suffix == 0) {
        return QString::number(static_cast<qulonglong>(bytes)) + " B";
    }

    return QString::number(value, 'f', 2) + ' ' + suffixes[suffix];
}
