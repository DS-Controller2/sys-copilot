#include "systemmonitor.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <sys/statvfs.h>
#include <QDebug>
#include <QDir>

SystemMonitor::SystemMonitor(QObject *parent) : QObject(parent) {}

void SystemMonitor::startMonitoring()
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SystemMonitor::pollDynamicData);

    readStaticData();
    emit staticDataReady(m_data);

    pollDynamicData();
    m_timer->start(2000); // Update every 2 seconds for process list
}

void SystemMonitor::pollDynamicData()
{
    readDynamicData();
    emit dynamicDataUpdated(m_data);
}

void SystemMonitor::readStaticData()
{
    std::string temp_str;
    std::ifstream hostFile("/proc/sys/kernel/hostname");
    if(hostFile.is_open()) { std::getline(hostFile, temp_str); m_data.hostname = QString::fromStdString(temp_str); }
    hostFile.close();

    std::ifstream kernelFile("/proc/version");
    if(kernelFile.is_open()) { std::getline(kernelFile, temp_str); m_data.kernelVersion = QString::fromStdString(temp_str); }
    kernelFile.close();

    std::ifstream cpuFile("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuFile, line)) {
        if (line.find("model name") != std::string::npos) {
            m_data.cpuModel = QString::fromStdString(line.substr(line.find(":") + 2));
            break;
        }
    }
    cpuFile.close();
}

void SystemMonitor::readDynamicData()
{
    m_data.cpuPercentage = readCpuUsage();
    m_data.memPercentage = readMemoryUsage();
    m_data.diskPercentage = readDiskUsage("/");
    readNetworkUsage();
    readProcessList();
}

void SystemMonitor::readProcessList()
{
    m_data.processes.clear();
    QDir procDir("/proc");
    QStringList entries = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for(const QString &pidStr : entries) {
        bool isNumber;
        int pid = pidStr.toInt(&isNumber);
        if (!isNumber) continue;

        ProcessData p_data;
        p_data.pid = pid;

        std::ifstream statusFile(QString("/proc/%1/status").arg(pid).toStdString());
        if(!statusFile.is_open()) continue;

        std::string line;
        long vmrss = 0;
        while(std::getline(statusFile, line)) {
            std::stringstream ss(line);
            std::string key;
            ss >> key;
            if (key == "Name:") {
                std::string name;
                ss >> name;
                p_data.name = QString::fromStdString(name);
            } else if (key == "VmRSS:") {
                ss >> vmrss;
                break; // VmRSS is usually after Name
            }
        }
        statusFile.close();

        p_data.memUsageMB = vmrss / 1024.0;
        m_data.processes.append(p_data);
    }
}

void SystemMonitor::readNetworkUsage()
{
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) return;

    long long totalBytesReceived = 0;
    long long totalBytesSent = 0;
    std::string line;
    std::getline(file, line); std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string ifaceName;
        ss >> ifaceName;
        if (ifaceName.find("lo:") != std::string::npos) continue;

        long long recv, sent;
        ss >> recv; for(int i=0; i<7; ++i) ss >> sent; ss >> sent;
        totalBytesReceived += recv;
        totalBytesSent += sent;
    }
    file.close();

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_previousTimestamp > 0) {
        double timeElapsedSeconds = (currentTime - m_previousTimestamp) / 1000.0;
        if (timeElapsedSeconds > 0) {
            m_data.netDownSpeed_KBps = (totalBytesReceived - m_previousNetBytesReceived) / timeElapsedSeconds / 1024.0;
            m_data.netUpSpeed_KBps = (totalBytesSent - m_previousNetBytesSent) / timeElapsedSeconds / 1024.0;
        }
    }
    m_previousNetBytesReceived = totalBytesReceived;
    m_previousNetBytesSent = totalBytesSent;
    m_previousTimestamp = currentTime;
}

double SystemMonitor::readMemoryUsage()
{
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return 0.0;
    std::string line;
    long long memTotal = 0, memAvailable = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string key;
        long long value;
        ss >> key >> value;
        if (key == "MemTotal:") memTotal = value;
        if (key == "MemAvailable:") memAvailable = value;
    }
    file.close();
    if (memTotal == 0) return 0.0;
    m_data.totalSystemMemoryMB = memTotal / 1024;
    long long memUsed = memTotal - memAvailable;
    return 100.0 * static_cast<double>(memUsed) / static_cast<double>(memTotal);
}

double SystemMonitor::readDiskUsage(const char* path)
{
    struct statvfs stat;
    if (statvfs(path, &stat) != 0) return 0.0;
    unsigned long long totalBytes = stat.f_blocks * stat.f_frsize;
    unsigned long long freeBytes = stat.f_bfree * stat.f_frsize;
    unsigned long long usedBytes = totalBytes - freeBytes;
    return 100.0 * static_cast<double>(usedBytes) / static_cast<double>(totalBytes);
}

double SystemMonitor::readCpuUsage()
{
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0;
    std::string line;
    std::getline(file, line);
    file.close();
    std::stringstream ss(line);
    std::string cpu;
    ss >> cpu;
    std::vector<long long> times;
    long long time;
    while (ss >> time) times.push_back(time);
    if (times.size() < 4) return 0.0;
    long long idleTime = times[3] + times[4];
    long long totalTime = 0;
    for(long long t : times) totalTime += t;
    double cpuUsage = 0.0;
    if (m_previousCpuTotalTime > 0) {
        long long deltaTotal = totalTime - m_previousCpuTotalTime;
        long long deltaIdle = idleTime - m_previousCpuIdleTime;
        if (deltaTotal > 0) {
            cpuUsage = 100.0 * (1.0 - static_cast<double>(deltaIdle) / static_cast<double>(deltaTotal));
        }
    }
    m_previousCpuTotalTime = totalTime;
    m_previousCpuIdleTime = idleTime;
    return cpuUsage;
}
