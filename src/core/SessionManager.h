#pragma once
#include <QObject>
#include <QVector>
#include <memory>
#include "ConnectionConfig.h"

class ProtocolHandler;

class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject* parent = nullptr);

    void loadConnections(const QString& filePath);
    void saveConnections(const QString& filePath);

    void addConnection(const ConnectionConfig& cfg);
    void removeConnection(const QString& name);
    ConnectionConfig* findConnection(const QString& name);

    const QVector<ConnectionConfig>& connections() const { return m_connections; }
    QVector<ConnectionConfig>& connections() { return m_connections; }

    ProtocolHandler* openSession(const QString& connectionName);
    void closeSession(const QString& connectionName);
    ProtocolHandler* activeSession(const QString& connectionName) const;

signals:
    void connectionsChanged();
    void sessionOpened(const QString& name);
    void sessionClosed(const QString& name);

private:
    QVector<ConnectionConfig> m_connections;
    QHash<QString, std::unique_ptr<ProtocolHandler>> m_sessions;

    ProtocolHandler* createHandler(const ConnectionConfig& cfg);
};
