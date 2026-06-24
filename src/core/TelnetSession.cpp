#include "TelnetSession.h"
#include <spdlog/spdlog.h>

TelnetSession::TelnetSession(const ConnectionConfig& cfg, QObject* parent)
    : ProtocolHandler(cfg, parent)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        m_connected = true;
        emit connected();
    });

    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        m_connected = false;
        emit disconnected();
    });

    connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
        const auto data = m_socket->readAll();
        m_stats.bytesReceived += data.size();
        emit dataReceived(data);
        emit sessionStatsChanged(m_stats);
    });

    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket->errorString());
    });
}

void TelnetSession::connect()
{
    m_socket->connectToHost(m_config.host, m_config.port);
}

void TelnetSession::disconnect()
{
    m_socket->disconnectFromHost();
}

void TelnetSession::write(const QByteArray& data)
{
    if (!m_connected) return;
    const auto wrote = m_socket->write(data);
    if (wrote > 0) {
        m_stats.bytesSent += wrote;
        emit sessionStatsChanged(m_stats);
    }
}
