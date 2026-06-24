#include "ConnectionConfig.h"

static const char* protocolName(ProtocolType p) {
    switch (p) {
        case ProtocolType::SSH:    return "ssh";
        case ProtocolType::Telnet: return "telnet";
        case ProtocolType::Serial: return "serial";
        case ProtocolType::RDP:    return "rdp";
        case ProtocolType::VNC:    return "vnc";
    }
    return "ssh";
}

static ProtocolType protocolFromName(const QString& n) {
    if (n == "telnet") return ProtocolType::Telnet;
    if (n == "serial") return ProtocolType::Serial;
    if (n == "rdp")    return ProtocolType::RDP;
    if (n == "vnc")    return ProtocolType::VNC;
    return ProtocolType::SSH;
}

static const char* authName(AuthMethod m) {
    switch (m) {
        case AuthMethod::Password:    return "password";
        case AuthMethod::KeyFile:     return "key";
        case AuthMethod::Agent:       return "agent";
        case AuthMethod::Interactive: return "interactive";
        case AuthMethod::GSSAPI:      return "gssapi";
        case AuthMethod::None:        return "none";
    }
    return "password";
}

static AuthMethod authFromName(const QString& n) {
    if (n == "key")         return AuthMethod::KeyFile;
    if (n == "agent")       return AuthMethod::Agent;
    if (n == "interactive") return AuthMethod::Interactive;
    if (n == "gssapi")      return AuthMethod::GSSAPI;
    if (n == "none")        return AuthMethod::None;
    return AuthMethod::Password;
}

QJsonObject ConnectionConfig::toJson() const {
    QJsonObject o;
    o["name"]         = name;
    o["protocol"]     = protocolName(protocol);
    o["host"]         = host;
    o["port"]         = port;
    o["username"]     = username;
    o["identityFile"] = identityFile;
    o["authMethod"]   = authName(authMethod);
    o["vaultEntryId"] = vaultEntryId;
    o["proxyHost"]    = proxyHost;
    o["proxyPort"]    = proxyPort;
    o["proxyUser"]    = proxyUser;
    o["timeoutSec"]   = timeoutSec;
    o["group"]        = group;
    o["tags"]         = tags;
    o["keepAlive"]    = keepAlive;
    return o;
}

ConnectionConfig ConnectionConfig::fromJson(const QJsonObject& o) {
    ConnectionConfig c;
    c.name         = o["name"].toString();
    c.protocol     = protocolFromName(o["protocol"].toString());
    c.host         = o["host"].toString();
    c.port         = static_cast<uint16_t>(o["port"].toInt(22));
    c.username     = o["username"].toString();
    c.identityFile = o["identityFile"].toString();
    c.authMethod   = authFromName(o["authMethod"].toString());
    c.vaultEntryId = o["vaultEntryId"].toString();
    c.proxyHost    = o["proxyHost"].toString();
    c.proxyPort    = static_cast<uint16_t>(o["proxyPort"].toInt(22));
    c.proxyUser    = o["proxyUser"].toString();
    c.timeoutSec   = o["timeoutSec"].toInt(10);
    c.group        = o["group"].toString();
    c.tags         = o["tags"].toString();
    c.keepAlive    = o["keepAlive"].toBool(true);
    return c;
}
