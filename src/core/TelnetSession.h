#pragma once
#include "ProtocolHandler.h"
#include <QTcpSocket>

class TelnetSession : public ProtocolHandler {
    Q_OBJECT
public:
    explicit TelnetSession(const ConnectionConfig& cfg, QObject* parent = nullptr);

    void connect() override;
    void disconnect() override;
    void write(const QByteArray& data) override;

private:
    QTcpSocket* m_socket = nullptr;
    QByteArray m_buffer;
};
