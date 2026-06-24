#include "SSHSession.h"
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <spdlog/spdlog.h>

// ---------------------------------------------------------------------------
// SSHWorker — runs in a dedicated thread, owns the libssh session
// ---------------------------------------------------------------------------

SSHWorker::SSHWorker(const ConnectionConfig& cfg, QObject* parent)
    : QObject(parent), m_cfg(cfg)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(50);
    connect(m_pollTimer, &QTimer::timeout, this, &SSHWorker::pollChannel);
}

SSHWorker::~SSHWorker()
{
    doDisconnect();
}

void SSHWorker::doConnect()
{
    spdlog::info("SSH connecting to {}:{} as {}",
                 m_cfg.host.toStdString(), m_cfg.port, m_cfg.username.toStdString());

    m_session = ssh_new();
    if (!m_session) {
        emit errorOccurred("Failed to create SSH session");
        return;
    }

    ssh_options_set(m_session, SSH_OPTIONS_HOST, m_cfg.host.toUtf8().constData());
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_cfg.port);
    ssh_options_set(m_session, SSH_OPTIONS_USER, m_cfg.username.toUtf8().constData());
    ssh_options_set(m_session, SSH_OPTIONS_TIMEOUT, &m_cfg.timeoutSec);

    int verbosity = SSH_LOG_WARNING;
    ssh_options_set(m_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

    int rc = ssh_connect(m_session);
    if (rc != SSH_OK) {
        emit errorOccurred(QString("Connection failed: %1").arg(ssh_get_error(m_session)));
        ssh_free(m_session);
        m_session = nullptr;
        return;
    }

    rc = authenticate();
    if (rc != SSH_AUTH_SUCCESS) {
        emit errorOccurred("Authentication failed");
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
        return;
    }

    rc = openChannel();
    if (rc != SSH_OK) {
        emit errorOccurred("Failed to open SSH channel");
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
        return;
    }

    m_running = true;
    m_pollTimer->start();
    spdlog::info("SSH connected to {}", m_cfg.host.toStdString());
    emit connected();
}

void SSHWorker::doDisconnect()
{
    m_running = false;
    m_pollTimer->stop();

    if (m_channel) {
        ssh_channel_close(m_channel);
        ssh_channel_free(m_channel);
        m_channel = nullptr;
    }
    if (m_session) {
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
    }
    emit disconnected();
}

void SSHWorker::doWrite(const QByteArray& data)
{
    if (!m_channel || !m_running) return;
    int n = ssh_channel_write(m_channel, data.constData(), static_cast<uint32_t>(data.size()));
    if (n < 0) {
        emit errorOccurred(QString("Write error: %1").arg(ssh_get_error(m_session)));
    }
}

void SSHWorker::doResize(int cols, int rows)
{
    if (!m_channel || !m_running) return;
    ssh_channel_change_pty_size(m_channel, cols, rows);
}

// ── private ─────────────────────────────────────────────────────────────────

int SSHWorker::authenticate()
{
    // Try auto (publickey + agent) first
    int rc = ssh_userauth_publickey_auto(m_session, nullptr, nullptr);
    if (rc == SSH_AUTH_SUCCESS) {
        spdlog::info("SSH authenticated via publickey/agent");
        return rc;
    }

    // Try password if available
    QString password = m_password;
    if (password.isEmpty()) password = m_cfg.password;

    if (!password.isEmpty()) {
        rc = ssh_userauth_password(m_session, nullptr, password.toUtf8().constData());
        if (rc == SSH_AUTH_SUCCESS) {
            spdlog::info("SSH authenticated via password");
            return rc;
        }
    }

    // Try keyboard-interactive
    if (ssh_userauth_kbdint(m_session, nullptr, nullptr) == SSH_AUTH_INFO) {
        int more = 1;
        while (more) {
            const char* name = ssh_userauth_kbdint_getname(m_session);
            const char* instruction = ssh_userauth_kbdint_getinstruction(m_session);
            int nPrompts = ssh_userauth_kbdint_getnprompts(m_session);

            // Send password for the first prompt if we have it
            for (int i = 0; i < nPrompts; ++i) {
                char echo;
                const char* prompt = ssh_userauth_kbdint_getprompt(m_session, i, &echo);
                (void)prompt;
                if (!password.isEmpty()) {
                    ssh_userauth_kbdint_setanswer(m_session, i, password.toUtf8().constData());
                } else if (!echo) {
                    ssh_userauth_kbdint_setanswer(m_session, i, "");
                } else {
                    ssh_userauth_kbdint_setanswer(m_session, i, "");
                }
            }
            more = ssh_userauth_kbdint(m_session, nullptr, nullptr);
        }
        if (ssh_userauth_kbdint_getnprompts(m_session) == 0) {
            rc = SSH_AUTH_SUCCESS;
            spdlog::info("SSH authenticated via keyboard-interactive");
            return rc;
        }
    }

    spdlog::warn("All SSH auth methods failed");
    return SSH_AUTH_DENIED;
}

int SSHWorker::openChannel()
{
    m_channel = ssh_channel_new(m_session);
    if (!m_channel) return SSH_ERROR;

    int rc = ssh_channel_open_session(m_channel);
    if (rc != SSH_OK) return rc;

    rc = ssh_channel_request_pty(m_channel);
    if (rc != SSH_OK) return rc;

    rc = ssh_channel_request_shell(m_channel);
    return rc;
}

void SSHWorker::pollChannel()
{
    if (!m_channel || !m_running) return;

    char buf[32768];
    int n = ssh_channel_read_nonblocking(m_channel, buf, sizeof(buf), 0);

    if (n > 0) {
        QByteArray data(buf, n);
        emit dataReceived(data);
    } else if (n == SSH_ERROR) {
        spdlog::error("SSH channel read error: {}", ssh_get_error(m_session));
        m_running = false;
        m_pollTimer->stop();
        emit errorOccurred(QString("Channel error: %1").arg(ssh_get_error(m_session)));
    }

    // Check if channel is closed
    if (ssh_channel_is_eof(m_channel)) {
        spdlog::info("SSH channel EOF");
        doDisconnect();
    }
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

void SSHSession::setPassword(const QString& password)
{
    m_worker->setPassword(password);
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
    QMetaObject::invokeMethod(m_worker, [this, d = data]() { m_worker->doWrite(d); }, Qt::QueuedConnection);
}

void SSHSession::resizeTerminal(int cols, int rows)
{
    QMetaObject::invokeMethod(m_worker, [this, cols, rows]() { m_worker->doResize(cols, rows); }, Qt::QueuedConnection);
}
