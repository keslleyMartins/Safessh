#include "SSHSession.h"
#include <spdlog/spdlog.h>

// ---------------------------------------------------------------------------
// SSHWorker — runs in a dedicated thread, owns the libssh session
// ---------------------------------------------------------------------------

SSHWorker::SSHWorker(const ConnectionConfig& cfg, QObject* parent)
    : QObject(parent), m_cfg(cfg)
{
}

SSHWorker::~SSHWorker()
{
    doDisconnect();
}

void SSHWorker::doConnect()
{
    // TODO: implement full libssh connect + auth
    spdlog::info("SSHWorker::doConnect to {}:{}", m_cfg.host.toStdString(), m_cfg.port);
    emit errorOccurred("SSH not yet implemented — skeleton");
}

void SSHWorker::doDisconnect()
{
    m_running = false;
    // TODO: libssh_Disconnect, libssh_free
}

void SSHWorker::doWrite(const QByteArray& data)
{
    (void)data;
    // TODO: libssh_channel_write
}

int SSHWorker::authenticate()
{
    // TODO: password / key / agent / interactive
    return -1;
}

// ---------------------------------------------------------------------------
// SSHSession — public API, forwards calls to the worker thread
// ---------------------------------------------------------------------------

SSHSession::SSHSession(const ConnectionConfig& cfg, QObject* parent)
    : ProtocolHandler(cfg, parent)
{
    m_worker = new SSHWorker(cfg);

    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,  m_worker, &QObject::deleteLater);
    connect(m_worker, &SSHWorker::connected,       this,     &ProtocolHandler::connected);
    connect(m_worker, &SSHWorker::disconnected,    this,     &ProtocolHandler::disconnected);
    connect(m_worker, &SSHWorker::dataReceived,    this,     &ProtocolHandler::dataReceived);
    connect(m_worker, &SSHWorker::errorOccurred,   this,     &ProtocolHandler::errorOccurred);

    m_workerThread.start();
}

SSHSession::~SSHSession()
{
    disconnect();
    m_workerThread.quit();
    m_workerThread.wait();
}

void SSHSession::connect()
{
    QMetaObject::invokeMethod(m_worker, "doConnect", Qt::QueuedConnection);
}

void SSHSession::disconnect()
{
    QMetaObject::invokeMethod(m_worker, "doDisconnect", Qt::QueuedConnection);
}

void SSHSession::write(const QByteArray& data)
{
    QMetaObject::invokeMethod(m_worker, [this, data]() { m_worker->doWrite(data); }, Qt::QueuedConnection);
}
