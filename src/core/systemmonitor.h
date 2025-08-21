#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include "../common/systemdata.h"

class SystemMonitor : public QObject
{
    Q_OBJECT

public:
    explicit SystemMonitor(QObject *parent = nullptr);
    SystemData getSystemData() const;

public slots:
    void startMonitoring();

signals:
    void dynamicDataUpdated(const SystemData &data);
    void staticDataReady(const SystemData &data);
    void finished();

private slots:
    void pollDynamicData();

private:
    void readStaticData();
    void readDynamicData();

    double readCpuUsage();
    double readMemoryUsage();
    double readDiskUsage(const char* path);
    void readNetworkUsage();
    void readProcessList();

    QTimer *m_timer;
    SystemData m_data;

    long long m_previousCpuIdleTime = 0;
    long long m_previousCpuTotalTime = 0;

    long long m_previousNetBytesReceived = 0;
    long long m_previousNetBytesSent = 0;
    qint64 m_previousTimestamp = 0;
};

#endif // SYSTEMMONITOR_H
