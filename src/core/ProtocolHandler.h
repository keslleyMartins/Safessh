#pragma once
#include <QObject>
#include <QByteArray>
#include "ConnectionConfig.h"

struct SessionStats {
    qint64 bytesSent = 0;
    qint64 bytesReceived = 0;
    int    exitCode = -1;
};

class ProtocolHandler : public QObject {
    Q_OBJECT
public:
    explicit ProtocolHandler(const ConnectionConfig& cfg, QObject* parent = nullptr);
    virtual ~ProtocolHandler() = default;

    const ConnectionConfig& config() const { return m_config; }
    bool isConnected() const { return m_connected; }

    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual void write(const QByteArray& data) = 0;
    virtual SessionStats stats() const { return m_stats; }

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& message);
    void sessionStatsChanged(const SessionStats& stats);

protected:
    ConnectionConfig m_config;
    bool m_connected = false;
    SessionStats m_stats;
};
