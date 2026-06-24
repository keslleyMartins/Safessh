#pragma once
#include <QVector>
#include <core/ConnectionConfig.h>
#include <QString>

QVector<ConnectionConfig> importSSHConfig(const QString& filePath);
