#include "mainwindow.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <algorithm>
#include <signal.h>

// Constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_copilot = new Copilot(this);
    setupUi();

    m_monitorThread = new QThread(this);
    m_monitor = new SystemMonitor();
    m_monitor->moveToThread(m_monitorThread);

    connect(m_monitorThread, &QThread::started, m_monitor, &SystemMonitor::startMonitoring);
    connect(m_monitor, &SystemMonitor::finished, m_monitorThread, &QThread::quit);
    connect(m_monitorThread, &QThread::finished, m_monitor, &SystemMonitor::deleteLater);
    connect(m_monitorThread, &QThread::finished, m_monitorThread, &QThread::deleteLater);

    connect(m_monitor, &SystemMonitor::dynamicDataUpdated, this, &MainWindow::onDynamicDataUpdated);
    connect(m_monitor, &SystemMonitor::staticDataReady, this, &MainWindow::onStaticDataReady);
    connect(m_monitor, &SystemMonitor::dynamicDataUpdated, m_copilot, &Copilot::onSystemDataUpdated); // New connection for Copilot

    m_monitorThread->start();
}

// Destructor
MainWindow::~MainWindow()
{
    if(m_monitorThread->isRunning()) {
        m_monitorThread->quit();
        m_monitorThread->wait();
    }
}

void MainWindow::onExplainClicked()
{
    auto selectedItems = m_processTableWidget->selectedItems();
    if (selectedItems.isEmpty()) return;
    QString processName = m_processTableWidget->item(selectedItems.first()->row(), 1)->text();
    m_copilot->onExplainClicked(processName);
}

// --- Core UI and Process Management Functions ---
void MainWindow::onDynamicDataUpdated(const SystemData &data)
{
    m_cpuProgressBar->setValue(static_cast<int>(data.cpuPercentage));
    m_memProgressBar->setValue(static_cast<int>(data.memPercentage));
    m_diskProgressBar->setValue(static_cast<int>(data.diskPercentage));
    applyStylesheet(m_cpuProgressBar, m_cpuProgressBar->value());
    applyStylesheet(m_memProgressBar, m_memProgressBar->value());
    applyStylesheet(m_diskProgressBar, m_diskProgressBar->value());
    m_netDownValueLabel->setText(QString::number(data.netDownSpeed_KBps, 'f', 2) + " KB/s");
    m_netUpValueLabel->setText(QString::number(data.netUpSpeed_KBps, 'f', 2) + " KB/s");
    updateProcessTable(data.processes);
}

void MainWindow::onStaticDataReady(const SystemData &data)
{
    m_hostnameValueLabel->setText(data.hostname);
    m_kernelValueLabel->setText(data.kernelVersion);
    m_cpuModelValueLabel->setText(data.cpuModel);
}

void MainWindow::onProcessSelectionChanged()
{
    bool hasSelection = m_processTableWidget->selectedItems().size() > 0;
    m_killButton->setEnabled(hasSelection);
    m_stopButton->setEnabled(hasSelection);
    m_resumeButton->setEnabled(hasSelection);
    m_explainButton->setEnabled(hasSelection);
}

int MainWindow::getSelectedPid()
{
    auto selectedItems = m_processTableWidget->selectedItems();
    if (selectedItems.isEmpty()) return -1;
    int row = selectedItems.first()->row();
    if (row >= m_processTableWidget->rowCount()) return -1;
    return m_processTableWidget->item(row, 0)->text().toInt();
}

void MainWindow::onKillClicked()
{
    int pid = getSelectedPid();
    if (pid > 0) { if (kill(pid, SIGKILL) != 0) QMessageBox::warning(this, "Error", "Could not kill process. Check permissions."); }
}

void MainWindow::onStopClicked()
{
    int pid = getSelectedPid();
    if (pid > 0) { if (kill(pid, SIGSTOP) != 0) QMessageBox::warning(this, "Error", "Could not stop process. Check permissions."); }
}

void MainWindow::onResumeClicked()
{
    int pid = getSelectedPid();
    if (pid > 0) { if (kill(pid, SIGCONT) != 0) QMessageBox::warning(this, "Error", "Could not resume process. Check permissions."); }
}

void MainWindow::updateProcessTable(const QList<ProcessData> &processes)
{
    m_processTableWidget->setSortingEnabled(false);
    QMap<int, int> currentPids;
    for(int i = 0; i < m_processTableWidget->rowCount(); ++i) {
        currentPids.insert(m_processTableWidget->item(i, 0)->text().toInt(), i);
    }
    for(const auto &process : processes) {
        if(currentPids.contains(process.pid)) {
            int row = currentPids.value(process.pid);
            m_processTableWidget->item(row, 2)->setText(QString::number(process.memUsageMB, 'f', 2) + " MB");
            currentPids.remove(process.pid);
        } else {
            int newRow = m_processTableWidget->rowCount();
            m_processTableWidget->insertRow(newRow);
            m_processTableWidget->setItem(newRow, 0, new QTableWidgetItem(QString::number(process.pid)));
            m_processTableWidget->setItem(newRow, 1, new QTableWidgetItem(process.name));
            m_processTableWidget->setItem(newRow, 2, new QTableWidgetItem(QString::number(process.memUsageMB, 'f', 2) + " MB"));
        }
    }
    QList<int> rowsToRemove = currentPids.values();
    std::sort(rowsToRemove.begin(), rowsToRemove.end(), std::greater<int>());
    for(int row : rowsToRemove) m_processTableWidget->removeRow(row);
    m_processTableWidget->setSortingEnabled(true);
}

