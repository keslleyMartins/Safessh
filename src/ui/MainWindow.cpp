#include "MainWindow.h"
#include "ConnectionListWidget.h"
#include "TerminalTabWidget.h"
#include "ThemeManager.h"
#include "ui/SettingsDialog.h"
#include "ui/ConnectionDialog.h"

#include <core/SessionManager.h>
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

    auto* importMenu = fileMenu->addMenu(tr("&Import"));
    importMenu->addAction(tr("From Termius..."));
    importMenu->addAction(tr("From ~/.ssh/config..."));

    fileMenu->addSeparator();

    auto* vaultAction = fileMenu->addAction(tr("&Unlock Vault..."));
    vaultAction->setShortcut(QKeySequence("Ctrl+Shift+V"));

    fileMenu->addSeparator();

    auto* settingsAction = fileMenu->addAction(tr("&Settings..."));
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
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
    toolbar->addAction(tr("New"));
    toolbar->addAction(tr("Connect"));
    toolbar->addSeparator();
    toolbar->addAction(tr("Disconnect"));
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
            this, [this](const QString& name) {
                m_sessionManager->openSession(name);
            });
}

void MainWindow::loadSettings()
{
    QSettings s("SafeSSH", "SafeSSH");
    restoreGeometry(s.value("geometry").toByteArray());
    restoreState(s.value("windowState").toByteArray());
}

void MainWindow::saveSettings()
{
    QSettings s("SafeSSH", "SafeSSH");
    s.setValue("geometry", saveGeometry());
    s.setValue("windowState", saveState());
}
