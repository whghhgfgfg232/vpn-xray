#pragma once

#include <QObject>
#include <QTimer>

class TrafficMonitor : public QObject {
    Q_OBJECT

public:
    explicit TrafficMonitor(QObject *parent = nullptr);

    void start();
    void stop();

signals:
    void totalsChanged(const QString &downloadText, const QString &uploadText);

private:
    static bool readTotals(quint64 &rx, quint64 &tx);
    static QString formatBytes(quint64 bytes);

    QTimer timer_;
    quint64 baselineRx_ = 0;
    quint64 baselineTx_ = 0;
};
