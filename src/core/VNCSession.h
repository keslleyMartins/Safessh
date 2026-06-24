#pragma once
#include "ProtocolHandler.h"

class VNCSession : public ProtocolHandler {
    Q_OBJECT
public:
    using ProtocolHandler::ProtocolHandler;

    void connect() override    { /* TODO: LibVNC integration */ emit errorOccurred("VNC not yet implemented"); }
    void disconnect() override { /* TODO */ }
    void write(const QByteArray&) override { /* TODO */ }
};