void MainWindow::applyStylesheet(QProgressBar* bar, int value)
{
    QString style = "QProgressBar::chunk { background-color: %1; }";
    QString color; if (value > 80) color = "#d9534f"; else if (value > 60) color = "#f0ad4e"; else color = "#5cb85c";
    bar->setStyleSheet(style.arg(color));
}

void MainWindow::setupUi()
{
    setWindowTitle("Ultimate AI System Monitor");
    resize(700, 600);
    QTabWidget *tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);
    tabWidget->addTab(createMonitorTab(), "Live Monitor");
    tabWidget->addTab(createProcessTab(), "Processes");
    tabWidget->addTab(createInfoTab(), "System Information");
    tabWidget->addTab(m_copilot->createAssistantTab(), "Copilot");
}

QWidget* MainWindow::createProcessTab()
{
    QWidget *processTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(processTab);
    m_processTableWidget = new QTableWidget(this);
    m_processTableWidget->setColumnCount(3);
    m_processTableWidget->setHorizontalHeaderLabels({"PID", "Name", "Memory Usage"});
    m_processTableWidget->setSortingEnabled(true);
    m_processTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_processTableWidget->verticalHeader()->setVisible(false);
    m_processTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_processTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_processTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_processTableWidget, &QTableWidget::itemSelectionChanged, this, &MainWindow::onProcessSelectionChanged);
    layout->addWidget(m_processTableWidget);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_explainButton = new QPushButton("Explain Process (AI)", this);
    m_resumeButton = new QPushButton("Resume Process", this);
    m_stopButton = new QPushButton("Stop Process", this);
    m_killButton = new QPushButton("Kill Process", this);
    m_explainButton->setEnabled(false);
    m_resumeButton->setEnabled(false);
    m_stopButton->setEnabled(false);
    m_killButton->setEnabled(false);
    connect(m_explainButton, &QPushButton::clicked, this, &MainWindow::onExplainClicked);
    connect(m_resumeButton, &QPushButton::clicked, this, &MainWindow::onResumeClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_killButton, &QPushButton::clicked, this, &MainWindow::onKillClicked);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_explainButton);
    buttonLayout->addWidget(m_resumeButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_killButton);
    layout->addLayout(buttonLayout);
    return processTab;
}

QWidget* MainWindow::createMonitorTab()
{
    QWidget *monitorTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(monitorTab);
    QGroupBox *cpuGroup = new QGroupBox("CPU Usage", this);
    QGridLayout *cpuLayout = new QGridLayout(cpuGroup);
    m_cpuProgressBar = new QProgressBar(this);
    m_cpuProgressBar->setRange(0, 100); m_cpuProgressBar->setFormat("%p%");
    cpuLayout->addWidget(m_cpuProgressBar);
    mainLayout->addWidget(cpuGroup);
    QGroupBox *memGroup = new QGroupBox("Memory Usage", this);
    QGridLayout *memLayout = new QGridLayout(memGroup);
    m_memProgressBar = new QProgressBar(this);
    m_memProgressBar->setRange(0, 100); m_memProgressBar->setFormat("%p%");
    memLayout->addWidget(m_memProgressBar);
    mainLayout->addWidget(memGroup);
    QGroupBox *diskGroup = new QGroupBox("Disk Usage (/)", this);
    QGridLayout *diskLayout = new QGridLayout(diskGroup);
    m_diskProgressBar = new QProgressBar(this);
    m_diskProgressBar->setRange(0, 100); m_diskProgressBar->setFormat("%p%");
    diskLayout->addWidget(m_diskProgressBar);
    mainLayout->addWidget(diskGroup);
    QGroupBox *netGroup = new QGroupBox("Network Speed", this);
    QGridLayout *netLayout = new QGridLayout(netGroup);
    netLayout->addWidget(new QLabel("Download:", this), 0, 0);
    m_netDownValueLabel = new QLabel("- KB/s", this);
    netLayout->addWidget(m_netDownValueLabel, 0, 1);
    netLayout->addWidget(new QLabel("Upload:", this), 1, 0);
    m_netUpValueLabel = new QLabel("- KB/s", this);
    netLayout->addWidget(m_netUpValueLabel, 1, 1);
    mainLayout->addWidget(netGroup);
    return monitorTab;
}

QWidget* MainWindow::createInfoTab()
{
    QWidget *infoTab = new QWidget();
    QGridLayout *layout = new QGridLayout(infoTab);
    layout->addWidget(new QLabel("Hostname:", this), 0, 0);
    m_hostnameValueLabel = new QLabel("-", this);
    layout->addWidget(m_hostnameValueLabel, 0, 1);
    layout->addWidget(new QLabel("Kernel:", this), 1, 0);
    m_kernelValueLabel = new QLabel("-", this);
    m_kernelValueLabel->setWordWrap(true);
    layout->addWidget(m_kernelValueLabel, 1, 1);
    layout->addWidget(new QLabel("CPU Model:", this), 2, 0);
    m_cpuModelValueLabel = new QLabel("-", this);
    m_cpuModelValueLabel->setWordWrap(true);
    layout->addWidget(m_cpuModelValueLabel, 2, 1);
    layout->setColumnStretch(1, 1);
    layout->setRowStretch(3, 1);
    return infoTab;
}