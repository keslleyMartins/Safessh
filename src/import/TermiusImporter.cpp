#include "TermiusImporter.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <spdlog/spdlog.h>

QVector<ConnectionConfig> importTermius(const QString& filePath)
{
    QVector<ConnectionConfig> result;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        spdlog::error("Cannot open Termius export: {}", filePath.toStdString());
        return result;
    }

    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return result;

    // Termius exports as a JSON object with "hosts" or "configs" array
    const auto root = doc.object();
    const auto hosts = root.value("hosts").toArray().isEmpty()
        ? root.value("configs").toArray()
        : root.value("hosts").toArray();

    for (const auto& val : hosts) {
        const auto h = val.toObject();
        ConnectionConfig c;
        c.name     = h.value("label").toString();
        c.host     = h.value("hostname").toString();
        c.port     = static_cast<uint16_t>(h.value("port").toInt(22));
        c.username = h.value("username").toString();
        c.group    = h.value("group").toString();

        const auto proto = h.value("protocol").toString().toLower();
        if (proto == "telnet") c.protocol = ProtocolType::Telnet;
        else if (proto == "mosh") c.protocol = ProtocolType::SSH; // mosh ≈ SSH
        else c.protocol = ProtocolType::SSH;

        result.push_back(c);
    }

    spdlog::info("Imported {} connections from Termius", result.size());
    return result;
}
