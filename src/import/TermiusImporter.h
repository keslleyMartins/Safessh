#pragma once
#include <QVector>
#include <core/ConnectionConfig.h>
#include <QString>

QVector<ConnectionConfig> importTermius(const QString& filePath);
