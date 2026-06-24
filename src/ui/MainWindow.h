#pragma once
#include <QMainWindow>

class SessionManager;
class Vault;
class ConnectionListWidget;
class TerminalTabWidget;
class ThemeManager;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewConnection();
    void onOpenConnection(const QString& name);
    void onDisconnectCurrent();
    void onUnlockVault();
    void onImportTermius();
    void onImportSSHConfig();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupDockWidgets();
    void connectSignals();
    void loadSettings();
    void saveSettings();

    void wireTerminal(QObject* term, class ProtocolHandler* session);

    SessionManager*       m_sessionManager = nullptr;
    Vault*                m_vault = nullptr;
    ConnectionListWidget* m_connectionList = nullptr;
    TerminalTabWidget*    m_terminalTabs = nullptr;
    ThemeManager*         m_themeManager = nullptr;
};
