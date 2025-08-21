#ifndef SYSTEMDATA_H
#define SYSTEMDATA_H

#include <QString>
#include <QList>

struct ProcessData
{
    int pid;
    QString name;
    double memUsageMB;
};

struct SystemData
{
    // Dynamic Data
    double cpuPercentage;
    double memPercentage;
    double diskPercentage;
    double netDownSpeed_KBps;
    double netUpSpeed_KBps;
    QList<ProcessData> processes;

    // Static Data
    QString hostname;
    QString kernelVersion;
    QString cpuModel;
    long long totalSystemMemoryMB;
};

#endif // SYSTEMDATA_H
