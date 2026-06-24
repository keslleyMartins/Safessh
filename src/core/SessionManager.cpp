#include "SessionManager.h"
#include "SSHSession.h"
#include "TelnetSession.h"
#include "SerialSession.h"
#include "RDPSession.h"
#include "VNCSession.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <spdlog/spdlog.h>

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{
}

void SessionManager::loadConnections(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        spdlog::warn("Could not open connections file: {}", filePath.toStdString());
        return;
    }
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return;
    m_connections.clear();
    for (const auto& val : doc.array()) {
        m_connections.push_back(ConnectionConfig::fromJson(val.toObject()));
    }
    emit connectionsChanged();
}

void SessionManager::saveConnections(const QString& filePath)
{
    QJsonArray arr;
    for (const auto& c : m_connections) {
        arr.append(c.toJson());
    }
    QFile f(filePath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    }
}

void SessionManager::addConnection(const ConnectionConfig& cfg)
{
    m_connections.push_back(cfg);
    emit connectionsChanged();
}

void SessionManager::removeConnection(const QString& name)
{
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [&](const ConnectionConfig& c) { return c.name == name; }),
        m_connections.end());
    emit connectionsChanged();
}

ConnectionConfig* SessionManager::findConnection(const QString& name)
{
    for (auto& c : m_connections) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

ProtocolHandler* SessionManager::createHandler(const ConnectionConfig& cfg)
{
    switch (cfg.protocol) {
        case ProtocolType::SSH:    return new SSHSession(cfg, this);
        case ProtocolType::Telnet: return new TelnetSession(cfg, this);
        case ProtocolType::Serial: return new SerialSession(cfg, this);
        case ProtocolType::RDP:    return new RDPSession(cfg, this);
        case ProtocolType::VNC:    return new VNCSession(cfg, this);
    }
    return nullptr;
}

ProtocolHandler* SessionManager::openSession(const QString& connectionName)
{
    auto* cfg = findConnection(connectionName);
    if (!cfg) return nullptr;

    auto it = m_sessions.find(connectionName);
    if (it != m_sessions.end()) return it->second.get();

    auto* handler = createHandler(*cfg);
    handler->connect();
    m_sessions[connectionName] = std::unique_ptr<ProtocolHandler>(handler);
    emit sessionOpened(connectionName);
    return handler;
}

void SessionManager::closeSession(const QString& connectionName)
{
    auto it = m_sessions.find(connectionName);
    if (it == m_sessions.end()) return;
    it->second->disconnect();
    m_sessions.erase(it);
    emit sessionClosed(connectionName);
}

ProtocolHandler* SessionManager::activeSession(const QString& connectionName) const
{
    auto it = m_sessions.find(connectionName);
    return it != m_sessions.end() ? it->second.get() : nullptr;
}
