#ifndef COPILOT_H
#define COPILOT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include "../common/systemdata.h"
#include "../core/systemmonitor.h"

class Copilot : public QObject
{
    Q_OBJECT

public:
    explicit Copilot(QObject *parent = nullptr);
    QWidget* createAssistantTab();

public slots:
    void onExplainClicked(const QString& processName);
    void onSystemDataUpdated(const SystemData &data);

signals: // New signal for requesting system data
    void requestSystemData();

private slots:
    void onSendMessageClicked();
    void onChatReplyFinished(QNetworkReply* reply);
    void onExplainReplyFinished(QNetworkReply* reply);


private:
    QString getApiKey();
    void sendChatRequest();
    void appendToChatHistory(const QString& author, const QString& text);

    QNetworkAccessManager *m_networkManager;
    QTextEdit *m_chatHistory;
    QLineEdit *m_chatInput;
    QPushButton *m_sendButton;
    QJsonArray m_chatConversationHistory;
    SystemData m_lastSystemData;
};

#endif // COPILOT_H
