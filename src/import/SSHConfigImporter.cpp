#include "SSHConfigImporter.h"
#include <QFile>
#include <QTextStream>
#include <spdlog/spdlog.h>

QVector<ConnectionConfig> importSSHConfig(const QString& filePath)
{
    QVector<ConnectionConfig> result;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        spdlog::error("Cannot open SSH config: {}", filePath.toStdString());
        return result;
    }

    QTextStream in(&f);
    ConnectionConfig current;
    bool inHost = false;

    auto flushHost = [&]() {
        if (inHost && !current.host.isEmpty()) {
            if (current.name.isEmpty()) current.name = current.host;
            result.push_back(current);
        }
        current = ConnectionConfig();
        inHost = true;
    };

    while (!in.atEnd()) {
        auto line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        if (line.startsWith("Host ", Qt::CaseInsensitive)) {
            flushHost();
            current.name = line.section(' ', 1).trimmed();
            continue;
        }

        auto key   = line.section(' ', 0, 0).toLower();
        auto value = line.section(' ', 1).trimmed();

        if (key == "hostname")       current.host         = value;
        else if (key == "port")      current.port         = value.toUShort();
        else if (key == "user")      current.username     = value;
        else if (key == "identityfile") current.identityFile = value;
        else if (key == "proxyjump") {
            // ProxyJump format: user@host:port
            auto at = value.indexOf('@');
            auto co = value.indexOf(':');
            if (at >= 0) {
                current.proxyUser = value.left(at);
                current.proxyHost = value.mid(at + 1, co - at - 1);
            } else {
                current.proxyHost = value.left(co);
            }
            if (co >= 0) current.proxyPort = value.mid(co + 1).toUShort();
        }
    }
    flushHost();

    spdlog::info("Imported {} hosts from SSH config", result.size());
    return result;
}
