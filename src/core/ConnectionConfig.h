#pragma once
#include <QString>
#include <QJsonObject>
#include <cstdint>

enum class ProtocolType : uint8_t {
    SSH,
    Telnet,
    Serial,
    RDP,
    VNC
};

enum class AuthMethod : uint8_t {
    Password,
    KeyFile,
    Agent,
    Interactive,
    GSSAPI,
    None
};

struct ConnectionConfig {
    QString      name;
    ProtocolType protocol = ProtocolType::SSH;
    QString      host;
    uint16_t     port = 22;
    QString      username;
    QString      identityFile;
    AuthMethod   authMethod = AuthMethod::Password;
    QString      vaultEntryId;
    QString      proxyHost;
    uint16_t     proxyPort = 22;
    QString      proxyUser;
    int          timeoutSec = 10;
    QString      group;
    QString      tags;
    bool         keepAlive = true;

    QJsonObject toJson() const;
    static ConnectionConfig fromJson(const QJsonObject& obj);
};
