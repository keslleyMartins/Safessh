#include "SerialSession.h"
#include <spdlog/spdlog.h>

SerialSession::SerialSession(const ConnectionConfig& cfg, QObject* parent)
    : ProtocolHandler(cfg, parent)
{
    m_port = new QSerialPort(this);

    connect(m_port, &QSerialPort::readyRead, this, [this]() {
        const auto data = m_port->readAll();
        m_stats.bytesReceived += data.size();
        emit dataReceived(data);
        emit sessionStatsChanged(m_stats);
    });

    connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError) {
        emit errorOccurred(m_port->errorString());
    });
}

void SerialSession::connect()
{
    m_port->setPortName(m_config.host);
    m_port->setBaudRate(QSerialPort::Baud115200);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port->open(QIODevice::ReadWrite)) {
        emit errorOccurred(m_port->errorString());
        return;
    }
    m_connected = true;
    emit connected();
}

void SerialSession::disconnect()
{
    if (m_port->isOpen()) m_port->close();
    m_connected = false;
    emit disconnected();
}

void SerialSession::write(const QByteArray& data)
{
    if (!m_connected) return;
    const auto wrote = m_port->write(data);
    if (wrote > 0) {
        m_stats.bytesSent += wrote;
        emit sessionStatsChanged(m_stats);
    }
}
