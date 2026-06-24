#pragma once
#include <QByteArray>
#include <QString>

QByteArray deriveKey(const QString& password, const QByteArray& salt,
                     int opsLimit = 3, int memLimit = 67108864);
