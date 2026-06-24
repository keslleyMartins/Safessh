#include "MainWindow.h"
#include "ConnectionListWidget.h"
#include "TerminalTabWidget.h"
#include "ThemeManager.h"
#include "SettingsDialog.h"
#include "ConnectionDialog.h"

#include <core/SessionManager.h>
#include <core/SSHSession.h>
#include <core/ProtocolHandler.h>
#include <crypto/Vault.h>

#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QTermWidget>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("SafeSSH");
    resize(1280, 800);

    m_sessionManager = new SessionManager(this);
    m_vault          = new Vault(this);
    m_themeManager   = new ThemeManager(this);

    setupUi();
    setupMenuBar();
    setupToolBar();
    setupDockWidgets();
    connectSignals();
    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUi()
{
    auto* central = new QSplitter(Qt::Horizontal, this);
    m_terminalTabs = new TerminalTabWidget(this);
    central->addWidget(m_terminalTabs);
    setCentralWidget(central);
}

void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu(tr("&File"));

    auto* newConnAction = fileMenu->addAction(tr("&New Connection..."));
    newConnAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(newConnAction, &QAction::triggered, this, &MainWindow::onNewConnection);

    auto* importMenu = fileMenu->addMenu(tr("&Import"));
    importMenu->addAction(tr("From Termius..."));
    importMenu->addAction(tr("From ~/.ssh/config..."));

    fileMenu->addSeparator();

    auto* vaultAction = fileMenu->addAction(tr("&Unlock Vault..."));
    vaultAction->setShortcut(QKeySequence("Ctrl+Shift+V"));

    fileMenu->addSeparator();

    auto* settingsAction = fileMenu->addAction(tr("&Settings..."));
    settingsAction->setShortcut(QKeySequence("Ctrl,+"));
    connect(settingsAction, &QAction::triggered, this, [this]() {
        SettingsDialog dlg(m_themeManager, this);
        dlg.exec();
    });

    fileMenu->addSeparator();
    auto* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::setupToolBar()
{
    auto* toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);

    auto* newAction = toolbar->addAction(tr("New"));
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewConnection);

    auto* connectAction = toolbar->addAction(tr("Connect"));
    connect(connectAction, &QAction::triggered, this, [this]() {
        auto selected = m_connectionList->selectedConnection();
        if (!selected.isEmpty()) onOpenConnection(selected);
    });

    toolbar->addSeparator();

    auto* disconnectAction = toolbar->addAction(tr("Disconnect"));
    connect(disconnectAction, &QAction::triggered, this, &MainWindow::onDisconnectCurrent);
}

void MainWindow::setupDockWidgets()
{
    auto* dock = new QDockWidget(tr("Connections"), this);
    m_connectionList = new ConnectionListWidget(m_sessionManager, this);
    dock->setWidget(m_connectionList);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void MainWindow::connectSignals()
{
    connect(m_connectionList, &ConnectionListWidget::connectionActivated,
            this, &MainWindow::onOpenConnection);
}

void MainWindow::loadSettings()
{
    QSettings s("SafeSSH", "SafeSSH");
    restoreGeometry(s.value("geometry").toByteArray());
    restoreState(s.value("windowState").toByteArray());

    auto configPath = s.value("configPath").toString();
    if (!configPath.isEmpty())
        m_sessionManager->loadConnections(configPath);
}

void MainWindow::saveSettings()
{
    QSettings s("SafeSSH", "SafeSSH");
    s.setValue("geometry", saveGeometry());
    s.setValue("windowState", saveState());

    auto configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/connections.json";
    QDir().mkpath(QFileInfo(configPath).absolutePath());
    m_sessionManager->saveConnections(configPath);
    s.setValue("configPath", configPath);
}

// ── Slots ─────────────────────────────────────────────────────────────────

void MainWindow::onNewConnection()
{
    ConnectionDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto cfg = dlg.config();
    if (cfg.name.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Connection name cannot be empty"));
        return;
    }
    m_sessionManager->addConnection(cfg);
}

void MainWindow::onOpenConnection(const QString& name)
{
    auto* cfg = m_sessionManager->findConnection(name);
    if (!cfg) return;

    if (m_sessionManager->activeSession(name)) {
        m_terminalTabs->setCurrentIndex(m_terminalTabs->findTab(name));
        return;
    }

    auto* session = m_sessionManager->openSession(name);
    if (!session) return;

    auto* term = m_terminalTabs->openTerminal(name);
    if (!term) return;

    wireTerminal(term, session);

    session->connect();
}

void MainWindow::onDisconnectCurrent()
{
    auto* term = m_terminalTabs->currentTerminal();
    if (!term) return;
    auto name = m_terminalTabs->currentTabName();
    if (name.isEmpty()) return;

    m_sessionManager->closeSession(name);
    m_terminalTabs->closeCurrentTerminal();
}

void MainWindow::wireTerminal(QObject* termObj, ProtocolHandler* session)
{
    auto* term = qobject_cast<QTermWidget*>(termObj);
    if (!term) return;

    // User types → send to SSH session
    connect(term, &QTermWidget::sendData, this, [session](const char* data, int size) {
        session->write(QByteArray(data, size));
    });

    // SSH receives data → display in terminal
    connect(session, &ProtocolHandler::dataReceived, term, [term](const QByteArray& data) {
        term->putData(data.constData(), data.size());
    });

    // Connection errors → show in terminal
    connect(session, &ProtocolHandler::errorOccurred, term, [term](const QString& msg) {
        auto err = msg.toUtf8();
        term->putData(err.constData(), err.size());
    });

    // Terminal resize → notify session
    connect(term, &QTermWidget::ptySizeChanged, this, [session](int cols, int rows) {
        session->resizeTerminal(cols, rows);
    });
}
