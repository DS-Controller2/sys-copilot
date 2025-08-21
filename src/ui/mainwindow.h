#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QThread>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLineEdit>
#include <QTextEdit>
#include <QJsonArray>
#include <QDockWidget>
#include "core/systemmonitor.h"
#include "common/systemdata.h"
#include "copilot/copilot.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onDynamicDataUpdated(const SystemData &data);
    void onStaticDataReady(const SystemData &data);

signals:
    void systemDataUpdated(const SystemData &data);

private slots:
    void onProcessSelectionChanged();
    void onKillClicked();
    void onStopClicked();
    void onResumeClicked();
    void onExplainClicked();

private:
    void setupUi();
    QWidget* createMonitorTab();
    QWidget* createInfoTab();
    QWidget* createProcessTab();
    void updateProcessTable(const QList<ProcessData> &processes);
    void applyStylesheet(QProgressBar* bar, int value);
    int getSelectedPid();

    SystemMonitor *m_monitor;
    QThread *m_monitorThread;
    Copilot *m_copilot;
    QDockWidget *m_copilotDock;

    // --- Widgets ---
    // Monitor Tab
    QProgressBar *m_cpuProgressBar;
    QProgressBar *m_memProgressBar;
    QProgressBar *m_diskProgressBar;
    QLabel *m_netDownValueLabel;
    QLabel *m_netUpValueLabel;

    // Info Tab
    QLabel *m_hostnameValueLabel;
    QLabel *m_kernelValueLabel;
    QLabel *m_cpuModelValueLabel;

    // Process Tab
    QTableWidget *m_processTableWidget;
    QPushButton *m_killButton;
    QPushButton *m_stopButton;
    QPushButton *m_resumeButton;
    QPushButton *m_explainButton;
};

#endif // MAINWINDOW_H
