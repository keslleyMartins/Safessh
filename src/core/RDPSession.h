#pragma once
#include "ProtocolHandler.h"

class RDPSession : public ProtocolHandler {
    Q_OBJECT
public:
    using ProtocolHandler::ProtocolHandler;

    void connect() override    { /* TODO: FreeRDP integration */ emit errorOccurred("RDP not yet implemented"); }
    void disconnect() override { /* TODO */ }
    void write(const QByteArray&) override { /* TODO */ }
};
