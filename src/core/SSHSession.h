#pragma once
#include "ProtocolHandler.h"
#include <QThread>
#include <memory>

struct ssh_session_struct;
struct ssh_channel_struct;

class SSHWorker : public QObject {
    Q_OBJECT
public:
    explicit SSHWorker(const ConnectionConfig& cfg, QObject* parent = nullptr);
    ~SSHWorker() override;

public slots:
    void doConnect();
    void doDisconnect();
    void doWrite(const QByteArray& data);

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& msg);

private:
    ConnectionConfig m_cfg;
    ssh_session_struct* m_session = nullptr;
    ssh_channel_struct* m_channel = nullptr;
    bool m_running = false;

    void pollChannel();
    int  authenticate();
};

class SSHSession : public ProtocolHandler {
    Q_OBJECT
public:
    explicit SSHSession(const ConnectionConfig& cfg, QObject* parent = nullptr);
    ~SSHSession() override;

    void connect() override;
    void disconnect() override;
    void write(const QByteArray& data) override;

private:
    QThread m_workerThread;
    SSHWorker* m_worker = nullptr;
};
