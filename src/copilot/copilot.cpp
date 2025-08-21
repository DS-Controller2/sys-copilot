#include "copilot.h"
#include "../common/systemdata.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>

Copilot::Copilot(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_chatHistory(new QTextEdit(nullptr)),
      m_chatInput(new QLineEdit(nullptr)),
      m_sendButton(new QPushButton("Send", nullptr))
{
    m_chatHistory->setReadOnly(true);
    connect(m_sendButton, &QPushButton::clicked, this, &Copilot::onSendMessageClicked);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &Copilot::onSendMessageClicked);
}

void Copilot::onSystemDataUpdated(const SystemData &data)
{
    m_lastSystemData = data;
}

QWidget* Copilot::createAssistantTab()
{
    QWidget *assistantTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(assistantTab);
    layout->addWidget(m_chatHistory);
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_chatInput);
    inputLayout->addWidget(m_sendButton);
    layout->addLayout(inputLayout);
    return assistantTab;
}

void Copilot::onSendMessageClicked()
{
    QString userInput = m_chatInput->text().trimmed();
    if (userInput.isEmpty()) return;

    appendToChatHistory("User", userInput);
    m_chatInput->clear();

    QJsonObject userPart;
    userPart["text"] = userInput;
    QJsonObject userTurn;
    userTurn["role"] = "user";
    userTurn["parts"] = QJsonArray({userPart});
    m_chatConversationHistory.append(userTurn);

    sendChatRequest();
}

void Copilot::sendChatRequest()
{
    QString apiKey = getApiKey();
    if (apiKey.isEmpty()) {
        appendToChatHistory("System", "API Key not found. Please check your .env file.");
        return;
    }

    QString url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent?key=" + apiKey;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject functionDeclaration;
    functionDeclaration["name"] = "run_shell_command";
    functionDeclaration["description"] = "Executes a shell command to get information about the system.";
    QJsonObject commandParam;
    commandParam["type"] = "STRING";
    commandParam["description"] = "The shell command to execute.";
    QJsonObject properties;
    properties["command"] = commandParam;
    QJsonObject parameters;
    parameters["type"] = "OBJECT";
    parameters["properties"] = properties;
    functionDeclaration["parameters"] = parameters;

    QJsonObject functionDeclarationSystemInfo;
    functionDeclarationSystemInfo["name"] = "getSystemInfo";
    functionDeclarationSystemInfo["description"] = "Retrieves comprehensive system information including CPU, memory, disk, network usage, and process list.";
    QJsonObject parametersSystemInfo;
    parametersSystemInfo["type"] = "OBJECT";
    parametersSystemInfo["properties"] = QJsonObject(); // No properties for this function
    functionDeclarationSystemInfo["parameters"] = parametersSystemInfo;

    QJsonObject functionDeclarationKillProcess;
    functionDeclarationKillProcess["name"] = "killProcess";
    functionDeclarationKillProcess["description"] = "Kills a process given its PID.";
    QJsonObject pidParamKill;
    pidParamKill["type"] = "NUMBER";
    pidParamKill["description"] = "The PID of the process to kill.";
    QJsonObject propertiesKill;
    propertiesKill["pid"] = pidParamKill;
    QJsonObject parametersKill;
    parametersKill["type"] = "OBJECT";
    parametersKill["properties"] = propertiesKill;
    functionDeclarationKillProcess["parameters"] = parametersKill;

    QJsonObject functionDeclarationStopProcess;
    functionDeclarationStopProcess["name"] = "stopProcess";
    functionDeclarationStopProcess["description"] = "Stops a process given its PID.";
    QJsonObject pidParamStop;
    pidParamStop["type"] = "NUMBER";
    pidParamStop["description"] = "The PID of the process to stop.";
    QJsonObject propertiesStop;
    propertiesStop["pid"] = pidParamStop;
    QJsonObject parametersStop;
    parametersStop["type"] = "OBJECT";
    parametersStop["properties"] = propertiesStop;
    functionDeclarationStopProcess["parameters"] = parametersStop;

    QJsonObject functionDeclarationResumeProcess;
    functionDeclarationResumeProcess["name"] = "resumeProcess";
    functionDeclarationResumeProcess["description"] = "Resumes a stopped process given its PID.";
    QJsonObject pidParamResume;
    pidParamResume["type"] = "NUMBER";
    pidParamResume["description"] = "The PID of the process to resume.";
    QJsonObject propertiesResume;
    propertiesResume["pid"] = pidParamResume;
    QJsonObject parametersResume;
    parametersResume["type"] = "OBJECT";
    parametersResume["properties"] = propertiesResume;
    functionDeclarationResumeProcess["parameters"] = parametersResume;

    QJsonObject functionDeclarationFindProcessPid;
    functionDeclarationFindProcessPid["name"] = "findProcessPid";
    functionDeclarationFindProcessPid["description"] = "Finds the PID of a process given its name.";
    QJsonObject nameParamFind;
    nameParamFind["type"] = "STRING";
    nameParamFind["description"] = "The name of the process to find.";
    QJsonObject propertiesFind;
    propertiesFind["name"] = nameParamFind;
    QJsonObject parametersFind;
    parametersFind["type"] = "OBJECT";
    parametersFind["properties"] = propertiesFind;
    functionDeclarationFindProcessPid["parameters"] = parametersFind;

    QJsonObject tool;
    tool["function_declarations"] = QJsonArray({functionDeclaration, functionDeclarationSystemInfo, functionDeclarationKillProcess, functionDeclarationStopProcess, functionDeclarationResumeProcess, functionDeclarationFindProcessPid});

    QJsonObject payload;
    payload["contents"] = m_chatConversationHistory;
    payload["tools"] = QJsonArray({tool});

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onChatReplyFinished(reply); });
}

