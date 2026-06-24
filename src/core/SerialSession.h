#pragma once
#include "ProtocolHandler.h"
#include <QSerialPort>

class SerialSession : public ProtocolHandler {
    Q_OBJECT
public:
    explicit SerialSession(const ConnectionConfig& cfg, QObject* parent = nullptr);

    void connect() override;
    void disconnect() override;
    void write(const QByteArray& data) override;

private:
    QSerialPort* m_port = nullptr;
};
