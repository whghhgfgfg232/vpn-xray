#include "TrafficMonitor.h"

#include <QString>

#ifdef Q_OS_WIN
#include <iphlpapi.h>
#endif

TrafficMonitor::TrafficMonitor(QObject *parent)
    : QObject(parent)
{
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, [this]() {
        quint64 rx = 0;
        quint64 tx = 0;
        if (!readTotals(rx, tx)) {
            emit totalsChanged(QStringLiteral("н/д"), QStringLiteral("н/д"));
            return;
        }

        const quint64 deltaRx = (rx >= baselineRx_) ? (rx - baselineRx_) : 0;
        const quint64 deltaTx = (tx >= baselineTx_) ? (tx - baselineTx_) : 0;
        emit totalsChanged(formatBytes(deltaRx), formatBytes(deltaTx));
    });
}

void TrafficMonitor::start()
{
    if (!readTotals(baselineRx_, baselineTx_)) {
        baselineRx_ = 0;
        baselineTx_ = 0;
    }

    emit totalsChanged(QStringLiteral("0 B"), QStringLiteral("0 B"));
    timer_.start();
}

void TrafficMonitor::stop()
{
    timer_.stop();
}

bool TrafficMonitor::readTotals(quint64 &rx, quint64 &tx)
{
#ifdef Q_OS_WIN
    rx = 0;
    tx = 0;

    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR || table == nullptr) {
        return false;
    }

    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_IF_ROW2 &row = table->Table[i];
        if (row.OperStatus != IfOperStatusUp || row.Type == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }
        rx += row.InOctets;
        tx += row.OutOctets;
    }

    FreeMibTable(table);
    return true;
#else
    Q_UNUSED(rx);
    Q_UNUSED(tx);
    return false;
#endif
}

QString TrafficMonitor::formatBytes(quint64 bytes)
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