void Copilot::onChatReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        appendToChatHistory("System", "Network Error: " + reply->errorString() + "\n" + reply->readAll());
        reply->deleteLater();
        return;
    }

    QByteArray response_data = reply->readAll();
    reply->deleteLater();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response_data);
    QJsonObject jsonObj = jsonDoc.object();

    if (!jsonObj.contains("candidates") || !jsonObj["candidates"].isArray()) {
        appendToChatHistory("System", "Invalid API response format.");
        return;
    }

    QJsonObject firstCandidate = jsonObj["candidates"].toArray()[0].toObject();
    QJsonObject content = firstCandidate["content"].toObject();
    QJsonArray parts = content["parts"].toArray();
    QJsonObject firstPart = parts[0].toObject();

    QJsonObject modelTurn;
    modelTurn["role"] = "model";
    modelTurn["parts"] = parts;
    m_chatConversationHistory.append(modelTurn);

    if (firstPart.contains("functionCall")) {
        QJsonObject functionCall = firstPart["functionCall"].toObject();
        QString functionName = functionCall["name"].toString();
        QJsonObject args = functionCall["args"].toObject();
        QString command = args["command"].toString();

        QMessageBox::StandardButton confirmation;
        confirmation = QMessageBox::question(nullptr, "Confirm Command Execution",
            QString("The AI assistant wants to run the following command:\n\n%1\n\nDo you approve?").arg(command),
            QMessageBox::Yes | QMessageBox::No);

        if (functionName == "run_shell_command") {
            QMessageBox::StandardButton confirmation;
            confirmation = QMessageBox::question(nullptr, "Confirm Command Execution",
                QString("The AI assistant wants to run the following command:\n\n%1\n\nDo you approve?").arg(command),
                QMessageBox::Yes | QMessageBox::No);

            if (confirmation == QMessageBox::Yes) {
                QProcess *process = new QProcess(this);
                connect(process, &QProcess::finished, this, [this, process, functionName](int exitCode, QProcess::ExitStatus exitStatus){
                    QString output = process->readAllStandardOutput();
                    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                        output += "\nError: " + process->readAllStandardError();
                    }

                    QJsonObject responseContent;
                    responseContent["content"] = output;

                    QJsonObject functionResponse;
                    functionResponse["name"] = functionName;
                    functionResponse["response"] = responseContent;

                    QJsonObject toolPart;
                    toolPart["functionResponse"] = functionResponse;

                    QJsonObject toolTurn;
                    toolTurn["role"] = "tool";
                    toolTurn["parts"] = QJsonArray({toolPart});

                    m_chatConversationHistory.append(toolTurn);

                    sendChatRequest();
                    process->deleteLater();
                });
                process->start("bash", {"-c", command});
            } else {
                appendToChatHistory("System", "Command execution denied by user.");
                QJsonObject emptyResponse;
                emptyResponse["content"] = "User denied execution.";
                QJsonObject functionResponse;
                functionResponse["name"] = functionName;
                functionResponse["response"] = emptyResponse;
                QJsonObject toolPart;
                toolPart["functionResponse"] = functionResponse;
                QJsonObject toolTurn;
                toolTurn["role"] = "tool";
                toolTurn["parts"] = QJsonArray({toolPart});
                m_chatConversationHistory.append(toolTurn);
                sendChatRequest();
            }
        } else if (functionName == "getSystemInfo") {
            QJsonObject systemInfoJson;
            systemInfoJson["hostname"] = m_lastSystemData.hostname;
            systemInfoJson["kernelVersion"] = m_lastSystemData.kernelVersion;
            systemInfoJson["cpuModel"] = m_lastSystemData.cpuModel;
            systemInfoJson["cpuPercentage"] = m_lastSystemData.cpuPercentage;
            systemInfoJson["memPercentage"] = m_lastSystemData.memPercentage;
            systemInfoJson["totalSystemMemoryMB"] = m_lastSystemData.totalSystemMemoryMB;
            systemInfoJson["diskPercentage"] = m_lastSystemData.diskPercentage;
            systemInfoJson["netDownSpeed_KBps"] = m_lastSystemData.netDownSpeed_KBps;
            systemInfoJson["netUpSpeed_KBps"] = m_lastSystemData.netUpSpeed_KBps;

            QJsonArray processesArray;
            for (const ProcessData &p_data : m_lastSystemData.processes) {
                QJsonObject processObject;
                processObject["pid"] = p_data.pid;
                processObject["name"] = p_data.name;
                processObject["memUsageMB"] = p_data.memUsageMB;
                processesArray.append(processObject);
            }
            systemInfoJson["processes"] = processesArray;

            QJsonObject functionResponse;
            functionResponse["name"] = functionName;
            functionResponse["response"] = systemInfoJson;

            QJsonObject toolPart;
            toolPart["functionResponse"] = functionResponse;

            QJsonObject toolTurn;
            toolTurn["role"] = "tool";
            toolTurn["parts"] = QJsonArray({toolPart});

            m_chatConversationHistory.append(toolTurn);

            sendChatRequest();
        } else if (functionName == "killProcess" || functionName == "stopProcess" || functionName == "resumeProcess") {
            int pid = args["pid"].toInt();
            QString command;
            if (functionName == "killProcess") command = QString("kill %1").arg(pid);
            else if (functionName == "stopProcess") command = QString("kill -STOP %1").arg(pid);
            else if (functionName == "resumeProcess") command = QString("kill -CONT %1").arg(pid);

            QProcess *process = new QProcess(this);
            connect(process, &QProcess::finished, this, [this, process, functionName, pid](int exitCode, QProcess::ExitStatus exitStatus){
                QString output;
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    QString funcNameCopy = functionName; // Create a non-const copy
                    output = QString("Process %1 (PID: %2) %3 successfully.").arg(funcNameCopy.replace("Process", ""), QString::number(pid), (functionName == "killProcess" ? "killed" : (functionName == "stopProcess" ? "stopped" : "resumed")));
                } else {
                    QString funcNameCopy = functionName; // Create a non-const copy
                    output = QString("Failed to %1 process %2 (PID: %3). Error: %4").arg(funcNameCopy.replace("Process", ""), QString::number(pid), process->readAllStandardError());
                }

                QJsonObject responseContent;
                responseContent["content"] = output;

                QJsonObject functionResponse;
                functionResponse["name"] = functionName;
                functionResponse["response"] = responseContent;

                QJsonObject toolPart;
                toolPart["functionResponse"] = functionResponse;

                QJsonObject toolTurn;
                toolTurn["role"] = "tool";
                toolTurn["parts"] = QJsonArray({toolPart});

                m_chatConversationHistory.append(toolTurn);
                sendChatRequest();
                process->deleteLater();
            });
            process->start("bash", {"-c", command});
        } else if (functionName == "findProcessPid") {
            QString processName = args["name"].toString();
            int pid = -1;
            for (const ProcessData &p_data : m_lastSystemData.processes) {
                if (p_data.name.compare(processName, Qt::CaseInsensitive) == 0) {
                    pid = p_data.pid;
                    break;
                }
            }

            QString output;
            if (pid != -1) {
                output = QString("Found PID %1 for process '%2'.").arg(pid).arg(processName);
            } else {
                output = QString("Could not find process '%1'.").arg(processName);
            }

            QJsonObject responseContent;
            responseContent["content"] = output;

            QJsonObject functionResponse;
            functionResponse["name"] = functionName;
            functionResponse["response"] = responseContent;

            QJsonObject toolPart;
            toolPart["functionResponse"] = functionResponse;

            QJsonObject toolTurn;
            toolTurn["role"] = "tool";
            toolTurn["parts"] = QJsonArray({toolPart});

            m_chatConversationHistory.append(toolTurn);
            sendChatRequest();
        }

    } else if (firstPart.contains("text")) {
        QString responseText = firstPart["text"].toString();
        appendToChatHistory("AI Assistant", responseText);
    }
}

