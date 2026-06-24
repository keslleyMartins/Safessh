#include "ProtocolHandler.h"

ProtocolHandler::ProtocolHandler(const ConnectionConfig& cfg, QObject* parent)
    : QObject(parent)
    , m_config(cfg)
{
}