void Copilot::appendToChatHistory(const QString& author, const QString& text)
{
    m_chatHistory->append(QString("<b>%1:</b><br>%2<br>").arg(author, text.toHtmlEscaped().replace("\n", "<br>")));
}

QString Copilot::getApiKey()
{
    QFile envFile(":/.env");
    if (!envFile.open(QIODevice::ReadOnly | QIODevice::Text)) return "";
    QTextStream in(&envFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("GEMINI_API_KEY=")) return line.mid(15);
    }
    return "";
}

void Copilot::onExplainClicked(const QString& processName)
{
    QString apiKey = getApiKey();
    if (apiKey.isEmpty()) {
        QMessageBox::critical(nullptr, "API Key Error", "Could not find GEMINI_API_KEY in the embedded .env file.");
        return;
    }

    QString url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent?key=" + apiKey;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject textPart; textPart["text"] = QString("Explain the purpose of the Linux process named '%1'. What does it typically do, and is it safe? Keep the explanation concise and easy to understand for a non-expert.").arg(processName);
    QJsonObject content; content["parts"] = QJsonArray({textPart});
    QJsonObject root; root["contents"] = QJsonArray({content});
    QJsonDocument doc(root);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onExplainReplyFinished(reply); });
}

void Copilot::onExplainReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(nullptr, "Network Error", "Failed to get explanation: " + reply->errorString() + "\n\n" + reply->readAll());
        reply->deleteLater();
        return;
    }

    QByteArray response_data = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response_data);
    QJsonObject jsonObj = jsonDoc.object();
    QString explanation = "Could not parse explanation from API response.";
    if (jsonObj.contains("candidates") && jsonObj["candidates"].isArray()) {
        QJsonArray candidates = jsonObj["candidates"].toArray();
        if (!candidates.isEmpty()) {
            QJsonObject firstCandidate = candidates[0].toObject();
            if (firstCandidate.contains("content")) {
                QJsonObject content = firstCandidate["content"].toObject();
                if (content.contains("parts") && content["parts"].isArray()) {
                    explanation = content["parts"].toArray()[0].toObject()["text"].toString();
                }
            }
        }
    }
    QMessageBox::information(nullptr, "Process Explanation", explanation);
    reply->deleteLater();
}
